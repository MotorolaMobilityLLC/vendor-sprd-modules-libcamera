#include "slice_drv.h"
#include <memory.h>
#include "core_yuvscaler_drv.h"

static uint8 YUV_DECI_MAP[]={2,4,8,16};

typedef struct _tagSliceWnd
{
    int s_row;
    int e_row;
    int s_col;
    int e_col;
    int overlap_left;
    int overlap_right;
    int overlap_up;
    int overlap_down;

} SliceWnd;

typedef struct _tag_pipe_overlap_context
{
    int frameWidth;
    int frameHeight;
    int pixelFormat; //0: yuv422 1: yuv420
}pipe_overlap_context;

typedef struct _tag_pipe_region_info
{
    int sx;
    int ex;
    int sy;
    int ey;
}pipe_region_info;

typedef struct _tag_pipe_scaler_phase_info
{    
    int init_phase_hor;
    int init_phase_ver;
} pipe_scaler_phase_info;

typedef struct _tag_pipe_overlap_scaler_param
{
    //in
    int trim_eb;
    int trim_start_x;
    int trim_start_y;
    int trim_size_x;
    int trim_size_y;
    int deci_x_eb;
    int deci_y_eb;
    int deci_x;
    int deci_y;
    int scaler_en;
    int des_size_x;
    int des_size_y;
    int yuv_output_format;
    int scaler_init_phase_hor;
    //run-time
    int ref_ox;
    int ref_oy;

    //out
    yuvscaler_param_t frame_param;
    yuvscaler_param_t slice_param;
    pipe_scaler_phase_info phase[CPP_MAX_SLICE_NUM];
    pipe_region_info region_input[CPP_MAX_SLICE_NUM];
    pipe_region_info region_output[CPP_MAX_SLICE_NUM];

}pipe_overlap_scaler_param;


static void scaler_frame_init(
    pipe_overlap_scaler_param *in_param_ptr, 
    pipe_overlap_context *context)
{
    yuvscaler_param_t *core_param = &in_param_ptr->frame_param;
    //
    core_param->bypass = 0;
    //
    core_param->trim0_info.trim_en         = in_param_ptr->trim_eb;
    core_param->trim0_info.trim_start_x    = in_param_ptr->trim_start_x;
    core_param->trim0_info.trim_start_y    = in_param_ptr->trim_start_y;
    core_param->trim0_info.trim_size_x     = in_param_ptr->trim_size_x;
    core_param->trim0_info.trim_size_y     = in_param_ptr->trim_size_y;
    if(0 == core_param->trim0_info.trim_en)
    {
        core_param->trim0_info.trim_start_x = 0;
        core_param->trim0_info.trim_start_y = 0;
        core_param->trim0_info.trim_size_x  = context->frameWidth;
        core_param->trim0_info.trim_size_y  = context->frameHeight;
    }
    //
    core_param->deci_info.deci_x_en = in_param_ptr->deci_x_eb;
    core_param->deci_info.deci_y_en = in_param_ptr->deci_y_eb;
    core_param->deci_info.deci_x    = YUV_DECI_MAP[in_param_ptr->deci_x];
    core_param->deci_info.deci_y    = YUV_DECI_MAP[in_param_ptr->deci_y];
    //
    core_param->scaler_info.scaler_en         = in_param_ptr->scaler_en;
    core_param->scaler_info.scaler_out_width  = in_param_ptr->des_size_x;
    core_param->scaler_info.scaler_out_height = in_param_ptr->des_size_y;
    //
    core_param->output_pixfmt = in_param_ptr->yuv_output_format;
    if(core_param->output_pixfmt == YUV422)
    {
        core_param->output_align_hor = 2;
        core_param->output_align_ver = 2;
    }
    else if(core_param->output_pixfmt == YUV420)
    {
        core_param->output_align_hor = 2;
        core_param->output_align_ver = 4;
    }

    core_param->input_pixfmt = context->pixelFormat;

    core_param->src_size_x = context->frameWidth;
    core_param->src_size_y = context->frameHeight;
    core_param->init_phase_hor = in_param_ptr->scaler_init_phase_hor;
    yuv_scaler_init_frame_info(core_param);
    core_param->scaler_info.input_pixfmt  = core_param->input_pixfmt;
    core_param->scaler_info.output_pixfmt = core_param->output_pixfmt;
}

static void scaler_calculate_region(
    pipe_region_info ref_slice_region_arr[],
    int rows,
    int cols,
    int fullsize_x,
    int fullsize_y,
    pipe_overlap_scaler_param *scaler_param_ptr,
    int v_flag)
{
    int i,j,index;
    uint16 prev_row_end, prev_col_end;
    int output_slice_end;
    pipe_region_info wndInTemp;
    pipe_region_info wndOutTemp;
    pipe_scaler_phase_info phaseTemp;
    pipe_region_info *wndOrgPtr;
    yuvscaler_param_t *core_param = &scaler_param_ptr->frame_param;

    phaseTemp.init_phase_hor = 0;
    phaseTemp.init_phase_ver = 0;

    prev_row_end = 0;
    for ( i = 0; i < rows; i++)
    {
        prev_col_end = 0;
        for ( j = 0;j < cols; j++)
        {

            index = i*cols + j;
            wndOrgPtr = &ref_slice_region_arr[index];

            //hor
            do
            {
                int trim_en            = core_param->trim0_info.trim_en;
                int trim_start         = core_param->trim0_info.trim_start_x;
                int trim_size          = core_param->trim0_info.trim_size_x;
                int deci               = core_param->deci_info.deci_x;
                //int scl_en             = core_param->scaler_info.scaler_en;
                int scl_factor_in      = core_param->scaler_info.scaler_factor_in_hor;
                int scl_factor_out     = core_param->scaler_info.scaler_factor_out_hor;
                int scl_tap            = core_param->scaler_info.scaler_y_hor_tap;
                int init_phase         = core_param->scaler_info.init_phase_info.scaler_init_phase[0];
                int input_slice_start  = wndOrgPtr->sx;//wndOrg.s_col;
                int input_slice_size   = wndOrgPtr->ex - wndOrgPtr->sx + 1;//wndOrg.e_col - wndOrg.s_col + 1;
                int output_pixel_align = core_param->output_align_hor;
                int output_slice_start;
                int output_slice_size;

                if(j == cols - 1)
                {
                    output_pixel_align = 2;
                }

                if(v_flag == 0)
                {
                    est_scaler_output_slice_info(
                        trim_en,
                        trim_start,
                        trim_size,
                        deci,
                        scl_factor_in,
                        scl_factor_out,
                        scl_tap,
                        init_phase,
                        input_slice_start,
                        input_slice_size,
                        output_pixel_align,
                        &output_slice_end);
                }
                else
                {
                    output_slice_end = fullsize_x;
                    est_scaler_output_slice_info_v2(
                        trim_en,
                        trim_start,
                        trim_size,
                        deci,
                        scl_factor_in,
                        scl_factor_out,
                        scl_tap,
                        init_phase,
                        input_slice_start,
                        input_slice_size,
                        output_pixel_align,
                        &output_slice_end);
                }

                output_slice_start = prev_col_end;
                output_slice_size = output_slice_end - output_slice_start;

                wndOutTemp.sx = output_slice_start;
                wndOutTemp.ex = output_slice_end - 1;

                if(output_slice_size > 0)
                {
                    int input_pixel_align = 2;
                    input_slice_size = fullsize_x;

                    calc_scaler_input_slice_info(trim_en,trim_start, trim_size, deci, scl_factor_in, scl_factor_out, scl_tap, init_phase,
                        output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

                    //this->slicePhaseInfoList.at(index).init_phase_hor = init_phase;
                    phaseTemp.init_phase_hor = init_phase;

                    wndInTemp.sx = input_slice_start;
                    wndInTemp.ex = wndInTemp.sx + input_slice_size - 1;
                }
                else
                {
                    wndInTemp.sx = wndOrgPtr->sx;//wndOrg.s_col;
                    wndInTemp.ex = wndOrgPtr->ex;//.e_col;
                }

                prev_col_end = output_slice_end;
            }while(0);

            //ver
            {               
                int scl_tap = 0;
                int trim_en            = core_param->trim0_info.trim_en;
                int trim_start         = core_param->trim0_info.trim_start_y;
                int trim_size          = core_param->trim0_info.trim_size_y;
                int deci               = core_param->deci_info.deci_y;
                int scl_en             = core_param->scaler_info.scaler_en;
                int scl_factor_in      = core_param->scaler_info.scaler_factor_in_ver;
                int scl_factor_out     = core_param->scaler_info.scaler_factor_out_ver;

                int init_phase         = core_param->scaler_info.init_phase_info.scaler_init_phase[1];
                int input_slice_start  = wndOrgPtr->sy;//wndOrg.s_row;
                int input_slice_size   = wndOrgPtr->ey - wndOrgPtr->sy + 1;//wndOrg.e_row - wndOrg.s_row + 1;
                int output_pixel_align = core_param->output_align_ver;
                int output_slice_start;
                int output_slice_size;

                if(scl_en)
                {
                    if(core_param->input_pixfmt == YUV422)
                        scl_tap = MAX(core_param->scaler_info.scaler_y_ver_tap, core_param->scaler_info.scaler_uv_ver_tap)   + 2;
                    else if(core_param->input_pixfmt == YUV420)
                        scl_tap = MAX(core_param->scaler_info.scaler_y_ver_tap, core_param->scaler_info.scaler_uv_ver_tap*2) + 2;
                }

                if(core_param->output_pixfmt == YUV420 && i == rows - 1)
                {
                    output_pixel_align = 4;
                }

                if ((i == rows - 1) && (core_param->output_pixfmt == YUV420))
                {
                    output_pixel_align = 4;
                }

                if(v_flag == 0)
                {
                    est_scaler_output_slice_info(
                        trim_en,
                        trim_start,
                        trim_size,
                        deci,
                        scl_factor_in,
                        scl_factor_out,
                        scl_tap,
                        init_phase,
                        input_slice_start,
                        input_slice_size,
                        output_pixel_align,
                        &output_slice_end);
                }
                else
                {
                    output_slice_end = fullsize_y;
                    est_scaler_output_slice_info_v2(
                        trim_en,
                        trim_start,
                        trim_size,
                        deci,
                        scl_factor_in,
                        scl_factor_out,
                        scl_tap,
                        init_phase,
                        input_slice_start,
                        input_slice_size,
                        output_pixel_align,
                        &output_slice_end);
                }

                output_slice_start = prev_row_end;
                output_slice_size  = output_slice_end - output_slice_start;

                wndOutTemp.sy = output_slice_start;
                wndOutTemp.ey = output_slice_end - 1;

                if(output_slice_size > 0)
                {
                    int input_pixel_align = 2;
                    input_slice_size = fullsize_y;

                    calc_scaler_input_slice_info(trim_en,trim_start, trim_size, deci, scl_factor_in, scl_factor_out, scl_tap, init_phase,
                        output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

                    //this->slicePhaseInfoList.at(index).init_phase_ver = init_phase;
                    phaseTemp.init_phase_ver = init_phase;

                    wndInTemp.sy = input_slice_start;
                    wndInTemp.ey = wndInTemp.sy + input_slice_size - 1;
                }
                else
                {
                    wndInTemp.sy = wndOrgPtr->sy;//wndOrg.s_row;
                    wndInTemp.ey = wndOrgPtr->ey;//wndOrg.e_row;
                }
            }

            //this->inputSliceList.slices.push_back(wndInTemp);
            //this->outputSliceList.slices.push_back(wndOutTemp);
            scaler_param_ptr->phase[index].init_phase_hor = phaseTemp.init_phase_hor;
            scaler_param_ptr->phase[index].init_phase_ver = phaseTemp.init_phase_ver;

            scaler_param_ptr->region_input[index].sx = wndInTemp.sx;
            scaler_param_ptr->region_input[index].sy = wndInTemp.sy;
            scaler_param_ptr->region_input[index].ex = wndInTemp.ex;
            scaler_param_ptr->region_input[index].ey = wndInTemp.ey;

            scaler_param_ptr->region_output[index].sx = wndOutTemp.sx;
            scaler_param_ptr->region_output[index].sy = wndOutTemp.sy;
            scaler_param_ptr->region_output[index].ex = wndOutTemp.ex;
            scaler_param_ptr->region_output[index].ey = wndOutTemp.ey;

        }
        prev_row_end = wndOutTemp.ey + 1;
    }
}

typedef struct _tag_fw_slice_init_context
{
    int slice_index;
    int rows;
    int cols;
    int slice_row_no;
    int slice_col_no;
    int slice_w;
    int slice_h;

} fw_slice_init_context;

static void scaler_slice_init(
    const pipe_region_info *input_slice_region,
    fw_slice_init_context *context,
    pipe_overlap_scaler_param *scaler_param_ptr)
{
    scaler_slice_t scaler_slice;
    scaler_slice_t input_slice_info;
    scaler_slice_t output_slice_info;
    yuvscaler_param_t *frame_param = &scaler_param_ptr->frame_param;
    yuvscaler_param_t *slice_param = &scaler_param_ptr->slice_param;
    int slice_index = context->slice_index;
    int rows = context->rows;
    int cols = context->cols;
    int slice_row_no = context->slice_row_no;
    int slice_col_no = context->slice_col_no;
    //scaler_overlap_t scaler_overlap;

    //scaler_slice.slice_id       = image->context.sliceID;
    scaler_slice.slice_id = slice_index;
    //scaler_slice.start_col      = image->context.sliceStartCol;
    scaler_slice.start_col = input_slice_region->sx;
    //scaler_slice.start_row      = image->context.sliceStartRow;
    scaler_slice.start_row = input_slice_region->sy;
    //scaler_slice.end_col        = image->context.sliceEndCol;
    scaler_slice.end_col = input_slice_region->ex;
    //scaler_slice.end_row        = image->context.sliceEndRow;
    scaler_slice.end_row = input_slice_region->ey;
    //scaler_slice.sliceRows      = image->context.sliceRows;
    scaler_slice.sliceRows = rows;
    //scaler_slice.sliceCols      = image->context.sliceCols;
    scaler_slice.sliceCols = cols;
    //scaler_slice.sliceRowNo     = image->context.sliceRowNo;
    scaler_slice.sliceRowNo = slice_row_no;
    //scaler_slice.sliceColNo     = image->context.sliceColNo;
    scaler_slice.sliceColNo = slice_col_no;
    //scaler_slice.slice_width    = image->width;
    scaler_slice.slice_width = context->slice_w;
    //scaler_slice.slice_height   = image->height;]
    scaler_slice.slice_height = context->slice_h;
    //scaler_slice.overlap_left   = image->context.sliceOverlapLeft;
    scaler_slice.overlap_left = 0;
    //scaler_slice.overlap_right  = image->context.sliceOverlapRight;
    scaler_slice.overlap_right = 0;
    //scaler_slice.overlap_up     = image->context.sliceOverlapUp;
    scaler_slice.overlap_up = 0;
    //scaler_slice.overlap_down   = image->context.sliceOverlapDown;
    scaler_slice.overlap_down = 0;

    //scaler_overlap.overlap_left = this->overlap.overlap_left;
    //scaler_overlap.overlap_right= this->overlap.overlap_right;
    //scaler_overlap.overlap_up   = this->overlap.overlap_up;
    //scaler_overlap.overlap_down = this->overlap.overlap_down;

    //int sliceID = image->context.sliceID;
    //const SliceWnd  &wndInputOrg  = this->inputSliceList.slices.at(sliceID);
    //const SliceWnd  &wndOutputOrg = this->outputSliceList.slices.at(sliceID);
    //const PhaseInfo &phaseInfo    = this->slicePhaseInfoList.at(sliceID);

    //scaler_slice.init_phase_hor = phaseInfo.init_phase_hor;
    //scaler_slice.init_phase_ver = phaseInfo.init_phase_ver;
    scaler_slice.init_phase_hor = scaler_param_ptr->phase[slice_index].init_phase_hor;
    scaler_slice.init_phase_ver = scaler_param_ptr->phase[slice_index].init_phase_ver;

    if (slice_col_no == 0)
    {
        //this->ref_ox = 0;
        scaler_param_ptr->ref_ox = 0;
    }
    if (/*image->context.sliceRowNo*/ slice_row_no == 0)
    {
        //this->ref_oy = 0;
        scaler_param_ptr->ref_oy = 0;
    }

    //yuv_scaler_init_slice_info(&this->core_param, &this->scaler_param, &scaler_slice, &scaler_overlap, this->ref_ox, this->ref_oy);


    //     input_slice_info.start_col = wndInputOrg.s_col;
    //     input_slice_info.end_col   = wndInputOrg.e_col;
    //     input_slice_info.start_row = wndInputOrg.s_row;
    //     input_slice_info.end_row   = wndInputOrg.e_row;
    input_slice_info.start_col = scaler_param_ptr->region_input[slice_index].sx;
    input_slice_info.end_col   = scaler_param_ptr->region_input[slice_index].ex;
    input_slice_info.start_row = scaler_param_ptr->region_input[slice_index].sy;
    input_slice_info.end_row   = scaler_param_ptr->region_input[slice_index].ey;
    //     {
    //         image->context.sliceOverlapLeft  = (input_slice_info.start_col - image->context.sliceStartCol)/this->core_param.deci_info.deci_x;
    //         image->context.sliceOverlapRight = (image->context.sliceEndCol - input_slice_info.end_col)/this->core_param.deci_info.deci_x;
    //         image->context.sliceOverlapUp    = (input_slice_info.start_row - image->context.sliceStartRow)/this->core_param.deci_info.deci_y;
    //         image->context.sliceOverlapDown = (image->context.sliceEndRow - input_slice_info.end_row)/this->core_param.deci_info.deci_y;
    //     }

    //     output_slice_info.start_col = wndOutputOrg.s_col;
    //     output_slice_info.end_col   = wndOutputOrg.e_col;
    //     output_slice_info.start_row = wndOutputOrg.s_row;
    //     output_slice_info.end_row   = wndOutputOrg.e_row;
    output_slice_info.start_col = scaler_param_ptr->region_output[slice_index].sx;
    output_slice_info.end_col   = scaler_param_ptr->region_output[slice_index].ex;
    output_slice_info.start_row = scaler_param_ptr->region_output[slice_index].sy;
    output_slice_info.end_row   = scaler_param_ptr->region_output[slice_index].ey;

    yuv_scaler_init_slice_info_v3(
        //&this->core_param, 
        //&this->scaler_param,
        frame_param,
        slice_param,
        &scaler_slice,
        &input_slice_info,
        &output_slice_info);
}

static void fetch_slice(
    int img_w,
    int img_h,
    int slice_w,
    int slice_h,
    pipe_region_info slice_region_arr[],
    int *slice_rows,
    int *slice_cols
    )
{
    int i,j,index;
    int imgW        = img_w;//inParam.image_w;
    int imgH        = img_h;//inParam.image_h;
    int sliceW      = slice_w;//.slice_w;
    int sliceH      = slice_h;//inParam.slice_h;
    int overlapUp   = 0;//inParam.overlap_up;
    int overlapDown = 0;//inParam.overlap_down;
    int overlapLeft = 0;//inParam.overlap_left;
    int overlapRight= 0;//inParam.overlap_right;
    int col_num;
    int row_num;
    int tem_slice_overlap_w;
    int tem_slice_overlap_h;
    SliceWnd context;

    if(sliceW <= 0)
        sliceW = imgW;
    if(sliceH <= 0)
        sliceH = imgH;

    col_num = imgW/sliceW + (imgW%sliceW?1:0);
    row_num = imgH/sliceH + (imgH%sliceH?1:0);

    *slice_rows = row_num;
    *slice_cols = col_num;

    for ( i = 0;i < row_num; i++)
    {
        for ( j = 0; j < col_num; j++)
        {
            index = i*col_num +j;
            context.s_row = i * sliceH;
            context.s_col = j * sliceW;
            context.e_row = context.s_row + sliceH - 1;
            context.e_col = context.s_col + sliceW - 1;

            context.overlap_left  = overlapLeft;
            context.overlap_right = overlapRight;
            context.overlap_up    = overlapUp;
            context.overlap_down  = overlapDown;

            tem_slice_overlap_w = overlapLeft + overlapRight;
            tem_slice_overlap_h = overlapUp   + overlapDown;

            //
            if(tem_slice_overlap_w < imgW && tem_slice_overlap_h < imgH)
            {
                //l-top
                if((0 == i) && (0 == j))
                {
                    context.overlap_left = 0;
                    context.overlap_up = 0;

                    context.s_row = 0;
                    context.s_col = 0;
                }
                //r-top
                if((0 == i) && (col_num-1 == j))
                {
                    context.overlap_right = 0;
                    context.overlap_up = 0;

                    context.s_row = 0;
                    context.e_col = (imgW -1);
                }
                //l-bottom
                if((row_num-1 == i) && (0 == j))
                {
                    context.overlap_left = 0;
                    context.overlap_down = 0;

                    context.s_col= 0;
                    context.e_row= (imgH - 1);
                }
                //r-bottom
                if((row_num-1 == i) && (col_num-1 == j))
                {
                    context.overlap_right= 0;
                    context.overlap_down = 0;

                    context.e_row = (imgH -1);
                    context.e_col = (imgW -1);
                }
                //up
                if((0 == i) && (0<j && j<col_num-1))
                {
                    context.overlap_up = 0;
                    context.s_row = 0;
                }
                //down
                if((row_num-1 == i) && (0<j && j<col_num-1))
                {
                    context.overlap_down = 0;
                    context.e_row = (imgH - 1);
                }
                //left
                if((0 == j) && (0<i && i<row_num-1))
                {
                    context.overlap_left = 0;
                    context.s_col = 0;
                }
                //right
                if((col_num-1 == j) && (0<i && i<row_num-1))
                {
                    context.overlap_right = 0;
                    context.e_col = (imgW - 1);
                }
            }

            //
            context.s_row -= context.overlap_up;
            context.e_row += context.overlap_down;
            context.s_col -= context.overlap_left;
            context.e_col += context.overlap_right;

            if(context.s_row < 0 
                || context.s_col < 0
                || context.e_col >= imgW
                || context.e_row >= imgH)
            {
                //ISPThrow("[Error] slice region [sx:%d,sy:%d,ex:%d,ey:%d] overflow!",context.s_col,context.s_row,context.e_col,context.e_row);
            }

            //check
            if(tem_slice_overlap_w >= imgW)
            {
                context.overlap_left  = 0;
                context.overlap_right = 0;
            }
            if(tem_slice_overlap_h >= imgH)
            {
                context.overlap_up   = 0;
                context.overlap_down = 0;
            }

            //
            //outParam.slices.push_back(context);
            slice_region_arr[index].sx = context.s_col;
            slice_region_arr[index].sy = context.s_row;
            slice_region_arr[index].ex = context.e_col;
            slice_region_arr[index].ey = context.e_row;
        }
    }
}

static void fetch_slice_ref(
    int img_w,
    int img_h,
    //    int slice_w,
    //    int slice_h,
    pipe_region_info slice_region_arr_ref[],
    int slice_rows,
    int slice_cols,
    pipe_region_info slice_region_arr[]
)
{
    int i,j,index;
    int imgW        = img_w;//inParam.image_w;
    int imgH        = img_h;//inParam.image_h;
    //int sliceW      = slice_w;//.slice_w;
    //int sliceH      = slice_h;//inParam.slice_h;
    int overlapUp   = 0;//inParam.overlap_up;
    int overlapDown = 0;//inParam.overlap_down;
    int overlapLeft = 0;//inParam.overlap_left;
    int overlapRight= 0;//inParam.overlap_right;
    int col_num;
    int row_num;
    int tem_slice_overlap_w;
    int tem_slice_overlap_h;
    SliceWnd context;

    //     if(sliceW <= 0)
    //         sliceW = imgW;
    //     if(sliceH <= 0)
    //         sliceH = imgH;

    //     col_num = imgW/sliceW + (imgW%sliceW?1:0);
    //     row_num = imgH/sliceH + (imgH%sliceH?1:0);
    row_num = slice_rows;
    col_num = slice_cols;
    //*slice_rows = row_num;
    //*slice_cols = col_num;

    for ( i = 0;i < row_num; i++)
    {
        for ( j = 0; j < col_num; j++)
        {
            index = i*col_num +j;
            //             context.s_row = i * sliceH;
            //             context.s_col = j * sliceW;
            //             context.e_row = context.s_row + sliceH - 1;
            //             context.e_col = context.s_col + sliceW - 1;
            context.s_row = slice_region_arr_ref[index].sy;
            context.s_col = slice_region_arr_ref[index].sx;
            context.e_row = slice_region_arr_ref[index].ey;
            context.e_col = slice_region_arr_ref[index].ex;

            context.overlap_left  = overlapLeft;
            context.overlap_right = overlapRight;
            context.overlap_up    = overlapUp;
            context.overlap_down  = overlapDown;

            tem_slice_overlap_w = overlapLeft + overlapRight;
            tem_slice_overlap_h = overlapUp   + overlapDown;

            //
            if(0/*tem_slice_overlap_w < imgW && tem_slice_overlap_h < imgH*/)
            {
                //l-top
                if((0 == i) && (0 == j))
                {
                    context.overlap_left = 0;
                    context.overlap_up = 0;

                    context.s_row = 0;
                    context.s_col = 0;
                }
                //r-top
                if((0 == i) && (col_num-1 == j))
                {
                    context.overlap_right = 0;
                    context.overlap_up = 0;

                    context.s_row = 0;
                    context.e_col = (imgW -1);
                }
                //l-bottom
                if((row_num-1 == i) && (0 == j))
                {
                    context.overlap_left = 0;
                    context.overlap_down = 0;

                    context.s_col= 0;
                    context.e_row= (imgH - 1);
                }
                //r-bottom
                if((row_num-1 == i) && (col_num-1 == j))
                {
                    context.overlap_right= 0;
                    context.overlap_down = 0;

                    context.e_row = (imgH -1);
                    context.e_col = (imgW -1);
                }
                //up
                if((0 == i) && (0<j && j<col_num-1))
                {
                    context.overlap_up = 0;
                    context.s_row = 0;
                }
                //down
                if((row_num-1 == i) && (0<j && j<col_num-1))
                {
                    context.overlap_down = 0;
                    context.e_row = (imgH - 1);
                }
                //left
                if((0 == j) && (0<i && i<row_num-1))
                {
                    context.overlap_left = 0;
                    context.s_col = 0;
                }
                //right
                if((col_num-1 == j) && (0<i && i<row_num-1))
                {
                    context.overlap_right = 0;
                    context.e_col = (imgW - 1);
                }
            }

            //
            context.s_row -= context.overlap_up;
            context.e_row += context.overlap_down;
            context.s_col -= context.overlap_left;
            context.e_col += context.overlap_right;

            if(context.s_row < 0 
                || context.s_col < 0
                || context.e_col >= imgW
                || context.e_row >= imgH)
            {
                //ISPThrow("[Error] slice region [sx:%d,sy:%d,ex:%d,ey:%d] overflow!",context.s_col,context.s_row,context.e_col,context.e_row);
            }

            //check
            if(tem_slice_overlap_w >= imgW)
            {
                context.overlap_left  = 0;
                context.overlap_right = 0;
            }
            if(tem_slice_overlap_h >= imgH)
            {
                context.overlap_up   = 0;
                context.overlap_down = 0;
            }

            //
            //outParam.slices.push_back(context);
            slice_region_arr[index].sx = context.s_col;
            slice_region_arr[index].sy = context.s_row;
            slice_region_arr[index].ex = context.e_col;
            slice_region_arr[index].ey = context.e_row;
        }
    }
}

static void max_slice_region(
    pipe_region_info slice_region_arr_1[],
    pipe_region_info slice_region_arr_2[],
    int slice_rows,
    int slice_cols,
    pipe_region_info slice_region_arr[])
{
    int i,j,index;
    pipe_region_info *ptr1,*ptr2,*ptr_out;
    for (i=0;i<slice_rows;i++)
    {
        for (j=0;j<slice_cols;j++)
        {
            index = i*slice_cols+j;
            ptr1 = &slice_region_arr_1[index];
            ptr2 = &slice_region_arr_2[index];
            ptr_out = &slice_region_arr[index];

            ptr_out->sx = ptr1->sx;
            if(ptr2->sx < ptr1->sx)
                ptr_out->sx = ptr2->sx;

            ptr_out->ex = ptr1->ex;
            if(ptr2->ex > ptr1->ex)
                ptr_out->ex = ptr2->ex;

            ptr_out->sy = ptr1->sy;
            if(ptr2->sy < ptr1->sy)
                ptr_out->sy = ptr2->sy;

            ptr_out->ey = ptr1->ey;
            if(ptr2->ey > ptr1->ey)
                ptr_out->ey = ptr2->ey;
        }
    }
}
#if 0
static void set_slice_region(
    const pipe_region_info slice_region_arr_1[],
    pipe_region_info slice_region_arr_2[],
    int slice_rows,
    int slice_cols)
{
    int i,j,index;
    const pipe_region_info *ptr1;
    pipe_region_info *ptr2;
    for (i=0;i<slice_rows;i++)
    {
        for (j=0;j<slice_cols;j++)
        {
            index = i*slice_cols+j;
            ptr1 = &slice_region_arr_1[index];
            ptr2 = &slice_region_arr_2[index];

            ptr2->sx = ptr1->sx;
            ptr2->sy = ptr1->sy;
            ptr2->ex = ptr1->ex;
            ptr2->ey = ptr1->ey;
        }
    }
}
#endif

void slice_drv_param_calc(slice_drv_param_t *param_ptr)
{
    int i,j,index;
    int slice_rows;
    int slice_cols;
    pipe_region_info slice_region_arr_org[CPP_MAX_SLICE_NUM];
    pipe_region_info slice_region_arr_out[CPP_MAX_SLICE_NUM];
    pipe_region_info slice_region_arr_max[CPP_MAX_SLICE_NUM];
    pipe_overlap_context context;
    pipe_overlap_scaler_param scaler_path_param;
    pipe_overlap_scaler_param bypass_path_param;
    slice_drv_slice_param_t *fw_slice_param_ptr;
    pipe_region_info *region_ptr;

    memset(&scaler_path_param,0,sizeof(pipe_overlap_scaler_param));
    memset(&bypass_path_param,0,sizeof(pipe_overlap_scaler_param));

    // init context
    context.frameWidth  = param_ptr->img_w;
    context.frameHeight = param_ptr->img_h;
    if(param_ptr->crop_en)
    {
        context.frameWidth  = param_ptr->crop_width;
        context.frameHeight = param_ptr->crop_height;
    }
    context.pixelFormat = param_ptr->img_format;

    // init deci
    scaler_path_param.deci_x_eb = param_ptr->deci_param.deci_x_en;
    scaler_path_param.deci_x    = param_ptr->deci_param.deci_x;
    scaler_path_param.deci_y_eb = param_ptr->deci_param.deci_y_en;
    scaler_path_param.deci_y    = param_ptr->deci_param.deci_y;

    bypass_path_param.deci_x_eb = param_ptr->deci_param.deci_x_en;
    bypass_path_param.deci_x    = param_ptr->deci_param.deci_x;
    bypass_path_param.deci_y_eb = param_ptr->deci_param.deci_y_en;
    bypass_path_param.deci_y    = param_ptr->deci_param.deci_y;

    // init scaler
    scaler_path_param.trim_eb           = param_ptr->scaler_path_param.trim_eb;
    scaler_path_param.trim_start_x      = param_ptr->scaler_path_param.trim_start_x;
    scaler_path_param.trim_start_y      = param_ptr->scaler_path_param.trim_start_y;
    scaler_path_param.trim_size_x       = param_ptr->scaler_path_param.trim_size_x;
    scaler_path_param.trim_size_y       = param_ptr->scaler_path_param.trim_size_y;
    scaler_path_param.scaler_en         = param_ptr->scaler_path_param.scaler_en;
    scaler_path_param.des_size_x        = param_ptr->scaler_path_param.scaler_des_size_x;
    scaler_path_param.des_size_y        = param_ptr->scaler_path_param.scaler_des_size_y;
    scaler_path_param.yuv_output_format = param_ptr->scaler_path_param.scaler_output_format;

    bypass_path_param.trim_eb           = param_ptr->bypass_path_param.trim_eb;
    bypass_path_param.trim_start_x      = param_ptr->bypass_path_param.trim_start_x;
    bypass_path_param.trim_start_y      = param_ptr->bypass_path_param.trim_start_y;
    bypass_path_param.trim_size_x       = param_ptr->bypass_path_param.trim_size_x;
    bypass_path_param.trim_size_y       = param_ptr->bypass_path_param.trim_size_y;
    bypass_path_param.scaler_en         = 0;
    bypass_path_param.des_size_x        = 0;
    bypass_path_param.des_size_y        = 0;
    bypass_path_param.yuv_output_format = param_ptr->img_format;
    if(0 == param_ptr->bypass_path_param.enable)
    {
        bypass_path_param.trim_eb = 0;
    }

    fetch_slice(context.frameWidth,context.frameHeight,param_ptr->slice_w,9999,slice_region_arr_org,&slice_rows,&slice_cols);

    scaler_path_param.scaler_init_phase_hor = param_ptr->scaler_path_param.scaler_init_phase_hor;

    scaler_frame_init(&scaler_path_param, &context);
    {
        for (i=0;i<8;i++)
        {
            for (j=0;j<8;j++)
            {
                param_ptr->output.scaler_path_coef.y_hor_coef[i][j] = scaler_path_param.frame_param.scaler_info.scaler_coef_info.y_hor_coef[i][j];
                param_ptr->output.scaler_path_coef.c_hor_coef[i][j] = scaler_path_param.frame_param.scaler_info.scaler_coef_info.c_hor_coef[i][j];
            }
        }
        for (i=0;i<9;i++)
        {
            for (j=0;j<16;j++)
            {
                param_ptr->output.scaler_path_coef.y_ver_coef[i][j] = scaler_path_param.frame_param.scaler_info.scaler_coef_info.y_ver_coef[i][j];
                param_ptr->output.scaler_path_coef.c_ver_coef[i][j] = scaler_path_param.frame_param.scaler_info.scaler_coef_info.c_ver_coef[i][j];
            }
        }
    }
    //if(param_ptr->bypass_path_param.enable)
    //{
        scaler_frame_init(&bypass_path_param, &context);
    //}

    //step 1
    scaler_calculate_region(
        slice_region_arr_org,
        slice_rows,
        slice_cols,
        context.frameWidth,
        context.frameHeight,
        &scaler_path_param,
        0);

    //if(param_ptr->bypass_path_param.enable)
    //{
        scaler_calculate_region(
            slice_region_arr_org,
            slice_rows,
            slice_cols,
            context.frameWidth,
            context.frameHeight,
            &bypass_path_param,
            0);
    //}

    //if(param_ptr->bypass_path_param.enable)
    //{
        max_slice_region(scaler_path_param.region_input,bypass_path_param.region_input,slice_rows,slice_cols,slice_region_arr_max);
    //}
    //else
    //{
        //set_slice_region(scaler_path_param.region_input,slice_region_arr_max,slice_rows,slice_cols);
    //}

    //step 2
    scaler_calculate_region(
        slice_region_arr_max,
        slice_rows,
        slice_cols,
        context.frameWidth,
        context.frameHeight,
        &scaler_path_param,
        1);

    //if(param_ptr->bypass_path_param.enable)
    //{
        scaler_calculate_region(
            slice_region_arr_max,
            slice_rows,
            slice_cols,
            context.frameWidth,
            context.frameHeight,
            &bypass_path_param,
            1);
    //}

    //if(param_ptr->bypass_path_param.enable)
    //{
        max_slice_region(scaler_path_param.region_input,bypass_path_param.region_input,slice_rows,slice_cols,slice_region_arr_max);
    //}
    //else
    //{
        //set_slice_region(scaler_path_param.region_input,slice_region_arr_max,slice_rows,slice_cols);
    //}

    fetch_slice_ref(context.frameWidth,context.frameHeight,slice_region_arr_max,slice_rows,slice_cols,slice_region_arr_out);

    // calc slice param
    param_ptr->output.slice_count = slice_rows*slice_cols;
    for (i=0;i<slice_rows;i++)
    {
        for (j=0;j<slice_cols;j++)
        {
            int slice_w,slice_h;
            fw_slice_init_context slice_context;
            deci_info_t *deci_ptr = &scaler_path_param.frame_param.deci_info;

            index = i*slice_cols+j;
            region_ptr = &slice_region_arr_out[index];
            slice_w = region_ptr->ex - region_ptr->sx + 1;
            slice_h = region_ptr->ey - region_ptr->sy + 1;

            slice_context.slice_index = index;
            slice_context.rows = slice_rows;
            slice_context.cols = slice_cols;
            slice_context.slice_row_no = i;
            slice_context.slice_col_no = j;
            slice_context.slice_w = slice_w;
            slice_context.slice_h = slice_h;
            

            fw_slice_param_ptr = &param_ptr->output.hw_slice_param[index];
            //set HW param
            do 
            {
                fw_slice_param_ptr->Path0_src_pitch    = param_ptr->img_w;
                fw_slice_param_ptr->Path0_src_offset_x = region_ptr->sx;
                fw_slice_param_ptr->Path0_src_offset_y = region_ptr->sy;
                if(param_ptr->crop_en)
                {
                    fw_slice_param_ptr->Path0_src_offset_x += param_ptr->crop_start_x;
                    fw_slice_param_ptr->Path0_src_offset_y += param_ptr->crop_start_y;
                }
                fw_slice_param_ptr->Path0_src_width  = slice_w;
                fw_slice_param_ptr->Path0_src_height = slice_h;
                fw_slice_param_ptr->Input_format = param_ptr->img_format;
            } while (0);

            //deci
            //set HW param
            do
            {
                fw_slice_param_ptr->hor_deci = 0;
                fw_slice_param_ptr->ver_deci = 0;
                if(param_ptr->deci_param.deci_x_en)
                {
                    fw_slice_param_ptr->hor_deci = param_ptr->deci_param.deci_x + 1;
                }
                if(param_ptr->deci_param.deci_y_en)
                {
                    fw_slice_param_ptr->ver_deci = param_ptr->deci_param.deci_y + 1;
                }
                fw_slice_param_ptr->Sc_in_trim_src_width   = slice_w;
                fw_slice_param_ptr->Sc_in_trim_src_height  = slice_h;
                fw_slice_param_ptr->bypass_trim_src_width  = slice_w;
                fw_slice_param_ptr->bypass_trim_src_height = slice_h;
            }while(0);

            if(deci_ptr->deci_x_en == 1 || deci_ptr->deci_y_en == 1)
            {
                uint8  deci_x, deci_y;  //factor
                uint16 deci_h, deci_w ; //out size
                uint16 in_width;
                uint16 in_height;

                in_width  = slice_w;
                in_height = slice_h;
                deci_y = deci_ptr->deci_y;
                deci_x = deci_ptr->deci_x;
                deci_w = in_width/deci_x;
                deci_h = in_height/deci_y;

                slice_context.slice_w = deci_w;
                slice_context.slice_h = deci_h;

                //set HW param
                do
                {
                    fw_slice_param_ptr->Sc_in_trim_src_width  = deci_w;
                    fw_slice_param_ptr->Sc_in_trim_src_height = deci_h;
                }while(0);
            }

            //scaler path
            //set HW param
            do
            {
                fw_slice_param_ptr->Sc_full_in_width  = scaler_path_param.frame_param.src_size_x;
                fw_slice_param_ptr->Sc_full_in_height = scaler_path_param.frame_param.src_size_y;

                if(scaler_path_param.frame_param.deci_info.deci_x_en)
                    fw_slice_param_ptr->Sc_full_in_width  = scaler_path_param.frame_param.src_size_x/scaler_path_param.frame_param.deci_info.deci_x;
                if(scaler_path_param.frame_param.deci_info.deci_y_en)
                    fw_slice_param_ptr->Sc_full_in_height = scaler_path_param.frame_param.src_size_y/scaler_path_param.frame_param.deci_info.deci_y;

                if(scaler_path_param.frame_param.trim0_info.trim_en)
                {
                    fw_slice_param_ptr->Sc_full_in_width  = scaler_path_param.frame_param.trim0_info.trim_size_x;
                    fw_slice_param_ptr->Sc_full_in_height = scaler_path_param.frame_param.trim0_info.trim_size_y;
                }

                fw_slice_param_ptr->Sc_full_out_width  = scaler_path_param.frame_param.dst_size_x;
                fw_slice_param_ptr->Sc_full_out_height = scaler_path_param.frame_param.dst_size_y;
            }
            while(0);

            //////////////////////////////////////////////////////////////////////////
            scaler_slice_init(
                region_ptr,
                &slice_context,
                &scaler_path_param);

            //scaler path
            //set HW param
            do 
            {
                fw_slice_param_ptr->Path0_sc_des_offset_x = scaler_path_param.ref_ox;
                fw_slice_param_ptr->Path0_sc_des_offset_y = scaler_path_param.ref_oy;
            } while (0);

            scaler_path_param.ref_ox += scaler_path_param.slice_param.trim1_info.trim_size_x;
            if((j + 1) == slice_cols)
                scaler_path_param.ref_oy += scaler_path_param.slice_param.trim1_info.trim_size_y;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            scaler_slice_init(
                region_ptr,
                &slice_context,
                &bypass_path_param);

            //bypass path
            //set HW param
            do 
            {
                fw_slice_param_ptr->Path0_bypass_des_offset_x = bypass_path_param.ref_ox;
                fw_slice_param_ptr->Path0_bypass_des_offset_y = bypass_path_param.ref_oy;
            } while (0);

            bypass_path_param.ref_ox += bypass_path_param.slice_param.trim1_info.trim_size_x;
            if((j + 1) == slice_cols)
                bypass_path_param.ref_oy += bypass_path_param.slice_param.trim1_info.trim_size_y;
            //////////////////////////////////////////////////////////////////////////

            //set HW param
            do
            {
                if(1)// scaler path
                {
                    if(scaler_path_param.slice_param.trim0_info.trim_en)
                    {
                        fw_slice_param_ptr->Sc_in_trim_offset_x = scaler_path_param.slice_param.trim0_info.trim_start_x;
                        fw_slice_param_ptr->Sc_in_trim_offset_y = scaler_path_param.slice_param.trim0_info.trim_start_y;
                        fw_slice_param_ptr->Sc_in_trim_width    = scaler_path_param.slice_param.trim0_info.trim_size_x;
                        fw_slice_param_ptr->Sc_in_trim_height   = scaler_path_param.slice_param.trim0_info.trim_size_y;
                    }
                    else
                    {
                        fw_slice_param_ptr->Sc_in_trim_offset_x = 0;
                        fw_slice_param_ptr->Sc_in_trim_offset_y = 0;
                        fw_slice_param_ptr->Sc_in_trim_width    = fw_slice_param_ptr->Sc_in_trim_src_width;
                        fw_slice_param_ptr->Sc_in_trim_height   = fw_slice_param_ptr->Sc_in_trim_src_height;
                    }
                    fw_slice_param_ptr->Sc_slice_in_width   = fw_slice_param_ptr->Sc_in_trim_width;
                    fw_slice_param_ptr->Sc_slice_in_height  = fw_slice_param_ptr->Sc_in_trim_height;
                    fw_slice_param_ptr->Sc_slice_out_width  = scaler_path_param.slice_param.dst_size_x;
                    fw_slice_param_ptr->Sc_slice_out_height = scaler_path_param.slice_param.dst_size_y;
                }

                if(1)//bypass path
                {
                    fw_slice_param_ptr->path0_bypass_path_en = param_ptr->bypass_path_param.enable;
                    if(bypass_path_param.slice_param.trim0_info.trim_en)
                    {
                        fw_slice_param_ptr->bypass_trim_offset_x = bypass_path_param.slice_param.trim0_info.trim_start_x;
                        fw_slice_param_ptr->bypass_trim_offset_y = bypass_path_param.slice_param.trim0_info.trim_start_y;
                        fw_slice_param_ptr->bypass_trim_width    = bypass_path_param.slice_param.trim0_info.trim_size_x;
                        fw_slice_param_ptr->bypass_trim_height   = bypass_path_param.slice_param.trim0_info.trim_size_y;
                    }
                    else
                    {
                        fw_slice_param_ptr->bypass_trim_offset_x = 0;
                        fw_slice_param_ptr->bypass_trim_offset_y = 0;
                        fw_slice_param_ptr->bypass_trim_width    = fw_slice_param_ptr->Sc_in_trim_src_width;
                        fw_slice_param_ptr->bypass_trim_height   = fw_slice_param_ptr->Sc_in_trim_src_height;
                    }
                }
            }while(0);

            //set HW param
            do
            {
                if(1)//scaler path
                {
                    fw_slice_param_ptr->y_hor_ini_phase_int   = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_int[0][0];
                    fw_slice_param_ptr->y_hor_ini_phase_frac  = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_rmd[0][0];
                    fw_slice_param_ptr->uv_hor_ini_phase_int  = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_int[0][1];
                    fw_slice_param_ptr->uv_hor_ini_phase_frac = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_rmd[0][1];

                    fw_slice_param_ptr->y_ver_ini_phase_int   = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_int[1][0];
                    fw_slice_param_ptr->y_ver_ini_phase_frac  = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_rmd[1][0];
                    fw_slice_param_ptr->uv_ver_ini_phase_int  = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_int[1][1];
                    fw_slice_param_ptr->uv_ver_ini_phase_frac = scaler_path_param.slice_param.scaler_info.init_phase_info.scaler_init_phase_rmd[1][1];

                    fw_slice_param_ptr->y_ver_tap  = scaler_path_param.slice_param.scaler_info.scaler_y_ver_tap;
                    fw_slice_param_ptr->uv_ver_tap = scaler_path_param.slice_param.scaler_info.scaler_uv_ver_tap;

                    fw_slice_param_ptr->Sc_out_trim_src_width  = fw_slice_param_ptr->Sc_slice_out_width;
                    fw_slice_param_ptr->Sc_out_trim_src_height = fw_slice_param_ptr->Sc_slice_out_height;
                    fw_slice_param_ptr->Sc_out_trim_offset_x   = scaler_path_param.slice_param.trim1_info.trim_start_x;
                    fw_slice_param_ptr->Sc_out_trim_offset_y   = scaler_path_param.slice_param.trim1_info.trim_start_y;
                    fw_slice_param_ptr->Sc_out_trim_width      = scaler_path_param.slice_param.trim1_info.trim_size_x;
                    fw_slice_param_ptr->Sc_out_trim_height     = scaler_path_param.slice_param.trim1_info.trim_size_y;

                    fw_slice_param_ptr->Path0_sc_des_width     = fw_slice_param_ptr->Sc_out_trim_width;
                    fw_slice_param_ptr->Path0_sc_des_height    = fw_slice_param_ptr->Sc_out_trim_height;
                    fw_slice_param_ptr->Path0_sc_output_format = scaler_path_param.slice_param.output_pixfmt; //0:422 1:420
                    if (param_ptr->scaler_path_param.scaler_des_pitch != 0)
						fw_slice_param_ptr->Path0_sc_des_pitch     = param_ptr->scaler_path_param.scaler_des_pitch;
                    else
						fw_slice_param_ptr->Path0_sc_des_pitch     = scaler_path_param.frame_param.dst_size_x;
                }

                if(1)//bypass path
                {
                    fw_slice_param_ptr->Path0_bypass_des_width  = fw_slice_param_ptr->bypass_trim_width;
                    fw_slice_param_ptr->Path0_bypass_des_height = fw_slice_param_ptr->bypass_trim_height;
                    if (param_ptr->bypass_path_param.bp_des_pitch != 0)
						fw_slice_param_ptr->Path0_bypass_des_pitch  = param_ptr->bypass_path_param.bp_des_pitch;
                    else
						fw_slice_param_ptr->Path0_bypass_des_pitch  = bypass_path_param.frame_param.dst_size_x;
                }
            }while(0);

            //
            do 
            {
                if(YUV422 == fw_slice_param_ptr->Input_format)//422
                    fw_slice_param_ptr->Input_format = 2;
                if(YUV420 == fw_slice_param_ptr->Input_format)//420
                    fw_slice_param_ptr->Input_format = 0;

                if(YUV422 == fw_slice_param_ptr->Path0_sc_output_format)
                    fw_slice_param_ptr->Path0_sc_output_format = 2;
                if(YUV420 == fw_slice_param_ptr->Path0_sc_output_format)
                    fw_slice_param_ptr->Path0_sc_output_format = 0;

                fw_slice_param_ptr->Path0_bypass_output_format =fw_slice_param_ptr->Input_format;
            } while (0);
        }
    }

}
