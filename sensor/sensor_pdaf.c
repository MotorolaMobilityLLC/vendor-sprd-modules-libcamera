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
            SENSOR_LOGD("invalid buffer");      \
    } while(0)

#define SENSOR_PDAF_CHECK_POINTER(a)            \
    do {                                        \
        if(!a) {                                \
            SENSOR_LOGD("invalid param");       \
            return SENSOR_FAIL;                 \
        }                                       \
    } while(0)

struct pdaf_roi_param {
    cmr_int roi_start_x;
    cmr_int roi_start_y;
    cmr_int roi_width;
    cmr_int roi_height;
    cmr_int pd_area_width;
    cmr_int pd_area_height;
};

enum PDAF_DATA_PROCESS {
    PDAF_INVALID_BUFFER,
    PDAF_TYPE2_BUFFER = 2,
    PDAF_TYPE3_BUFFER = 3
};

cmr_int sensor_pdaf_dump_func(void *buffer, cmr_int dump_numb, unsigned char *name);
cmr_int sensor_pdaf_mipi_data_processor(cmr_u8 *input_buf, cmr_u16 *output_buf, cmr_u32 size);
cmr_int sensor_pdaf_data_extraction(cmr_u16 *src, cmr_u16 *dst, struct pdaf_roi_param *roi_param);
cmr_int sensor_pdaf_type3_convertor(struct pdaf_buffer_handle *pdaf_buffer,
                                    struct pdaf_roi_param *roi_param);
cmr_int sensor_pdaf_mipi_raw_type3(void *buffer_handle, cmr_u32 *param);
cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param);
cmr_int sensor_pdaf_mipi_raw_type2(void *buffer_handle, cmr_u32 *param);
cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param);

cmr_int sensor_pdaf_mipi_data_processor(cmr_u8 *input_buf, cmr_u16 *output_buf, cmr_u32 size) {
    cmr_u16 convertor[5];
    int count = 0;
    int judge = 0;
    cmr_u16 *pointer_for_output = output_buf;
    char value[128];
    for(int i = 0; i < size; i++) {
        convertor[count++] = input_buf[i];
        if((count % 5) == 0) {
            count = 0;
            convertor[0] = ((convertor[0] << 2) | (convertor[4] & 0x03));
            convertor[1] = ((convertor[1] << 2) | (convertor[4] & 0x0C) >> 2);
            convertor[2] = ((convertor[2] << 2) | (convertor[4] & 0x30) >> 4);
            convertor[3] = ((convertor[3] << 2) | (convertor[4] & 0xC0) >> 6);
            memcpy(pointer_for_output + 4 * judge, convertor, 4 * sizeof(cmr_u16));
            judge++;
        }
    }
    property_get("persist.vendor.cam.sensor.pdaf.mipi.dump", value, "0");
    if(atoi(value)) {
        sensor_pdaf_dump_func(input_buf, size, "sensor_pdaf_input.mipi_raw");
        sensor_pdaf_dump_func(output_buf, size / 5 * 8, "sensor_pdaf_raw16.raw");
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

void sensor_pdaf_calculate_roi(struct sensor_pdaf_info *pdaf_info,
                                struct pdaf_roi_param *roi_param) {
    cmr_int pdaf_area_width, pdaf_area_height;
    cmr_int base_width, base_height, pd_area_start_x, pd_area_start_y;
    cmr_int pd_block_w, pd_block_h, offset_width, offset_height;
    cmr_int roi_img_start_x, roi_img_start_y, roi_width, roi_height;
    cmr_int block_number_x, block_number_y;
    pd_block_h = 8 * pdaf_info->pd_block_w;
    pd_block_w = 8 * pdaf_info->pd_block_h;
    pdaf_area_height = (pdaf_info->pd_end_y - pdaf_info->pd_offset_y);
    pdaf_area_width = (pdaf_info->pd_end_x - pdaf_info->pd_offset_x);
    pd_area_start_x = pdaf_info->pd_offset_x;
    pd_area_start_y = pdaf_info->pd_offset_y;
    base_width = DEFAULT_ROI_WIDTH;
    base_height = DEFAULT_ROI_HEIGHT;

    if(pdaf_area_width < 8 * base_width || pdaf_area_height < 8 * base_height) {
        base_width = (pdaf_area_width / pd_block_w / 8) * pd_block_w;
        base_height = (pdaf_area_height / pd_block_h / 8) * pd_block_h;
        roi_width = 4 * base_width;
        roi_height = 4 * base_height;
    }

    roi_img_start_x = pdaf_info->pd_offset_x + 2 * base_width;
    roi_img_start_y = pdaf_info->pd_offset_y + 2 * base_height;
    offset_width = pdaf_area_width - 8 * base_width;
    offset_height = pdaf_area_height - 8 * base_height;

#ifdef PDAF_CALC_ROI
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
    roi_param->roi_height = pdaf_area_height / pd_block_h;
    roi_param->roi_width = pdaf_area_width / pd_block_w;
    roi_param->roi_start_x = (roi_img_start_x - pdaf_info->pd_offset_x) / pdaf_info->pd_density_x;
    roi_param->roi_start_y = (roi_img_start_y - pdaf_info->pd_offset_y) / pdaf_info->pd_density_y;
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
    sprintf(file_path, "%s%s", PDAF_BUFFER_DUMP_PATH, name);
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

    sensor_pdaf_calculate_roi(pdaf_info, &roi_param);
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
    cmr_int block_height, line_coordinate, right_line, left_line, line_width, line_height;
    cmr_int left_flag, right_flag, output_line_width;
    cmr_u16 *left = (cmr_u16 *)pdaf_buffer->left_output;
    cmr_u16 *right = (cmr_u16 *)pdaf_buffer->right_output;
    cmr_u16 *input;
    int output_line;

    SENSOR_PDAF_INIT_INT(output_line, left_line, right_line);
    block_height = descriptor->block_height;
    line_width = roi_param->pd_area_width;
    line_height = roi_param->pd_area_height;
    input = pdaf_buffer->right_buffer;
    output_line_width = line_width / 2;

    SENSOR_LOGV("line width -> %d, line height -> %d, output_line_width -> %d",
                 line_width, line_height, output_line_width);
    SENSOR_LOGV("block height = %d", descriptor->block_height);
    for(int i = 0; i < line_height; i++) {
        SENSOR_PDAF_INIT_INT(left_flag, right_flag, line_coordinate);
        for(int j = 0; j < line_width; j++) {
            if(*(judger->pos_info + line_coordinate)) {
                memcpy((right + (right_line * output_line_width + right_flag)),
                       (input + (i * line_width + j)), sizeof(cmr_u16));
                right_flag++;
            } else {
                memcpy((left + (left_line * output_line_width + left_flag)),
                       (input + (i * line_width + j)), sizeof(cmr_u16));
                left_flag++;
            }
            line_coordinate++;
            if(line_coordinate == (judger->number))
                line_coordinate = 0;
        }
        if(!((i + 1) % block_height)) {
            output_line++;
            judger = descriptor->pd_line_coordinate;
        } else
            judger = judger + 1;
        if(right_flag)
            right_line++;
        if(left_flag)
            left_line++;
    }

    SENSOR_LOGD("block seprator output line is %d with left_line -> %d, right_line -> %d",
                 output_line, left_line, right_line);
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
    line_feed = roi_param->pd_area_width;
    line_height = roi_param->pd_area_height;
    pointer_for_right = (cmr_u8 *)pdaf_buffer->right_output;
    pointer_for_left = (cmr_u8 *)pdaf_buffer->left_output;

    SENSOR_PDAF_INIT_INT(coordinate_tab, right_line, left_line);
    SENSOR_LOGV("pd area width = %d; pd area height = %d", line_feed, line_height);

    for(i = 0; i < line_height; i++) {
        pointer_for_src = (cmr_u8 *)(temp_buff + i * line_feed * 2);
        pointer_for_left = (cmr_u8 *)(pdaf_buffer->left_output + left_line * line_feed * 2);
        pointer_for_right = (cmr_u8 *)(pdaf_buffer->right_output + right_line * line_feed * 2);
        if(coordinate_tab == (descriptor->block_height))
            coordinate_tab = 0;
        if(*(pd_coordinate_tab + coordinate_tab)) {
            memcpy(pointer_for_right, pointer_for_src, line_feed * 2);
            right_line++;
        } else {
            memcpy(pointer_for_left, pointer_for_src, line_feed * 2);
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
        SENSOR_LOGE("NOT valid input");
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

    switch(descriptor->is_special_format)
    {
    case CONVERTOR_FOR_IMX258:
        sensor_pdaf_buffer_process_for_imx258(pdaf_buffer, pdaf_info);
        break;
    default:
        sensor_pdaf_mipi_data_processor(pdaf_buffer->left_buffer,
                                        pdaf_buffer->right_buffer, pdaf_area_size * 5 / 4);
        sensor_pdaf_calculate_roi(pdaf_info, &roi_param);
        sensor_pdaf_buffer_seprator(pdaf_buffer, pdaf_info, &roi_param, descriptor);
        break;
    }
    if(atoi(value)) {
       sensor_pdaf_dump_func(pdaf_buffer->left_output, pdaf_area_size, "type2_left_output.raw");
       sensor_pdaf_dump_func(pdaf_buffer->right_output, pdaf_area_size, "tyep2_right_ouptut.raw");
    }
    SENSOR_LOGV("left_input -> %p, left_output %p, right_input %p, right_output %p <-<- out",
                 pdaf_buffer->left_buffer, pdaf_buffer->left_output,
                 pdaf_buffer->right_buffer, pdaf_buffer->right_output);

    SENSOR_LOGD("pdaf running finished with %ld", ret);
    return ret;
}

cmr_int sensor_pdaf_format_convertor(void *buffer_handle,
                                     cmr_int pdaf_supported, cmr_u32 *param) {
    if(!buffer_handle|| !param)
        return SENSOR_FAIL;
    int ret = SENSOR_SUCCESS;

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
    return ret;
}
