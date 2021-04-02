 #define LOG_TAG "sensor_pdaf"

#include <cutils/trace.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/sockets.h>
#include "sensor_drv_u.h"
#include "dlfcn.h"
#include <fcntl.h>
#include <cutils/properties.h>

#define DEFAULT_ROI_WIDTH 512
#define DEFAULT_ROI_HEIGHT 384

#define PIXEL_NUMB_FOR_GM1 42240
#define PD_LINE_WIDTH_FOR_IMX258 1600
#define PD_BUFFER_OFFSET_FOR_IMX258 80

#define PDAF_BUFFER_DUMP_PATH "data/vendor/cameraserver/"
#define SENSOR_PDAF_INIT_INT(a, b, c)           \
    do {                                        \
        a = 0;                                  \
        b = 0;                                  \
        c = 0;                                  \
    } while(0)


#define SENSOR_PDAF_FREE_BUFFER(a)              \
    do {                                        \
        if(a != NULL) {                         \
            free(a);                            \
            a = NULL;                           \
        } else                                  \
            SENSOR_LOGD("invid buffer");        \
    } while(0)

#define SENSOR_PDAF_CHECK_POINTER(a)            \
    do {                                        \
        if(!a) {                                \
            SENSOR_LOGD("invid param");         \
            return SENSOR_FAIL;                 \
        }                                       \
    } while(0)
#define SENSOR_PDAF_4BYTE_IGNMENT(a) ((a % 4) ? (a) : (a + 4 - a % 4))

#define SENSOR_PDAF_ABS_value(a) ((a < 0) ? (0) : (a))

struct pdaf_roi_param {
    cmr_int roi_start_x;
    cmr_int roi_start_y;
    cmr_int roi_width;
    cmr_int roi_height;
    cmr_int pd_area_width;
    cmr_int pd_area_height;
    cmr_int pd_area_sum;
    cmr_int line_offset_x[1024];
    cmr_int line_offset_y[1024];
    cmr_u16 *pd_pos_info;
};

enum PDAF_DATA_PROCESS {
    PDAF_INVID_BUFFER,
    PDAF_TYPE2_BUFFER = 2,
    PDAF_TYPE3_BUFFER = 3
};

cmr_int sensor_pdaf_dump_func(void *buffer, cmr_int dump_numb, unsigned char *name);
cmr_int sensor_pdaf_dump_func_with_frameid(void *buffer, cmr_int dump_numb, unsigned char *name, int frame_id);
cmr_int sensor_pdaf_mipi_data_processor(cmr_u8 *input_buf, cmr_u32 *output_buf, cmr_u32 size);
cmr_int sensor_pdaf_data_extraction(cmr_u16 *src, cmr_u16 *dst, struct pdaf_roi_param *roi_param);
cmr_int sensor_pdaf_type3_convertor(struct pdaf_buffer_handle *pdaf_buffer,
                                    struct pdaf_roi_param *roi_param);
cmr_int sensor_pdaf_mipi_raw_type3(void *buffer_handle, cmr_u32 *param);
cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param);
cmr_int sensor_pdaf_mipi_raw_type2(void *buffer_handle, cmr_u32 *param);
cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param);

double tick(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec + 1E-6 * t.tv_usec;
}

cmr_int sensor_pdaf_mipi_data_processor(cmr_u8 *input_buf, cmr_u32 *output_buf, cmr_u32 size) {
    cmr_u32 convertor[5];
    int count = 0;
    int judge = 0;
    cmr_u32 *pointer_for_output = output_buf;
    for(int i = 0; i < size; i++) {
        convertor[count++] = (cmr_u32)input_buf[i];
        if((count % 5) == 0) {
            count = 0;
            convertor[0] = ((convertor[0] << 2) | (convertor[4] & 0x03));
            convertor[1] = ((convertor[1] << 2) | (convertor[4] & 0x0C) >> 2);
            convertor[2] = ((convertor[2] << 2) | (convertor[4] & 0x30) >> 4);
            convertor[3] = ((convertor[3] << 2) | (convertor[4] & 0xC0) >> 6);
            memcpy(pointer_for_output + 4 * judge, convertor, 4 * sizeof(cmr_u32));
            judge++;
        }
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_data_extraction(cmr_u16 *src, cmr_u16 *dst, struct pdaf_roi_param *roi_param) {
    cmr_int start_pos = roi_param->roi_start_y * roi_param->pd_area_width + roi_param->roi_start_x;
    cmr_u16 *pointer_for_src = src + start_pos;
    cmr_u16 *pointer_for_dst = dst;
    cmr_int roi_line_feed = roi_param->roi_width;
    cmr_int pd_area_width = roi_param->pd_area_width;
    cmr_int pd_area_height = roi_param->pd_area_height;
    memset(dst, 0, pd_area_width * pd_area_height * sizeof(cmr_u16));
    for(int i = 0 ; i < roi_param->roi_height; i++)
        memcpy(pointer_for_dst + i * roi_line_feed,
                pointer_for_src + i * pd_area_width, sizeof(cmr_u16) * roi_line_feed);
    return SENSOR_SUCCESS;
}

void sensor_pdaf_calculate_roi(struct sensor_pdaf_info *pdaf_info, struct pdaf_buffer_handle *buffer_handle,
                                struct pdaf_roi_param *roi_param) {
    cmr_int pdaf_area_width, pdaf_area_height;
    cmr_int base_width, base_height, pd_area_start_x, pd_area_start_y;
    cmr_int pd_block_w, pd_block_h, offset_width, offset_height;
    cmr_int roi_img_start_x, roi_img_start_y, roi_pdaf_start_x, roi_pdaf_start_y;
    cmr_int roi_width, roi_height;
    cmr_int block_number_x, block_number_y;
    pd_block_h = 8 * pdaf_info->pd_block_w;
    pd_block_w = 8 * pdaf_info->pd_block_h;
    pdaf_area_height = buffer_handle->roi_param.roi_area_height / (8 << pdaf_info->pd_block_h);
    pdaf_area_width = buffer_handle->roi_param.roi_area_width / (8 << pdaf_info->pd_block_w);
    pd_area_start_x = pdaf_info->pd_offset_x;
    pd_area_start_y = pdaf_info->pd_offset_y;
    base_width = DEFAULT_ROI_WIDTH;
    base_height = DEFAULT_ROI_HEIGHT;
    memset(roi_param, 0, sizeof(struct pdaf_roi_param));
    roi_img_start_x = buffer_handle->roi_param.roi_start_x;
    roi_img_start_y = buffer_handle->roi_param.roi_start_y;
    roi_pdaf_start_x = roi_img_start_x / (8 << pdaf_info->pd_block_w);
    roi_pdaf_start_y = roi_img_start_y / (8 << pdaf_info->pd_block_h);

#ifdef PDAF_CC_ROI
    if((offset_width / pd_block_w) > 1) {
        if(((offset_width / pd_block_w) % 2) == 0) {
            pd_area_start_x += offset_width / 2;
            roi_img_start_x += offset_width / 2;
        } else {
            pd_area_start_x += ((offset_width - pd_block_w) / 2);
            roi_img_start_x += ((offset_width - pd_block_w) / 2);
        }
    }
    if((offset_height / pd_block_h) > 1) {
        if(((offset_height / pd_block_h) % 2) == 0) {
            pd_area_start_y += offset_height / 2;
            roi_img_start_y += offset_height / 2;
        } else {
            pd_area_start_y += ((offset_height - pd_block_h) / 2);
            roi_img_start_y += ((offset_height - pd_block_h) / 2);
        }
    }
#endif

    roi_param->pd_area_height = pdaf_info->pd_block_num_y *
                                pdaf_info->descriptor->block_height;
    roi_param->pd_area_width = pdaf_info->pd_block_num_x *
                               pdaf_info->descriptor->block_width;
    roi_param->roi_height = pdaf_area_height * pdaf_info->descriptor->block_height;
    roi_param->roi_width = pdaf_area_width * pdaf_info->descriptor->block_width;
    roi_param->roi_start_x = roi_pdaf_start_x * pdaf_info->descriptor->block_width;
    roi_param->roi_start_y = roi_pdaf_start_y * pdaf_info->descriptor->block_height;
    roi_param->pd_pos_info = pdaf_info->pd_is_right;
    SENSOR_LOGV("ROI param is: height [%d], width [%d], roi_start_x [%d], roi_start_y [%d]",
                roi_param->roi_height, roi_param->roi_width, roi_param->roi_start_x, roi_param->roi_start_y);
    SENSOR_LOGV("FULL param is: height [%d], width [%d]",
                roi_param->pd_area_height, roi_param->pd_area_width);
    return;
}

cmr_int sensor_pdaf_type3_convertor(struct pdaf_buffer_handle *pdaf_buffer,
                                    struct pdaf_roi_param *roi_param)
{
    cmr_u8 *right_in = pdaf_buffer->right_buffer;
    cmr_u8 *left_in = pdaf_buffer->left_buffer;
    cmr_u8 ucByte[4];
    cmr_u32 *right_out = (int *)pdaf_buffer->right_output;
    cmr_u32 *left_out = (int *)pdaf_buffer->left_output;
    int a_dNumL = pdaf_buffer->roi_pixel_numb;
    int PD_bytesL = (a_dNumL / 3 ) << 2;
    int PD_bytesR = (a_dNumL /3 ) << 2;
    int indexL, indexR, i;
    SENSOR_PDAF_INIT_INT(indexL, indexR, i);
    for(i = 0;i < PD_bytesL; i += 4) {
        ucByte[0] = *(left_in + i);
        ucByte[1] = *(left_in + i + 1);
        ucByte[2] = *(left_in + i + 2);
        ucByte[3] = *(left_in + i + 3);

        *(left_out + indexL)   = ((ucByte[1] & 0x03) << 8) + ucByte[0];
        *(left_out+indexL + 1) = ((ucByte[2] & 0x0F) << 6) + ((ucByte[1] & 0xFC) >> 2);
        *(left_out+indexL + 2) = ((ucByte[3] & 0x3F) << 4) + ((ucByte[2] & 0xF0) >> 4);
        indexL = indexL + 3;

        ucByte[0] = *(right_in + i);
        ucByte[1] = *(right_in + i + 1);
        ucByte[2] = *(right_in + i + 2);
        ucByte[3] = *(right_in + i + 3);

        *(right_out + indexR) = ((ucByte[1] & 0x03) << 8) + ucByte[0];
        *(right_out + indexR + 1) = ((ucByte[2] & 0x0F) << 6) + ((ucByte[1] & 0xFC) >> 2);
        *(right_out + indexR + 2) = ((ucByte[3] & 0x3F) << 4) + ((ucByte[2] & 0xF0) >> 4);
        indexR = indexR + 3;
    }
    return 0;
}

cmr_int sensor_pdaf_dump_func(void *buffer, cmr_int dump_numb, unsigned char *name) {
    unsigned char file_path[128];
    sprintf(file_path, "%s_%s", PDAF_BUFFER_DUMP_PATH, name);
    FILE *fp = fopen(file_path, "wb+");
    SENSOR_PDAF_CHECK_POINTER(fp);
    fwrite(buffer, dump_numb, 1, fp);
    fclose(fp);
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_dump_func_with_frameid(void *buffer, cmr_int dump_numb, unsigned char *name, int frame_id) {
    unsigned char file_path[128];
    sprintf(file_path, "%s%d_%p_%s", PDAF_BUFFER_DUMP_PATH, frame_id, buffer, name);
    FILE *fp = fopen(file_path, "wb+");
    SENSOR_PDAF_CHECK_POINTER(fp);
    fwrite(buffer, dump_numb, 1, fp);
    fclose(fp);
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_mipi_raw_type3(void *buffer_handle, cmr_u32 *param) {
    struct sensor_pdaf_info *pdaf_info = (struct sensor_pdaf_info *)param;
    struct pdaf_buffer_handle *pdaf_buffer = (struct pdaf_buffer_handle *)buffer_handle;
    struct pdaf_roi_param roi_param;
    char property_value[128];
    int dump_numb;

    sensor_pdaf_calculate_roi(pdaf_info, pdaf_buffer, &roi_param);
    sensor_pdaf_type3_convertor(pdaf_buffer, &roi_param);
    dump_numb = pdaf_buffer->roi_pixel_numb;
    property_get("persist.vendor.cam.pdaf.dump.type3", property_value, "0");
    if(atoi(property_value)) {
        sensor_pdaf_dump_func(pdaf_buffer->right_buffer, dump_numb * 4 / 3, "type3_right_input.raw");
        sensor_pdaf_dump_func(pdaf_buffer->left_buffer, dump_numb * 4 / 3, "type3_left_input.raw");
        sensor_pdaf_dump_func(pdaf_buffer->right_output, dump_numb * 4, "type3_right_output.raw");
        sensor_pdaf_dump_func(pdaf_buffer->left_output, dump_numb * 4, "type3_left_output.raw");
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_buffer_block_seprator(struct pdaf_block_descriptor *descriptor,
                                          struct pdaf_buffer_handle *pdaf_buffer,
                                          struct pdaf_roi_param *roi_param) {
    struct pdaf_coordinate_tab *judger = descriptor->pd_line_coordinate;
    cmr_int block_height, line_coordinate, colum_coordinate, right_line, left_line, line_width, line_height;
    cmr_int left_flag, right_flag, output_line_width;
    cmr_u32 *left = (cmr_u32 *)pdaf_buffer->left_output;
    cmr_u32 *right = (cmr_u32 *)pdaf_buffer->right_output;
    cmr_u32 *input = (cmr_u32 *)pdaf_buffer->right_buffer;
    int output_line, num;

    SENSOR_PDAF_INIT_INT(output_line, left_line, right_line);
    SENSOR_PDAF_INIT_INT(colum_coordinate, line_coordinate, output_line_width);
    block_height = descriptor->block_height;
    line_width = roi_param->roi_width;
    line_height = roi_param->roi_height;
    output_line_width = line_width / 2;

    SENSOR_LOGV("line width -> %d, line height -> %d, output_line_width -> %d",
                 line_width, line_height, output_line_width);
    for(int i = 0; i < line_height; i++) {
        SENSOR_PDAF_INIT_INT(left_flag, right_flag, line_coordinate);
        for(int j = 0; j < line_width; j++) {
            if(roi_param->pd_pos_info[colum_coordinate * descriptor->block_width + line_coordinate]) {
                *(right + (right_line * output_line_width + right_flag)) =
                       *(input + (i * line_width + j));
                right_flag++;
            } else {
                *(left + (left_line * output_line_width + left_flag)) =
                       *(input + (i * line_width + j));
                left_flag++;
            }
            line_coordinate++;
            if(line_coordinate == descriptor->block_width)
                line_coordinate = 0;
        }
        colum_coordinate = ((i + 1) % descriptor->block_height);
        if(right_flag)
            right_line++;
        if(left_flag)
            left_line++;
    }

    SENSOR_LOGV("block seprator with left_line -> %d, right_line -> %d",
                 left_line, right_line);
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_buffer_lined_seprator(struct pdaf_block_descriptor *descriptor,
                                          struct pdaf_buffer_handle *pdaf_buffer,
                                          struct pdaf_roi_param *roi_param) {
    cmr_u8 *pointer_for_src = NULL;
    cmr_u8 *pointer_for_right, *pointer_for_left;
    cmr_int *pd_coordinate_tab = descriptor->coordinate_tab;
    cmr_int coordinate_tab, line_feed, line_height;
    cmr_int right_line, left_line;
    int i;
    unsigned char *temp_buff = (cmr_u8 *)pdaf_buffer->right_buffer;
    pointer_for_src = (cmr_u8 *)pdaf_buffer->right_buffer;
    line_feed = roi_param->roi_width;
    line_height = roi_param->roi_height;
    pointer_for_right = (cmr_u8 *)pdaf_buffer->right_output;
    pointer_for_left = (cmr_u8 *)pdaf_buffer->left_output;

    SENSOR_PDAF_INIT_INT(coordinate_tab, right_line, left_line);
    SENSOR_LOGV("pd area width = %d; pd area height = %d", line_feed, line_height);

    for(i = 0; i < line_height; i++) {
        pointer_for_src = (cmr_u8 *)(temp_buff + i * line_feed * 4);
        pointer_for_left = (cmr_u8 *)(pdaf_buffer->left_output + left_line * line_feed * 4);
        pointer_for_right = (cmr_u8 *)(pdaf_buffer->right_output + right_line * line_feed * 4);
        if(coordinate_tab == (descriptor->block_height))
            coordinate_tab = 0;
        if(*(pd_coordinate_tab + coordinate_tab)) {
            memcpy(pointer_for_right, pointer_for_src, line_feed * 4);
            right_line++;
        } else {
            memcpy(pointer_for_left, pointer_for_src, line_feed * 4);
            left_line++;
        }
        coordinate_tab++;
    }
    SENSOR_LOGV("output line of left is %d, right is %d, coordinate_tab is %d",
                 left_line, right_line, coordinate_tab);

    return SENSOR_SUCCESS;
}


cmr_int sensor_pdaf_buffer_seprator(struct pdaf_buffer_handle *pdaf_buffer,
                                    struct sensor_pdaf_info *pdaf_info,
                                    struct pdaf_roi_param *roi_param,
                                    struct pdaf_block_descriptor *descriptor) {
    int ret = SENSOR_SUCCESS;
    switch(descriptor->block_pattern)
    {
    case LINED_UP:
        ret = sensor_pdaf_buffer_lined_seprator(descriptor, pdaf_buffer, roi_param);
        break;
    case CROSS_PATTERN:
        ret = sensor_pdaf_buffer_block_seprator(descriptor, pdaf_buffer, roi_param);
        break;
    default:
        break;
    }
    return ret;
}
cmr_int sensor_pdaf_buffer_process_for_imx258(struct pdaf_buffer_handle *buffer_handle,
                                              struct sensor_pdaf_info *pdaf_info) {
    if(!buffer_handle || !pdaf_info) {
        SENSOR_LOGE("NOT vid input");
        return -1;
    }
    struct pdaf_block_descriptor *descriptor = (pdaf_info->descriptor);
    cmr_int ret = SENSOR_SUCCESS;
    cmr_int pd_buffer_line = PD_LINE_WIDTH_FOR_IMX258;
    cmr_int pd_colum_height = pdaf_info->type2_info.height;
    cmr_int pd_line_width = pdaf_info->pd_block_num_x * descriptor->block_width;
    cmr_int pd_buffer_size = pd_buffer_line * pd_colum_height;
    cmr_int pd_dummy = PD_BUFFER_OFFSET_FOR_IMX258;
    cmr_int pd_offset = pd_dummy * 5;
    cmr_int pd_line_offset = pd_buffer_line / 2;
    cmr_int index = 0;
    char value[128];
    cmr_u16 *input_pointer, *left_output_pointer, *right_output_pointer;
    input_pointer = buffer_handle->left_buffer;
    left_output_pointer = buffer_handle->left_output;
    right_output_pointer = buffer_handle->right_output;
    if(!input_pointer || !left_output_pointer || !right_output_pointer) {
        SENSOR_LOGD("input parameters are not correct");
        return -1;
    }
    property_get("persist.vendor.cam.dump.imx258.pdaf.input", value, "0");
    if(atoi(value))
        sensor_pdaf_dump_func(input_pointer, pd_buffer_size, "imx258_input.raw");
    for(int i = 0; i < pd_buffer_size; i += pd_buffer_line) {
        for(int j = 0; j < pd_line_width; j++) {
            *(left_output_pointer + index + j) = *(input_pointer + i + pd_dummy + j);
            *(right_output_pointer + index + j) = *(input_pointer + i + pd_offset + j);

        }
        index += pd_line_width;
        for(int j = 0; j < pd_line_width; j++) {
            *(right_output_pointer + index + j) = *(input_pointer + i + pd_line_offset + pd_dummy + j);
            *(left_output_pointer + index + j) = *(input_pointer + i + pd_line_offset + pd_offset + j);
        }
        index += pd_line_width;
    }
    SENSOR_LOGV("output index is %d", index);
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_prepare_roi(void *input_buffer, void *output_buffer, struct pdaf_roi_param *roi_param) {
    cmr_int roi_start_x, roi_start_y, roi_area_width, roi_area_height, roi_start_address;
    cmr_int actual_roi_start_address, actual_copy_size;
    cmr_int pd_area_width, pd_area_height, pd_area_size;
    unsigned char *buffer_input = (unsigned char *)input_buffer;
    unsigned char *buffer_output = (unsigned char *)output_buffer;
    roi_start_x = roi_param->roi_start_x;
    roi_start_y = roi_param->roi_start_y;
    roi_area_width = roi_param->roi_width;
    roi_area_height = roi_param->roi_height;
    pd_area_width = roi_param->pd_area_width;
    pd_area_height = roi_param->pd_area_height;
    pd_area_size = pd_area_height * pd_area_width;
    roi_param->pd_area_sum = 0;

    SENSOR_LOGV("PDAF INFO with: roi_start_address [%d, %d], roi_area_size [%d, %d]", roi_start_x, roi_start_y, roi_area_width, roi_area_height);
    for(int i = 0; i < roi_area_height; i++) {
        roi_start_address = pd_area_width * (i + roi_start_y) + roi_start_x;
        roi_param->line_offset_x[i] = roi_start_address % 4;
        roi_param->line_offset_y[i] = (roi_param->roi_width + roi_param->line_offset_x[i]) % 4;
        actual_roi_start_address = (roi_start_address - roi_param->line_offset_x[i]) * 5 / 4;
        actual_copy_size = (roi_param->roi_width + roi_param->line_offset_x[i] + roi_param->line_offset_y[i]) * 5 / 4;
        buffer_input = (unsigned char *)input_buffer + actual_roi_start_address;
        buffer_output += actual_copy_size;
        roi_param->pd_area_sum += actual_copy_size;
        memcpy(buffer_output, buffer_input, actual_copy_size);
    }
    SENSOR_LOGV("Actual copy size is %d with each line %d", roi_param->pd_area_sum, actual_copy_size);
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_buffer_alignment(void *input_buffer, void *output_buffer, struct pdaf_roi_param *roi_param) {
    unsigned char *buffer_input, *buffer_output;
    cmr_int offset_start_x, pd_roi_width, pd_roi_height;
    buffer_input = (unsigned char *)input_buffer;
    buffer_output = (unsigned char *)output_buffer;
    pd_roi_width = roi_param->roi_width;
    pd_roi_height = roi_param->roi_height;
    for(int i = 0; i < pd_roi_height; i++) {
        offset_start_x = roi_param->line_offset_x[i] + roi_param->line_offset_y[SENSOR_PDAF_ABS_value(i - 1)] + i * pd_roi_width;
        memcpy(buffer_output + 4 * i * pd_roi_width, input_buffer + offset_start_x * 4, pd_roi_width * 4);
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_pdaf_prepare_buffer(struct pdaf_buffer_handle *buffer_handle, struct pdaf_roi_param *roi_param) {
    if(!buffer_handle || !roi_param) {
        SENSOR_LOGE("NOT VALID INPUT");
        return -1;
    }
    cmr_int pd_area_size = roi_param->pd_area_height * roi_param->pd_area_width * 5 / 2;
    cmr_int pd_roi_size = roi_param->roi_height * roi_param->roi_width * 4;
    memset(buffer_handle->right_buffer, 0, pd_area_size);
    memset(buffer_handle->left_output, 0, pd_roi_size);
    memset(buffer_handle->right_output, 0, pd_roi_size);
    return 0;
}

cmr_int sensor_pdaf_mipi_raw_type2(void *buffer_handle, cmr_u32 *param) {
    struct sensor_pdaf_info *pdaf_info = (struct sensor_pdaf_info *)param;
    struct pdaf_buffer_handle *pdaf_buffer = (struct pdaf_buffer_handle *)buffer_handle;
    struct pdaf_roi_param roi_param;
    struct pdaf_block_descriptor *descriptor = (pdaf_info->descriptor);
    cmr_u32 pdaf_area_size;
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u8 value[128];

    property_get("persist.vendor.cam.pdaf.dump.type2", value, "0");
    pdaf_area_size = pdaf_info->pd_block_num_x * pdaf_info->pd_block_num_y *
                     pdaf_info->pd_pos_size * 2;

    if(atoi(value))
        sensor_pdaf_dump_func_with_frameid(pdaf_buffer->left_buffer, pdaf_area_size * 5 / 4,
                                           "type2_input_mipi.mipi_raw", pdaf_buffer->frameid);
    switch(descriptor->is_special_format)
    {
    case CONVERTOR_FOR_IMX258:
        sensor_pdaf_buffer_process_for_imx258(pdaf_buffer, pdaf_info);
        break;
    default:
        sensor_pdaf_calculate_roi(pdaf_info, pdaf_buffer, &roi_param);
	    sensor_pdaf_prepare_roi(pdaf_buffer->left_buffer, pdaf_buffer->right_buffer, &roi_param);
        sensor_pdaf_mipi_data_processor(pdaf_buffer->right_buffer,
                                        pdaf_buffer->left_buffer, roi_param.pd_area_sum);
        sensor_pdaf_buffer_alignment(pdaf_buffer->left_buffer, pdaf_buffer->right_buffer, &roi_param);
        sensor_pdaf_buffer_seprator(pdaf_buffer, pdaf_info, &roi_param, descriptor);
        break;
    }
    if(atoi(value)) {
       sensor_pdaf_dump_func_with_frameid(pdaf_buffer->left_output, roi_param.roi_width * roi_param.roi_height * 2,
                                          "type2_left_output.raw", pdaf_buffer->frameid);
       sensor_pdaf_dump_func_with_frameid(pdaf_buffer->right_output, roi_param.roi_width * roi_param.roi_height * 2,
                                          "tyep2_right_ouptut.raw", pdaf_buffer->frameid);
    }
    return ret;
}

cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param) {
    if(!buffer_handle|| !param)
        return SENSOR_FAIL;
    int ret = SENSOR_SUCCESS;
    double start = tick();
    switch (pdaf_supported)
    {
    case PDAF_TYPE3_BUFFER:
        ret = sensor_pdaf_mipi_raw_type3(buffer_handle, param);
        break;
    case PDAF_TYPE2_BUFFER:
        ret = sensor_pdaf_mipi_raw_type2(buffer_handle, param);
    default:
        break;
    }
    SENSOR_LOGD("FINISHED with Time Cost %f", tick() - start);
    return ret;
}


