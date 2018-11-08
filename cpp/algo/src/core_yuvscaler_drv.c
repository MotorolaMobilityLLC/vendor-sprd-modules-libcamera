#include "core_yuvscaler_drv.h"
#include <math.h>
#include <malloc.h>
#include <memory.h>
#include "algo_macro.h"
#include <stdlib.h>

#undef assert
#if 1
void __cppassert (const char *function, const char *file, unsigned int line,
		const char *assertion)
{
	printf("ASSERT: %s %s <%d> : %s\n", file, function, line, assertion);
	while(1);
}
#endif


//#define	assert(e)	((void)0)
#define	assert(e)	((e) ? (void)0 : __cppassert(__func__, __FILE__,  __LINE__, #e))

int16 CalY_ScalingCoef(int16 tap, float D, float I, int16 *y_coef_data_ptr, int16 dir);

int16 CalUV_ScalingCoef(int16 tap, float D, float I, int16 *uv_coef_data_ptr, int16 dir);

int16 CalYmodelCoef(int16 coef_lenght, int16 *coef_data_ptr, float tmp_M, float base);

int16 CalUVmodelCoef(int16 coef_lenght, int16 *coef_data_ptr, double tmp_M);

void  GetFilter(int16       pos_start       ,
    int16       *coef_data_ptr  ,
    int16       *out_filter     ,
    int16       iI_hor          ,
    int16       coef_len        ,
    int16       *filter_len
    );

void  WriteScalarCoef(int16 scale_coef[8][32]   ,
    int16   *coef_ptr           ,
    int16   len
    );

void convertYUV422ToYUV420(
    uint8   *pYUV422Data,
    uint8   **ppYUV420Data,
    uint16  width,
    uint16  height
    );
void convertYUV420PToYUV420SP(
    uint8   *pYUV420Data,
    uint16  width,
    uint16  height
    );

void convertYUV422PToYUV422SP(
    uint8   *pYUV422Data,
    uint16  width,
    uint16  height
    );

//uint8 ROTATE_MODE_MAP[]={ROTATE0,FLIP,ROTATE180,MIRROR};
#define SCL_HOR_MAX_TAP  8
#define SCL_VER_MAX_TAP  16
#define SCL_OVERLAP_UP     (SCL_VER_MAX_TAP/2)
#define SCL_OVERLAP_DOWN   (SCL_VER_MAX_TAP/2+1)
#define SCL_OVERLAP_LEFT   (SCL_HOR_MAX_TAP/2)
#define SCL_OVERLAP_RIGHT  (SCL_HOR_MAX_TAP/2+1)


void scaler_phase_mapping(int32 luma_phase[2], int32 chroma_phase[2], int pixfmt)
{
    switch (pixfmt)
    {
    case YUV420:
        chroma_phase[0] = luma_phase[0]/2;
        chroma_phase[1] = luma_phase[1]/2;
        break;
    case YUV422:
        chroma_phase[0] = luma_phase[0]/2;
        chroma_phase[1] = luma_phase[1];
    default:
        break;
    }
}

void calc_scaler_phase(int32 phase, uint16 factor, int16 *phase_int, uint16 *phase_rmd)
{
    phase_int[0] = (int16)floor(1.0f*phase/factor);
    phase_rmd[0] = (uint16)(phase - factor*phase_int[0]);
}

//input:    \CA\E4\C8\EBͼ\CF\F1(frame)\B5\C4pixel_alignment, trim_start, trim_size, decimation, scaler setting;
//          \CA\E4\B3\F6ͼ\CF\F1\D6е\C4sliceλ\D6\C3, \CA\E4\C8\EBͼ\CF\F1\B5\C4pixel\B6\D4\C6\EBҪ\C7\F3
//output:   \CA\E4\C8\EBͼ\CF\F1\D6е\C4sliceλ\D6\C3\D2Լ\B0scaler phase (other saler setting should be kept same as frame-level scaler setting
void calc_scaler_input_slice_info(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int output_slice_start, int output_slice_size, int input_pixel_align,
    int *input_slice_start, int *input_slice_size, int *input_slice_phase)
{
    int sphase, ephase;
    int spixel, epixel;
    int fullsize_size = *input_slice_size;
    int spixel_end = fullsize_size/deci;
    if(trim_en)
        spixel_end = trim_size;

    assert(scl_tap%2 == 0);
    //assert(trim_start%input_pixel_align == 0);
    //assert(trim_size%deci == 0);
    //assert(deci_size%input_pixel_align == 0);

    sphase = init_phase + output_slice_start*scl_factor_in;//start phase
    ephase = sphase + (output_slice_size-1)*scl_factor_in;//end phase (interior)

    spixel = sphase/scl_factor_out - scl_tap/2 + 1;//start pixel index
    epixel = ephase/scl_factor_out + scl_tap/2;//end pixel index (interior)

    //adjust the input slice position for alignment
    spixel = (spixel/input_pixel_align)*input_pixel_align;//start pixel (aligned)
    epixel = (epixel/input_pixel_align+1)*input_pixel_align;//end pixel (aligned, exterior); epixel = (((epixel+1) + (pixel_align-1))/pixel_align)*pixel_align = (epixel/pixel_align + 1)*pixel_align

    spixel = spixel < 0 ? 0 : spixel;
    epixel = epixel > spixel_end ? spixel_end : epixel;

    //calculate the initial phase of current slice (local phase)
    sphase -= spixel*scl_factor_out;

    //trim
    if(trim_en)
    {
        spixel += trim_start;
        epixel += trim_start;
    }

    //calculate the input slice position of deci
    spixel *= deci;
    epixel *= deci;

    *input_slice_start = spixel;
    *input_slice_size  = epixel - spixel;
    *input_slice_phase = sphase;
}

//input:    \CA\E4\C8\EBͼ\CF\F1(frame)\B5\C4pixel_alignment, trim_start, trim_size, decimation, scaler setting;
//          \CA\E4\C8\EBͼ\CF\F1\D6е\C4sliceλ\D6\C3, \CA\E4\B3\F6ͼ\CF\F1\B5\C4pixel\B6\D4\C6\EBҪ\C7\F3
//output:   \CA\E4\B3\F6ͼ\CF\F1\D6е\C4sliceλ\D6\C3(\C8\E7\B9\FB\CA\E4\C8\EBͼ\CF\F1sliceλ\D6\C3\D3\EBtrim\B4\B0\BF\DAû\D3\D0\D6ص\FE\C7\F8\D3\F2\A3\AC\D4\F2û\D3ж\D4Ӧ\B5\C4\CA\E4\B3\F6\A3\AC\B4\CBʱ\B7\B5\BBص\C4output_slice_size = 0)
void calc_scaler_output_slice_info(int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align,
    int *output_slice_start, int *output_slice_size)
{
    int spixel, epixel;
    int deci_size = trim_size/deci;
    int input_slice_end = input_slice_start + input_slice_size;
    int trim_end = trim_start + trim_size;

    assert(scl_tap%2 == 0);
    assert(trim_size%deci == 0);

    //trim
    input_slice_start = input_slice_start < trim_start ? trim_start : input_slice_start;
    input_slice_start = input_slice_start - trim_start;

    //deci
    input_slice_start = (input_slice_start + deci - 1)/deci;
    if (input_slice_start != 0)
        input_slice_start += scl_tap/2 - 1;

    //trim
    input_slice_end = input_slice_end > trim_end ? trim_end : input_slice_end;
    input_slice_end = input_slice_end - trim_start;

    //deci
    input_slice_end = input_slice_end/deci;
    if (input_slice_end != deci_size)
        input_slice_end -= scl_tap/2;

    //scale
    spixel = (input_slice_start*scl_factor_out - init_phase + scl_factor_in - 1)/scl_factor_in;
    //epixel = ((input_slice_end - 1)*scl_factor_out - init_phase)/scl_factor_in + 1;
    //epixel = (input_slice_end*(scl_factor_out-1) - init_phase)/scl_factor_in + 1;
    epixel = (input_slice_end*scl_factor_out - 1 - init_phase)/scl_factor_in + 1;

    //align
    spixel = ((spixel + output_pixel_align - 1)/output_pixel_align)*output_pixel_align;
    epixel = (epixel/output_pixel_align)*output_pixel_align;

    //output
    *output_slice_start = spixel;
    *output_slice_size = epixel > spixel ? epixel - spixel : 0;
}

void calc_scaler_output_slice_info_v2(int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align,
    int *output_slice_start, int *output_slice_size)
{
    int spixel, epixel;
    int input_slice_end = input_slice_start + input_slice_size;
    int trim_end = trim_start + trim_size;

    assert(scl_tap%2 == 0);
    assert(trim_size%deci == 0);

    //trim
    input_slice_start = input_slice_start < trim_start ? trim_start : input_slice_start;
    input_slice_start = input_slice_start - trim_start;

    //deci
    input_slice_start = (input_slice_start + deci - 1)/deci;
    //if (input_slice_start != 0)
    //    input_slice_start += scl_tap/2 - 1;

    //trim
    input_slice_end = input_slice_end > trim_end ? trim_end : input_slice_end;
    input_slice_end = input_slice_end - trim_start;

    //deci
    input_slice_end = input_slice_end/deci;
    //if (input_slice_end != deci_size)
    //    input_slice_end -= scl_tap/2;

    //scale
    spixel = (input_slice_start*scl_factor_out - init_phase)/scl_factor_in;
    //epixel = ((input_slice_end - 1)*scl_factor_out - init_phase)/scl_factor_in + 1;
    epixel = (input_slice_end*scl_factor_out - 1 - init_phase)/scl_factor_in + 1;

    //align
    spixel = ((spixel + output_pixel_align - 1)/output_pixel_align)*output_pixel_align;
    epixel = ((epixel + output_pixel_align - 1)/output_pixel_align)*output_pixel_align;

    //output
    *output_slice_start = spixel;
    *output_slice_size = epixel > spixel ? epixel - spixel : 0;
}

void est_scaler_output_slice_info(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align, int *output_slice_end)
{
    int /*spixel, */epixel;
    //int deci_size = trim_size/deci;
    int input_slice_end = input_slice_start + input_slice_size;
    int trim_end = trim_start + trim_size;

    assert(scl_tap%2 == 0);
    //assert(trim_size%deci == 0);

    //deci
    input_slice_end = input_slice_end/deci;

    //trim
    if(trim_en)
    {
        input_slice_end = input_slice_end > trim_end ? trim_end : input_slice_end;
        input_slice_end = input_slice_end - trim_start;
    }

    //scale
    epixel = (input_slice_end*scl_factor_out - 1 - init_phase)/scl_factor_in + 1;

    //align
    //epixel = ((epixel + output_pixel_align - 1)/output_pixel_align)*output_pixel_align;
    epixel = ((epixel + output_pixel_align/2)/output_pixel_align)*output_pixel_align;

    if(epixel < 0)
        epixel = 0;

    //output
    *output_slice_end = epixel;
}

void est_scaler_output_slice_info_v2(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align, int *output_slice_end)
{
    int /*spixel, */epixel;
    //int deci_size = trim_size/deci;
    int fullsize_size = *output_slice_end;
    int deci_size = fullsize_size/deci;
    int input_slice_end = input_slice_start + input_slice_size;
    int trim_end = trim_start + trim_size;
    int scaler_size = deci_size;
    if(trim_en)
        scaler_size = trim_size;

    assert(scl_tap%2 == 0);
    //assert(trim_size%deci == 0);

    //deci
    input_slice_end = input_slice_end/deci;

    //trim
    if(trim_en)
    {
        input_slice_end = input_slice_end > trim_end ? trim_end : input_slice_end;
        input_slice_end = input_slice_end - trim_start;
    }

    //scale
    if (input_slice_end != scaler_size)
        input_slice_end -= scl_tap/2;

    epixel = (input_slice_end*scl_factor_out - 1 - init_phase)/scl_factor_in + 1;

    //align
    epixel = (epixel/output_pixel_align)*output_pixel_align;

    if(epixel < 0)
        epixel = 0;

    //output
    *output_slice_end = epixel;
}

// for SharkL2 yuvscaler  huoxing 2016.3.14
void yuv_scaler_init_frame_info(yuvscaler_param_t *pYuvScaler)
{
    scaler_info_t *pScalerInfo = NULL;
    uint16 new_width = pYuvScaler->src_size_x;
    uint16 new_height = pYuvScaler->src_size_y;
    float adj_hor = 1.0f;
    float adj_ver = 1.0f;

    if (!pYuvScaler->bypass)
    {
        //init deci info
        if(pYuvScaler->deci_info.deci_x_en == 0)
            pYuvScaler->deci_info.deci_x = 1;
        if(pYuvScaler->deci_info.deci_y_en == 0)
            pYuvScaler->deci_info.deci_y = 1;

        //FIXME: need refer to indata_mode
        /*
        assert(pYuvScaler->trim0_info.trim_start_x % 4 == 0);
        assert(pYuvScaler->trim0_info.trim_size_x  % 4 == 0);
        if(pYuvScaler->input_pixfmt == YUV422)
        {
            assert(pYuvScaler->trim0_info.trim_start_y % 2 == 0);
            assert(pYuvScaler->trim0_info.trim_size_y  % 2 == 0);
        }
        else if(pYuvScaler->input_pixfmt == YUV420)
        {
            assert(pYuvScaler->trim0_info.trim_start_y % 4 == 0);
            assert(pYuvScaler->trim0_info.trim_size_y  % 4 == 0);
        }
        */

        if (pYuvScaler->deci_info.deci_x_en)
            new_width = new_width/pYuvScaler->deci_info.deci_x;
        
        if(pYuvScaler->trim0_info.trim_en)
            new_width = pYuvScaler->trim0_info.trim_size_x;

        if (pYuvScaler->deci_info.deci_y_en)
            new_height = new_height/pYuvScaler->deci_info.deci_y;
        
        if(pYuvScaler->trim0_info.trim_en)
            new_height = pYuvScaler->trim0_info.trim_size_y;

        pScalerInfo = &pYuvScaler->scaler_info;

        pScalerInfo->scaler_in_width = new_width;
        pScalerInfo->scaler_in_height= new_height;

        if (pScalerInfo->scaler_en)
        {
            int32  scl_init_phase_hor, scl_init_phase_ver;
            uint16 scl_factor_in_hor, scl_factor_out_hor;
            uint16 scl_factor_in_ver, scl_factor_out_ver;
            uint8  tap_luma_hor = 8, tap_chroma_hor = 8;
            uint8  tap_luma_ver = 8, tap_chroma_ver = 8;
            uint16 i_w,o_w,i_h,o_h;

            i_w = pScalerInfo->scaler_in_width;
            o_w = pScalerInfo->scaler_out_width;
            i_h = pScalerInfo->scaler_in_height;
            o_h = pScalerInfo->scaler_out_height;

            assert(i_w%2==0 && o_w%2==0 && i_w <= o_w*4 && o_w <= i_w*4);
            assert(i_h%2==0 && o_h%2==0 && i_h <= o_h*4 && o_h <= i_h*4);

            //if (MIN(i_w, o_w) < 256)
            //  adj_hor = 256.0f/MIN(i_w, o_w);
            //if (MIN(i_h, o_h) < 256)
            //  adj_ver = 256.0f/MIN(i_h, o_h);
            scl_factor_in_hor  = (uint16)(i_w*adj_hor);
            scl_factor_out_hor = (uint16)(o_w*adj_hor);
            scl_factor_in_ver  = (uint16)(i_h*adj_ver);
            scl_factor_out_ver = (uint16)(o_h*adj_ver);

            pScalerInfo->scaler_factor_in_hor = scl_factor_in_hor;
            pScalerInfo->scaler_factor_out_hor = scl_factor_out_hor;
            pScalerInfo->scaler_factor_in_ver = scl_factor_in_ver;
            pScalerInfo->scaler_factor_out_ver = scl_factor_out_ver;

            //better choice for initial_phase: initial_phase_y = (scl_factor_in_hor - scl_factor_out_hor)/2;
            //scl_init_phase_hor = 0;
            scl_init_phase_hor = pYuvScaler->init_phase_hor;
            scl_init_phase_ver = 0;
            pScalerInfo->init_phase_info.scaler_init_phase[0] = scl_init_phase_hor;
            pScalerInfo->init_phase_info.scaler_init_phase[1] = scl_init_phase_ver;

            // hor
            calc_scaler_phase(scl_init_phase_hor, scl_factor_out_hor, &pScalerInfo->init_phase_info.scaler_init_phase_int[0][0], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[0][0]);  //luma
            calc_scaler_phase(scl_init_phase_hor/4, scl_factor_out_hor/2, &pScalerInfo->init_phase_info.scaler_init_phase_int[0][1], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[0][1]);   //chroma

            // ver
              //luma
            calc_scaler_phase(scl_init_phase_ver, scl_factor_out_ver, &pScalerInfo->init_phase_info.scaler_init_phase_int[1][0], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[1][0]);
            //FIXME: need refer to input_pixfmt
              //chroma
            if(pYuvScaler->input_pixfmt == YUV422)
            {
                if(pYuvScaler->output_pixfmt == YUV422)
                    calc_scaler_phase(scl_init_phase_ver,   scl_factor_out_ver,   &pScalerInfo->init_phase_info.scaler_init_phase_int[1][1], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[1][1]);
                else if(pYuvScaler->output_pixfmt == YUV420)
                    calc_scaler_phase(scl_init_phase_ver/2, scl_factor_out_ver/2, &pScalerInfo->init_phase_info.scaler_init_phase_int[1][1], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[1][1]);
            }
            else if(pYuvScaler->input_pixfmt == YUV420)
            {
                if(pYuvScaler->output_pixfmt == YUV422)
                    calc_scaler_phase(scl_init_phase_ver/2,  scl_factor_out_ver,   &pScalerInfo->init_phase_info.scaler_init_phase_int[1][1], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[1][1]);
                else if(pYuvScaler->output_pixfmt == YUV420)
                    calc_scaler_phase(scl_init_phase_ver/4,  scl_factor_out_ver/2, &pScalerInfo->init_phase_info.scaler_init_phase_int[1][1], &pScalerInfo->init_phase_info.scaler_init_phase_rmd[1][1]);
            }

            //FIXME: need update the below function
            yuv_scaler_gen_scaler_coef(
                scl_factor_in_hor, 
                scl_factor_in_ver,
                scl_factor_out_hor,
                scl_factor_out_ver,
                pYuvScaler->input_pixfmt,
                pYuvScaler->output_pixfmt,
                &tap_luma_hor, 
                &tap_chroma_hor,
                &tap_luma_ver,
                &tap_chroma_ver,
                &pScalerInfo->scaler_coef_info);

            pScalerInfo->scaler_y_hor_tap  = tap_luma_hor;
            pScalerInfo->scaler_uv_hor_tap = tap_chroma_hor;
            pScalerInfo->scaler_y_ver_tap  = tap_luma_ver;
            pScalerInfo->scaler_uv_ver_tap = tap_chroma_ver;
        }
        else
        {
            pScalerInfo->init_phase_info.scaler_init_phase[0] = 0;
            pScalerInfo->init_phase_info.scaler_init_phase[1] = 0;

            pScalerInfo->scaler_y_hor_tap  = 0;
            pScalerInfo->scaler_uv_hor_tap = 0;
            pScalerInfo->scaler_y_ver_tap  = 0;
            pScalerInfo->scaler_uv_ver_tap = 0;

            pScalerInfo->scaler_out_width  = pScalerInfo->scaler_in_width;
            pScalerInfo->scaler_out_height = pScalerInfo->scaler_in_height;

            pScalerInfo->scaler_factor_in_hor  = pScalerInfo->scaler_in_width;
            pScalerInfo->scaler_factor_out_hor = pScalerInfo->scaler_out_width;
            pScalerInfo->scaler_factor_in_ver  = pScalerInfo->scaler_in_height;
            pScalerInfo->scaler_factor_out_ver = pScalerInfo->scaler_out_height;
        }
        new_width = pScalerInfo->scaler_out_width;
        new_height = pScalerInfo->scaler_out_height;

        pYuvScaler->trim1_info.trim_en = 0;
        pYuvScaler->trim1_info.trim_start_x = 0;
        pYuvScaler->trim1_info.trim_start_y = 0;
        pYuvScaler->trim1_info.trim_size_x = new_width;
        pYuvScaler->trim1_info.trim_size_y = new_height;
    }

    pYuvScaler->dst_size_x = new_width;
    pYuvScaler->dst_size_y = new_height;
}

/*
void yuv_scaler_init_slice_info(yuvscaler_param_t *frame_scaler, yuvscaler_param_t *slice_scaler, scaler_slice_t *slice_info, scaler_overlap_t *scaler_overlap, uint16 ref_ox, uint16 ref_oy)
{
    int trim_start, trim_size;
    int deci;
    int scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase;
    int input_slice_start, input_slice_size, input_pixel_align;
    int output_slice_start, output_slice_size, output_pixel_align;
    int overlap_head, overlap_tail;
    int start_col_org, start_row_org;

    memcpy(slice_scaler, frame_scaler, sizeof(yuvscaler_param_t));

    //hor
    trim_start = frame_scaler->trim0_info.trim_start_x;
    trim_size = frame_scaler->trim0_info.trim_size_x;
    deci = frame_scaler->deci_info.deci_x;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_hor;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_hor;
    scl_tap = frame_scaler->scaler_info.scaler_y_hor_tap;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[0];

    slice_scaler->src_size_x = slice_info->slice_width;

    overlap_head = slice_info->overlap_left > 0 ? slice_info->overlap_left - scaler_overlap->overlap_left : 0;
    overlap_tail = slice_info->overlap_right > 0 ? slice_info->overlap_right - scaler_overlap->overlap_right : 0;
    slice_info->overlap_left -= overlap_head;
    slice_info->overlap_right -= overlap_tail;

    start_col_org = slice_info->start_col; //copy for trim0

    slice_info->start_col += overlap_head;
    slice_info->end_col -= overlap_tail;
    slice_info->slice_width = slice_info->end_col - slice_info->start_col + 1;

    input_slice_start = slice_info->start_col;
    input_slice_size = slice_info->slice_width;
    input_pixel_align = 2; //YUV420
    output_pixel_align = 2; //YUV420

    output_slice_size = 0;
    calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_x = 0;
        slice_scaler->trim0_info.trim_size_x = 0;
    }
    else
    {
        if (output_slice_start < ref_ox)
        {
            output_slice_size -= ref_ox - output_slice_start;
            output_slice_start = ref_ox;
        }

        calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
            output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        slice_scaler->trim0_info.trim_start_x = input_slice_start - start_col_org;
        slice_scaler->trim0_info.trim_size_x  = input_slice_size;

        slice_scaler->scaler_info.scaler_in_width  = input_slice_size/deci;
        slice_scaler->scaler_info.scaler_out_width = output_slice_size;

        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[0] = init_phase;
        calc_scaler_phase(init_phase,   scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][0]);  //luma
        calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][1]);  //chroma

        //slice_scaler->trim1_info.trim_start_x = 0;
        //slice_scaler->trim1_info.trim_size_x = output_slice_size;
    }
    slice_scaler->dst_start_x = output_slice_start;
    slice_scaler->dst_size_x = output_slice_size;
    slice_scaler->trim1_info.trim_start_x = 0;
    slice_scaler->trim1_info.trim_size_x = output_slice_size;

    //ver
    trim_start = frame_scaler->trim0_info.trim_start_y;
    trim_size = frame_scaler->trim0_info.trim_size_y;
    deci = frame_scaler->deci_info.deci_y;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_ver;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_ver;
    //FIXME: 420 input
    if(frame_scaler->input_pixfmt == YUV422)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap)   + 2; //huoxing: +2 to avoid using special tap
    else if(frame_scaler->input_pixfmt == YUV420)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap*2) + 2;

    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[1];

    slice_scaler->src_size_y = slice_info->slice_height;

    overlap_head = slice_info->overlap_up > 0 ? slice_info->overlap_up - scaler_overlap->overlap_up : 0;
    overlap_tail = slice_info->overlap_down > 0 ? slice_info->overlap_down - scaler_overlap->overlap_down : 0;
    slice_info->overlap_up -= overlap_head;
    slice_info->overlap_down -= overlap_tail;

    start_row_org = slice_info->start_row; //copy for trim0

    slice_info->start_row += overlap_head;
    slice_info->end_row -= overlap_tail;
    slice_info->slice_height = slice_info->end_row - slice_info->start_row + 1;

    input_slice_start = slice_info->start_row;
    input_slice_size = slice_info->slice_height;
    //assert(frame_scaler->input_pixfmt == YUV422);
    input_pixel_align = 2; //YUV420
    if (frame_scaler->output_pixfmt == YUV422)
        output_pixel_align = 2; 
    else if (frame_scaler->output_pixfmt == YUV420) 
        output_pixel_align = 4;
    
    output_slice_size = 0;
    calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_y = 0;
        slice_scaler->trim0_info.trim_size_y = 0;
    }
    else
    {
        if (output_slice_start < ref_oy)
        {
            output_slice_size -= (ref_oy - output_slice_start);
            output_slice_start = ref_oy;
        }

        calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
            output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        slice_scaler->trim0_info.trim_start_y = input_slice_start - start_row_org;
        slice_scaler->trim0_info.trim_size_y = input_slice_size;

        slice_scaler->scaler_info.scaler_in_height = input_slice_size/deci;
        slice_scaler->scaler_info.scaler_out_height = output_slice_size;
        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[1] = init_phase;
        
        //FIXME: need refer to input_pixfmt
        //luma
        calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][0]);
        //chroma
        if(slice_scaler->scaler_info.input_pixfmt == YUV422)
        {           
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)          
                calc_scaler_phase(init_phase/2, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);           
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)           
                calc_scaler_phase(init_phase,   scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);

        }
        else if(slice_scaler->scaler_info.input_pixfmt == YUV420)
        {
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)
                calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)
                calc_scaler_phase(init_phase/2, scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
        }
        //slice_scaler->trim1_info.trim_start_y = 0;
        //slice_scaler->trim1_info.trim_size_y = output_slice_size;
    }
    slice_scaler->dst_start_y = output_slice_start;
    slice_scaler->dst_size_y  = output_slice_size;
    slice_scaler->trim1_info.trim_start_y = 0;
    slice_scaler->trim1_info.trim_size_y  = output_slice_size;
}
*/

/*
void yuv_scaler_init_slice_info_v2(yuvscaler_param_t *frame_scaler, yuvscaler_param_t *slice_scaler, 
    scaler_slice_t *slice_info, const scaler_slice_t *scaler_slice_info,
    scaler_overlap_t *scaler_overlap, uint16 ref_ox, uint16 ref_oy)
{
    int trim_start, trim_size;
    int deci;
    int scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase;
    int input_slice_start, input_slice_size, input_pixel_align;
    int output_slice_start, output_slice_size, output_pixel_align;
    int overlap_head, overlap_tail;
    int start_col_org, start_row_org;

    memcpy(slice_scaler, frame_scaler, sizeof(yuvscaler_param_t));

    //hor
    trim_start = frame_scaler->trim0_info.trim_start_x;
    trim_size = frame_scaler->trim0_info.trim_size_x;
    deci = frame_scaler->deci_info.deci_x;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_hor;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_hor;
    scl_tap = frame_scaler->scaler_info.scaler_y_hor_tap;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[0];

    slice_scaler->src_size_x = slice_info->slice_width;

    //overlap_head = slice_info->overlap_left > 0 ? slice_info->overlap_left - scaler_overlap->overlap_left : 0;
    //overlap_tail = slice_info->overlap_right > 0 ? slice_info->overlap_right - scaler_overlap->overlap_right : 0;
    overlap_head = scaler_slice_info->start_col - slice_info->start_col;
    overlap_tail = slice_info->end_col - scaler_slice_info->end_col;
    //slice_info->overlap_left  -= overlap_head;
    //slice_info->overlap_right -= overlap_tail;

    start_col_org = slice_info->start_col; //copy for trim0

    slice_info->start_col += overlap_head;
    slice_info->end_col   -= overlap_tail;
    slice_info->slice_width = slice_info->end_col - slice_info->start_col + 1;

    input_slice_start  = slice_info->start_col;
    input_slice_size   = slice_info->slice_width;
    input_pixel_align  = 2; //YUV420
    output_pixel_align = 2; //YUV420
    if(slice_info->sliceColNo != slice_info->sliceCols-1)
    {
        output_pixel_align = frame_scaler->output_align_hor;
    }

    output_slice_size = 0;
    calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_x = 0;
        slice_scaler->trim0_info.trim_size_x = 0;
    }
    else
    {
        if (output_slice_start < ref_ox)
        {
            output_slice_size -= ref_ox - output_slice_start;
            output_slice_start = ref_ox;
        }

        calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
            output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        slice_scaler->trim0_info.trim_start_x = input_slice_start - start_col_org;
        slice_scaler->trim0_info.trim_size_x = input_slice_size;

        slice_scaler->scaler_info.scaler_in_width = input_slice_size/deci;
        slice_scaler->scaler_info.scaler_out_width = output_slice_size;

        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[0] = init_phase;
        calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][0]);  //luma
        calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][1]);  //chroma

        //slice_scaler->trim1_info.trim_start_x = 0;
        //slice_scaler->trim1_info.trim_size_x = output_slice_size;
    }
    slice_scaler->dst_start_x = output_slice_start;
    slice_scaler->dst_size_x = output_slice_size;
    slice_scaler->trim1_info.trim_start_x = 0;
    slice_scaler->trim1_info.trim_size_x = output_slice_size;

    //ver
    trim_start = frame_scaler->trim0_info.trim_start_y;
    trim_size = frame_scaler->trim0_info.trim_size_y;
    deci = frame_scaler->deci_info.deci_y;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_ver;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_ver;
    //FIXME: 420 input
    if(frame_scaler->input_pixfmt == YUV422)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap)   + 2; //huoxing: +2 to avoid using special tap
    else if(frame_scaler->input_pixfmt == YUV420)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap*2) + 2;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[1];

    slice_scaler->src_size_y = slice_info->slice_height;

    //overlap_head = slice_info->overlap_up > 0 ? slice_info->overlap_up - scaler_overlap->overlap_up : 0;
    //overlap_tail = slice_info->overlap_down > 0 ? slice_info->overlap_down - scaler_overlap->overlap_down : 0;
    overlap_head = scaler_slice_info->start_row - slice_info->start_row;
    overlap_tail = slice_info->end_row - scaler_slice_info->end_row;
    //slice_info->overlap_up -= overlap_head;
    //slice_info->overlap_down -= overlap_tail;

    start_row_org = slice_info->start_row; //copy for trim0

    slice_info->start_row += overlap_head;
    slice_info->end_row   -= overlap_tail;
    slice_info->slice_height = slice_info->end_row - slice_info->start_row + 1;

    input_slice_start = slice_info->start_row;
    input_slice_size  = slice_info->slice_height;
    //assert(frame_scaler->input_pixfmt == YUV422);
    input_pixel_align = 2; //YUV420
    if (frame_scaler->output_pixfmt == YUV422)
        output_pixel_align = 2;
    else if (frame_scaler->output_pixfmt == YUV420)
    {
        output_pixel_align = 4;
        if(slice_info->sliceRowNo != slice_info->sliceRows-1)
        {
            output_pixel_align = frame_scaler->output_align_ver;
        }
    }
    else
        assert(0);

    output_slice_size = 0;
    calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_y = 0;
        slice_scaler->trim0_info.trim_size_y = 0;
    }
    else
    {
        if (output_slice_start < ref_oy)
        {
            output_slice_size -= (ref_oy - output_slice_start);
            output_slice_start = ref_oy;
        }

        calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
            output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        slice_scaler->trim0_info.trim_start_y = input_slice_start - start_row_org;
        slice_scaler->trim0_info.trim_size_y = input_slice_size;

        slice_scaler->scaler_info.scaler_in_height = input_slice_size/deci;
        slice_scaler->scaler_info.scaler_out_height = output_slice_size;
        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[1] = init_phase;
        //FIXME: need refer to input_pixfmt
        //luma
        calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][0]);
        //chroma
        if(slice_scaler->scaler_info.input_pixfmt == YUV422)
        {           
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)          
                calc_scaler_phase(init_phase/2, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);           
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)           
                calc_scaler_phase(init_phase,   scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);

        }
        else if(slice_scaler->scaler_info.input_pixfmt == YUV420)
        {
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)
                calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)
                calc_scaler_phase(init_phase/2, scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
        }
        //slice_scaler->trim1_info.trim_start_y = 0;
        //slice_scaler->trim1_info.trim_size_y = output_slice_size;
    }
    slice_scaler->dst_start_y = output_slice_start;
    slice_scaler->dst_size_y  = output_slice_size;
    slice_scaler->trim1_info.trim_start_y = 0;
    slice_scaler->trim1_info.trim_size_y  = output_slice_size;
}
*/

void yuv_scaler_init_slice_info_v3(
    yuvscaler_param_t    *frame_scaler, 
    yuvscaler_param_t    *slice_scaler, 
    scaler_slice_t       *slice_info,
    const scaler_slice_t *input_slice_info,
    const scaler_slice_t *output_slice_info)
{
    int trim_start, trim_size;
    int deci;
    int scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase;
    int input_slice_start, input_slice_size, input_pixel_align;
    int output_slice_start, output_slice_size, output_pixel_align;
    int overlap_head, overlap_tail;
    int start_col_org, start_row_org;

    memcpy(slice_scaler, frame_scaler, sizeof(yuvscaler_param_t));

    //hor
    trim_start = frame_scaler->trim0_info.trim_start_x;
    trim_size = frame_scaler->trim0_info.trim_size_x;
    deci = frame_scaler->deci_info.deci_x;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_hor;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_hor;
    scl_tap = frame_scaler->scaler_info.scaler_y_hor_tap;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[0];

    slice_scaler->src_size_x = slice_info->slice_width;

    //overlap_head = slice_info->overlap_left > 0 ? slice_info->overlap_left - scaler_overlap->overlap_left : 0;
    //overlap_tail = slice_info->overlap_right > 0 ? slice_info->overlap_right - scaler_overlap->overlap_right : 0;
    overlap_head = input_slice_info->start_col - slice_info->start_col;
    overlap_tail = slice_info->end_col - input_slice_info->end_col;
    //slice_info->overlap_left  -= overlap_head;
    //slice_info->overlap_right -= overlap_tail;

    start_col_org = slice_info->start_col; //copy for trim0

    slice_info->start_col += overlap_head;
    slice_info->end_col   -= overlap_tail;
    slice_info->slice_width = slice_info->end_col - slice_info->start_col + 1;

    input_slice_start  = slice_info->start_col;
    input_slice_size   = slice_info->slice_width;
    input_pixel_align  = 2; //YUV420
    output_pixel_align = 2; //YUV420
    if(slice_info->sliceColNo != slice_info->sliceCols-1)
    {
        output_pixel_align = frame_scaler->output_align_hor;
    }

    output_slice_start = output_slice_info->start_col;
    output_slice_size  = output_slice_info->end_col - output_slice_info->start_col + 1;

    //     output_slice_size = 0;
    //     calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
    //         input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_x = 0;
        slice_scaler->trim0_info.trim_size_x = 0;
    }
    else
    {
        int input_slice_start_deci;
        int input_slice_end_deci;
        //        output_slice_start = output_slice_info->start_col;
        //         if (output_slice_start < ref_ox)
        //         {
        //             output_slice_size -= ref_ox - output_slice_start;
        //             output_slice_start = ref_ox;
        //         }
        // 
        //         calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        //             output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        input_slice_start = input_slice_info->start_col;
        input_slice_size  = input_slice_info->end_col - input_slice_info->start_col + 1;
        //init_phase        = slice_info->init_phase_hor;

        //deci
        slice_scaler->scaler_info.scaler_in_width  = input_slice_size/deci;

        //trim
        input_slice_start_deci = input_slice_info->start_col/deci;
        input_slice_end_deci   = input_slice_info->end_col/deci;
        if(frame_scaler->trim0_info.trim_en)
        {
            slice_scaler->trim0_info.trim_start_x = (input_slice_info->start_col - start_col_org)/deci;       
            slice_scaler->trim0_info.trim_size_x  = input_slice_end_deci - input_slice_start_deci + 1;
            slice_scaler->scaler_info.scaler_in_width  = slice_scaler->trim0_info.trim_size_x;
        }

        
        slice_scaler->scaler_info.scaler_out_width = output_slice_size;

        init_phase = slice_info->init_phase_hor;
        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[0] = init_phase;
        calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][0]);  //luma
        calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][1]);  //chroma

        //slice_scaler->trim1_info.trim_start_x = 0;
        //slice_scaler->trim1_info.trim_size_x = output_slice_size;
    }
    slice_scaler->dst_start_x = output_slice_start;
    slice_scaler->dst_size_x  = output_slice_size;
    slice_scaler->trim1_info.trim_start_x = 0;
    slice_scaler->trim1_info.trim_size_x  = output_slice_size;

    //ver
    trim_start = frame_scaler->trim0_info.trim_start_y;
    trim_size = frame_scaler->trim0_info.trim_size_y;
    deci = frame_scaler->deci_info.deci_y;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_ver;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_ver;
    //FIXME: 420 input
    if(frame_scaler->input_pixfmt == YUV422)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap)   + 2; //huoxing: +2 to avoid using special tap
    else if(frame_scaler->input_pixfmt == YUV420)
        scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap*2) + 2;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[1];

    slice_scaler->src_size_y = slice_info->slice_height;

    //overlap_head = slice_info->overlap_up > 0 ? slice_info->overlap_up - scaler_overlap->overlap_up : 0;
    //overlap_tail = slice_info->overlap_down > 0 ? slice_info->overlap_down - scaler_overlap->overlap_down : 0;
    overlap_head = input_slice_info->start_row - slice_info->start_row;
    overlap_tail = slice_info->end_row - input_slice_info->end_row;
    //slice_info->overlap_up -= overlap_head;
    //slice_info->overlap_down -= overlap_tail;

    start_row_org = slice_info->start_row; //copy for trim0

    slice_info->start_row += overlap_head;
    slice_info->end_row   -= overlap_tail;
    slice_info->slice_height = slice_info->end_row - slice_info->start_row + 1;

    input_slice_start = slice_info->start_row;
    input_slice_size  = slice_info->slice_height;
    //assert(frame_scaler->input_pixfmt == YUV422);
    input_pixel_align = 2; //YUV420
    if (frame_scaler->output_pixfmt == YUV422)
        output_pixel_align = 2;
    else if (frame_scaler->output_pixfmt == YUV420)
    {
        output_pixel_align = 4;
        if(slice_info->sliceRowNo != slice_info->sliceRows-1)
        {
            output_pixel_align = frame_scaler->output_align_ver;
        }
    }
    else
        assert(0);

    output_slice_start = output_slice_info->start_row;
    output_slice_size  = output_slice_info->end_row - output_slice_info->start_row + 1;
    //     output_slice_size = 0;
    //     calc_scaler_output_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
    //         input_slice_start, input_slice_size, output_pixel_align, &output_slice_start, &output_slice_size);

    if(output_slice_size == 0)
    {
        slice_scaler->trim0_info.trim_start_y = 0;
        slice_scaler->trim0_info.trim_size_y = 0;
    }
    else
    {
        int input_slice_start_deci;
        int input_slice_end_deci;
        //         if (output_slice_start < ref_oy)
        //         {
        //             output_slice_size -= (ref_oy - output_slice_start);
        //             output_slice_start = ref_oy;
        //         }
        // 
        //         calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        //             output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

        input_slice_start = input_slice_info->start_row;
        input_slice_size  = input_slice_info->end_row - input_slice_info->start_row + 1;
        //init_phase        = slice_info->init_phase_ver;

        slice_scaler->scaler_info.scaler_in_height  = input_slice_size/deci;

        //slice_scaler->trim0_info.trim_start_y = input_slice_start - start_row_org;
        //slice_scaler->trim0_info.trim_size_y  = input_slice_size;
        input_slice_start_deci = input_slice_info->start_row/deci;
        input_slice_end_deci   = input_slice_info->end_row/deci;
        if(frame_scaler->trim0_info.trim_en)
        {
            slice_scaler->trim0_info.trim_start_y = (input_slice_info->start_row - start_row_org)/deci;
            slice_scaler->trim0_info.trim_size_y  = input_slice_end_deci - input_slice_start_deci + 1;
            slice_scaler->scaler_info.scaler_in_height  = slice_scaler->trim0_info.trim_size_y;
        }

        slice_scaler->scaler_info.scaler_out_height = output_slice_size;

        init_phase = slice_info->init_phase_ver;
        slice_scaler->scaler_info.init_phase_info.scaler_init_phase[1] = init_phase;
        //FIXME: need refer to input_pixfmt
        //luma
        calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][0]);
        //chroma
        if(slice_scaler->scaler_info.input_pixfmt == YUV422)
        {           
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)          
                calc_scaler_phase(init_phase/2, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);           
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)           
                calc_scaler_phase(init_phase,   scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);

        }
        else if(slice_scaler->scaler_info.input_pixfmt == YUV420)
        {
            if(slice_scaler->scaler_info.output_pixfmt == YUV420)
                calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
            else if(slice_scaler->scaler_info.output_pixfmt == YUV422)
                calc_scaler_phase(init_phase/2, scl_factor_out,   &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
        }
        //slice_scaler->trim1_info.trim_start_y = 0;
        //slice_scaler->trim1_info.trim_size_y = output_slice_size;
    }
    slice_scaler->dst_start_y = output_slice_start;
    slice_scaler->dst_size_y  = output_slice_size;
    slice_scaler->trim1_info.trim_start_y = 0;
    slice_scaler->trim1_info.trim_size_y  = output_slice_size;
}

/*
//added for 3dnr scaler
void yuv_scaler_get_input_slice(yuvscaler_param_t *slice_scaler, const yuvscaler_param_t *frame_scaler, const scaler_slice_t *slice_info)
{
    int trim_start, trim_size;
    int deci;
    int scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase;
    int input_slice_start, input_slice_size, input_pixel_align;
    int output_slice_start, output_slice_size;

    assert(frame_scaler->input_pixfmt == YUV422 && frame_scaler->output_pixfmt == YUV422);
    memcpy(slice_scaler, frame_scaler, sizeof(yuvscaler_param_t));
    slice_scaler->trim0_info.trim_en = 1;

    //hor
    trim_start = frame_scaler->trim0_info.trim_start_x;
    trim_size = frame_scaler->trim0_info.trim_size_x;
    deci = frame_scaler->deci_info.deci_x;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_hor;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_hor;
    scl_tap = frame_scaler->scaler_info.scaler_y_hor_tap;
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[0];
    output_slice_start= slice_info->start_col;
    output_slice_size = slice_info->slice_width;
    input_pixel_align = 2;

    calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

    slice_scaler->trim0_info.trim_start_x = input_slice_start;
    slice_scaler->trim0_info.trim_size_x = input_slice_size;
    slice_scaler->scaler_info.scaler_in_width = input_slice_size/deci;
    slice_scaler->scaler_info.scaler_out_width = output_slice_size;
    slice_scaler->scaler_info.init_phase_info.scaler_init_phase[0] = init_phase;
    calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][0]);  //luma
    calc_scaler_phase(init_phase/4, scl_factor_out/2, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[0][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[0][1]);  //chroma
    slice_scaler->trim1_info.trim_start_x = 0;
    slice_scaler->trim1_info.trim_size_x = output_slice_size;
    slice_scaler->dst_size_x = output_slice_size;

    //ver
    trim_start = frame_scaler->trim0_info.trim_start_y;
    trim_size = frame_scaler->trim0_info.trim_size_y;
    deci = frame_scaler->deci_info.deci_y;
    scl_en = frame_scaler->scaler_info.scaler_en;
    scl_factor_in = frame_scaler->scaler_info.scaler_factor_in_ver;
    scl_factor_out = frame_scaler->scaler_info.scaler_factor_out_ver;
    scl_tap = MAX(frame_scaler->scaler_info.scaler_y_ver_tap, frame_scaler->scaler_info.scaler_uv_ver_tap) + 2; //huoxing: +2 to avoid using special tap
    init_phase = frame_scaler->scaler_info.init_phase_info.scaler_init_phase[1];
    output_slice_start= slice_info->start_row;
    output_slice_size = slice_info->slice_height;
    input_pixel_align = 1;

    calc_scaler_input_slice_info(trim_start, trim_size, deci, scl_en, scl_factor_in, scl_factor_out, scl_tap, init_phase,
        output_slice_start, output_slice_size, input_pixel_align, &input_slice_start, &input_slice_size, &init_phase);

    slice_scaler->trim0_info.trim_start_y = input_slice_start;
    slice_scaler->trim0_info.trim_size_y = input_slice_size;
    slice_scaler->scaler_info.scaler_in_height = input_slice_size/deci;
    slice_scaler->scaler_info.scaler_out_height = output_slice_size;
    slice_scaler->scaler_info.init_phase_info.scaler_init_phase[1] = init_phase;
    calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][0], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][0]);
    calc_scaler_phase(init_phase, scl_factor_out, &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_int[1][1], &slice_scaler->scaler_info.init_phase_info.scaler_init_phase_rmd[1][1]);
    slice_scaler->trim1_info.trim_start_y = 0;
    slice_scaler->trim1_info.trim_size_y = output_slice_size;
    slice_scaler->dst_size_y = output_slice_size;
}
*/

void calc_trim0_info(uint16 trim_start,
    uint16 trim_size,
    uint16 overlap_bad_l_u,
    uint16 overlap_bad_r_d,
    uint16 start,
    uint16 end,
    uint16* slice_trim_start,
    uint16* slice_trim_size)
{
    uint16 start_temp, trim_size_temp;
    uint16 trim_end;

    start_temp = 0;
    trim_size_temp = 0;
    trim_end = trim_start + trim_size;

    if(trim_start > (start + overlap_bad_l_u))
    {
        start_temp = trim_start - start;
        if(trim_end < (end+1 - overlap_bad_r_d))
            trim_size_temp =trim_size;
        else
            trim_size_temp = (end+1 - overlap_bad_r_d) - trim_start;
    }
    else
    {
        start_temp = overlap_bad_l_u;
        if(trim_end < (end+1 - overlap_bad_r_d))
            trim_size_temp =trim_end - (start + overlap_bad_l_u);
        else
            trim_size_temp = (end-start)+1 - (overlap_bad_l_u + overlap_bad_r_d);
    }

    *slice_trim_start = start_temp;
    * slice_trim_size = trim_size_temp;
}

void deci_adjust_func(uint16 trim_start,
    uint16 trim_size,
    uint16 trim_start_adjusted,
    uint16 deci_align,
    uint16* trim_start_aligned,
    uint16* trim_size_aligned)
{
    uint16 trim_start_temp, trim_size_temp;

    trim_start_temp = 0;
    trim_size_temp = 0;

    trim_start_temp = trim_start + trim_start_adjusted;
    trim_size_temp = trim_size - trim_start_adjusted;
    trim_size_temp = trim_size_temp/deci_align*deci_align;

    *trim_start_aligned = trim_start_temp;
    *trim_size_aligned = trim_size_temp;
}

// coef_data_ptr -->> out_filter
void GetFilter(int16 pos_start,         //position
    int16 *coef_data_ptr,   //ptr== pointer
    int16 *out_filter,      // OUT
    int16 iI_hor,           //integer Interpolation  horizontal
    int16 coef_len,
    int16 *filter_len)      // OUT
{
    int16 i;
    for (i = 0; i < iI_hor; i++)
    {
        int16 len = 0;
        int16 j;
        int16 pos = pos_start + i;
        while (pos >= iI_hor)
        {
            pos -= iI_hor;
        }
        for (j = 0; j < coef_len; j+=iI_hor)
        {
            *out_filter++ = coef_data_ptr[j + pos];
            len ++;
        }
        *filter_len++ = len;
    }
}


static void normalize_inter(
    double *data,       ///< [in]source
    int16 *int_data,    ///< [out]normalized data
    uint8 ilen          //count
    )
{
    uint8   it;
    double  tmp_sum_val, d_inverse, tmp_d, *tmp_data;

    tmp_data = data;
    tmp_sum_val = 0;

    for (it = 0; it < ilen; it++)
    {
        tmp_sum_val += tmp_data[it];
    }
    d_inverse = 1 / tmp_sum_val;

    for (it = 0; it < ilen; it++)
    {
        tmp_d =  (tmp_data[it] * d_inverse * 256);
        int_data[it] = (int16) tmp_d;
    }
}

/* ------------------------------------------  */
static int16 sum_fun(int16 *data, int8 ilen)
{
    int16   tmp_sum ;
    int8    i       ;
    tmp_sum = 0;

    for (i = 0; i < ilen; i++)
        tmp_sum += *data++;

    return tmp_sum;
}

/// push down the peak when the sum is over 256.
static void adjust_filter_inter(int16 *filter, uint8 ilen)
{
    uint8   i, midi     ;
    int     tmpi, tmp_S ;
    int     tmp_val = 0 ;

    tmpi = sum_fun(filter, ilen) - 256;
    midi = ilen >> 1;
    SIGN2(tmp_val, tmpi);

    if ((tmpi & 1) == 1)  // tmpi is odd
    {
        filter[midi] = filter[midi] - tmp_val;
        tmpi -= tmp_val;
    }

    tmp_S = abs(tmpi>>1);

    if ((ilen & 1) == 1)  // ilen is odd
    {
        for (i = 0; i < tmp_S; i++)
        {
            filter[midi - (i+1)] = filter[midi - (i+1)] - tmp_val;
            filter[midi + (i+1)] = filter[midi + (i+1)] - tmp_val;
        }
    }
    else  // ilen is even
    {
        for (i = 0; i < tmp_S; i++)
        {
            filter[midi - (i+1)] = filter[midi - (i+1)] - tmp_val;
            filter[midi + i] = filter[midi + i] - tmp_val;
        }
    }

    if(filter[midi] > 255)
    {
        tmp_val = filter[midi] ;
        filter[midi] = 255 ;
        filter[midi - 1] = filter[midi - 1] + tmp_val - 255 ;
    }
}

void   WriteScalarCoef(int16 scale_coef[8][32],
    int16 *coef_ptr,
    int16 len)  //tap
{
    int i, j;

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < len; j++)
        {
            scale_coef[i][j] = *(coef_ptr + i*len + len - 1 - j);
        }
    }
}

void     yuv_scaler_gen_scaler_coef(int16   i_w,                                   // file write deleted   xing.huo  2015.5.13
    int16   i_h,
    int16   o_w,
    int16   o_h,
    uint8   i_pixfmt, //
    uint8   o_pixfmt,
    uint8   *luma_tap_hor,  //out
    uint8   *chroma_tap_hor,        //out
    uint8   *luma_tap_ver,  //out
    uint8   *chroma_tap_ver,        //out
    scaler_coef_info_t *scaler_coef
    )
{
    int16   iD_hor = i_w, D_hor_bak = i_w;    // decimition at horizontal
    int16   iI_hor = o_w, I_hor_bak = o_w;    //interpolation at horizaontal

    int16   iD_ver = i_h, D_ver_bak = i_h;          // decimition at vertical
    int16   iI_ver = o_h, I_ver_bak = o_h;          //interpolation at vertical
    int16   D_ver_bak_uv = i_h, I_ver_bak_uv = o_h;

    int16   cong_Ycom_hor[8][32]  = {{0}};
    //int16   cong_Ycom_hor_pos[8]  = {0};
    int16   cong_UVcom_hor[8][32] = {{0}};
    //int16 cong_UVcom_hor_pos[8] = {0};
    int16   cong_Ycom_ver[9][32]  = {{0}};
    //int16 cong_Ycom_ver_pos[8]  = {0};
    int16   cong_UVcom_ver[9][32] = {{0}};
    //int16 cong_UVcom_ver_pos[8] = {0};

    int16   y_coef_data_x[COUNT]  = {0};
    int16   uv_coef_data_x[COUNT] = {0};
    int16   y_coef_data_y[COUNT]  = {0};
    int16   uv_coef_data_y[COUNT] = {0};

    int16   coef_len, i, j;
    int16   Y_hor_filter[COUNT]     = {0},  Y_hor_filter_len[100]  = {0};
    int16   UV_hor_filter[COUNT]    = {0},  UV_hor_filter_len[100] = {0};
    int16   Y_ver_filter[COUNT]     = {0},  Y_ver_filter_len[100]  = {0};
    int16   UV_ver_filter[COUNT]    = {0},  UV_ver_filter_len[100] = {0};

    int16   pos_start   ;
    int16   tmpi, tmp_S ;
    int16   tmpval      ;

    //pc
    uint16  luma_ver_tap,chroma_ver_tap;
    uint16  luma_ver_maxtap=8,chroma_ver_maxtap = 8;

    float fD_hor = iD_hor;
    float fI_hor = iI_hor;

    float fD_ver = iD_ver;
    float fI_ver = iI_ver;

    fD_hor = fD_hor * 8 / fI_hor;
    fI_hor = 8;
    iI_hor = (int16) fI_hor;
    iD_hor = (int16) (fD_hor + 0.9);


    /* horizontal direction *//////////////////////////////////////////////
    /* Y component */
    coef_len = CalY_ScalingCoef(8, D_hor_bak, I_hor_bak, y_coef_data_x, 1);

    pos_start = coef_len / 2;
    while (pos_start >= iI_hor)
    {
        pos_start -= iI_hor;
    }

    GetFilter(pos_start, y_coef_data_x, Y_hor_filter, iI_hor, coef_len, Y_hor_filter_len);

    WriteScalarCoef(cong_Ycom_hor, Y_hor_filter, 8);

    for (i = 0; i < 8; i++)         //8 phase
    {
        for (j = 0; j < 8; j++)     //8 tap
        {
            tmpval = cong_Ycom_hor[i][j];

            if(tmpval > 255)        //adjust
            {
                tmpi = tmpval - 255 ;
                cong_Ycom_hor[i][j] = 255 ;
                tmp_S = abs(tmpi);
                if ((tmp_S & 1) == 1)  // ilen is odd
                {
                    cong_Ycom_hor[i][j + 1] = cong_Ycom_hor[i][j + 1] + (tmpi + 1) / 2 ;
                    cong_Ycom_hor[i][j - 1] = cong_Ycom_hor[i][j - 1] + (tmpi - 1) / 2 ;
                }
                else  // ilen is even
                {
                    cong_Ycom_hor[i][j + 1] = cong_Ycom_hor[i][j + 1] + (tmpi ) / 2 ;
                    cong_Ycom_hor[i][j - 1] = cong_Ycom_hor[i][j - 1] + (tmpi ) / 2 ;
                }
            }
        }
    }

    /* UV component */
    coef_len = CalUV_ScalingCoef(4, D_hor_bak, I_hor_bak, uv_coef_data_x, 1);

    pos_start = coef_len / 2;
    while (pos_start >= iI_hor)
    {
        pos_start -= iI_hor;
    }

    GetFilter(pos_start, uv_coef_data_x, UV_hor_filter, iI_hor, coef_len, UV_hor_filter_len);

    WriteScalarCoef(cong_UVcom_hor, UV_hor_filter, 4);

    for (i = 0; i < 8; i++)         //8 phase
    {
        for (j = 0; j < 4; j++)     //4 tap
        {
            tmpval = cong_UVcom_hor[i][j];

            if(tmpval > 255)        //adjust
            {
                tmpi = tmpval - 255 ;
                cong_UVcom_hor[i][j] = 255 ;
                tmp_S = abs(tmpi);
                if ((tmp_S & 1) == 1)  // ilen is odd
                {
                    cong_UVcom_hor[i][j + 1] = cong_UVcom_hor[i][j + 1] + (tmpi + 1) / 2 ;
                    cong_UVcom_hor[i][j - 1] = cong_UVcom_hor[i][j - 1] + (tmpi - 1) / 2 ;
                }
                else
                {
                    cong_UVcom_hor[i][j + 1] = cong_UVcom_hor[i][j + 1] + (tmpi ) / 2 ;
                    cong_UVcom_hor[i][j - 1] = cong_UVcom_hor[i][j - 1] + (tmpi ) / 2 ;
                }
            }
        }
    }

    *luma_tap_hor = 8;
    *chroma_tap_hor = 4;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    ///------------vertical scaling coef-------------

    luma_ver_tap   = ((uint8)(iD_ver / iI_ver)) * 2;
    chroma_ver_tap = ((uint8)(iD_ver / iI_ver)) * 2;
    if(i_pixfmt == YUV420 && o_pixfmt == YUV422)
    {
        chroma_ver_tap = ((uint8)(iD_ver/2 / iI_ver)) * 2;
    }

    if(luma_ver_tap > luma_ver_maxtap)  luma_ver_tap = luma_ver_maxtap;     //modified by Hongbo,max_tap 8-->16
    if(luma_ver_tap <= 2) luma_ver_tap = 4 ;
    I_ver_bak_uv = I_ver_bak;

    *luma_tap_ver = (uint8) luma_ver_tap ;

    fD_ver = iD_ver ;
    fI_ver = iI_ver ;
    fD_ver = fD_ver * 8 / fI_ver;
    fI_ver = 8      ;
    iI_ver = (int16) fI_ver;
    iD_ver = (int16) (fD_ver + 0.9);


    /* vertical direction */
    /* Y component */
    coef_len = CalY_ScalingCoef(luma_ver_tap, D_ver_bak, I_ver_bak, y_coef_data_y, 0);

    pos_start = coef_len / 2;
    while (pos_start >= iI_ver)
    {
        pos_start -= iI_ver;
    }
    GetFilter(pos_start, y_coef_data_y, Y_ver_filter, iI_ver, coef_len, Y_ver_filter_len);

    WriteScalarCoef(cong_Ycom_ver, Y_ver_filter, Y_ver_filter_len[0]);

    for (i = 0; i < 8; i++)             //8 phase
    {
        for (j = 0; j < luma_ver_tap; j++)      // variable tap
        {
            tmpval = cong_Ycom_ver[i][j];

            if(tmpval > 255)
            {
                tmpi = tmpval - 255 ;
                cong_Ycom_ver[i][j] = 255 ;
                tmp_S = abs(tmpi);
                if ((tmp_S & 1) == 1)  // ilen is odd
                {
                    cong_Ycom_ver[i][j + 1] = cong_Ycom_ver[i][j + 1] + (tmpi + 1) / 2 ;
                    cong_Ycom_ver[i][j - 1] = cong_Ycom_ver[i][j - 1] + (tmpi - 1) / 2 ;
                }
                else  // ilen is even
                {
                    cong_Ycom_ver[i][j + 1] = cong_Ycom_ver[i][j + 1] + (tmpi ) / 2 ;
                    cong_Ycom_ver[i][j - 1] = cong_Ycom_ver[i][j - 1] + (tmpi ) / 2 ;
                }
            }
        }
    }

    /* UV component */
    //chroma_ver_tap = ((uint8)(iD_ver / iI_ver)) * 2;
    if(i_pixfmt == YUV422 && o_pixfmt == YUV420)
    {
        I_ver_bak_uv /= 2;
        chroma_ver_tap *=2;
        chroma_ver_maxtap = 16;
    }
    if(chroma_ver_tap > chroma_ver_maxtap)  chroma_ver_tap = chroma_ver_maxtap;
    if(chroma_ver_tap <= 2) chroma_ver_tap = 4 ;

    *chroma_tap_ver = (uint8) chroma_ver_tap;

    if(YUV420 == i_pixfmt)
        D_ver_bak_uv /= 2;

    //if(1 == scaling2yuv420)
    //{
    //  fD_ver = D_ver_bak_uv   ;
    //  fI_ver = I_ver_bak_uv;
    //  fD_ver = fD_ver * 8 / fI_ver;
    //  fI_ver = 8      ;
    //  iI_ver = (int16) fI_ver;
    //  iD_ver = (int16) (fD_ver + 0.9);
    //}

    coef_len = CalUV_ScalingCoef((int16)(chroma_ver_tap), D_ver_bak_uv, I_ver_bak_uv, uv_coef_data_y, 0);
    pos_start = coef_len / 2;
    while (pos_start >= iI_ver)
    {
        pos_start -= iI_ver;
    }

    GetFilter(pos_start, uv_coef_data_y, UV_ver_filter, iI_ver, coef_len, UV_ver_filter_len);

    WriteScalarCoef(cong_UVcom_ver, UV_ver_filter, UV_ver_filter_len[0]);

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < chroma_ver_tap; j++)
        {
            tmpval = cong_UVcom_ver[i][j];

            if(tmpval > 255)
            {
                tmpi = tmpval - 255 ;
                cong_UVcom_ver[i][j] = 255 ;
                tmp_S = abs(tmpi);
                if ((tmp_S & 1) == 1)  // ilen is odd
                {
                    cong_UVcom_ver[i][j + 1] = cong_UVcom_ver[i][j + 1] + (tmpi + 1) / 2 ;
                    cong_UVcom_ver[i][j - 1] = cong_UVcom_ver[i][j - 1] + (tmpi - 1) / 2 ;
                }
                else  // ilen is even
                {
                    cong_UVcom_ver[i][j + 1] = cong_UVcom_ver[i][j + 1] + (tmpi ) / 2 ;
                    cong_UVcom_ver[i][j - 1] = cong_UVcom_ver[i][j - 1] + (tmpi ) / 2 ;
                }
            }
        }
    }

    // edge processing //
    // Y vertical .............. //
    if(1/*2*I_ver_bak <= D_ver_bak*/)
        //// scaling down and ratio<=1/2 using scaling down scaler,
        //// otherwise use scaling up scaler,
        //// scaling up doesn't use this special edge phase
    {
        int32     phase_temp[9]     ;
        int32     acc, i_sample_cnt ;
        int32     l                 ;
        uint8     phase, spec_tap   ;

        for (j = 0; j <= 8; j++)
            phase_temp[j] = j * I_ver_bak / 8;

        acc = 0         ;
        i_sample_cnt = 0;
        phase = 0       ;

        for(i = 0; i < I_ver_bak; i++)
        {
            spec_tap = i % 2;
            while (acc >= I_ver_bak)
            {
                acc -= I_ver_bak;
                i_sample_cnt++;
            }
            for (j = 0; j < 8; j++)
            {
                if (acc >= phase_temp[j] && acc < phase_temp[j + 1])
                {
                    phase = (uint8) j;
                    break;
                }
            }

            for (j = 1 - luma_ver_tap/2; j <= luma_ver_tap/2; j++)
            {
                l = i_sample_cnt + j;
                if(l <= 0)
                {
                    cong_Ycom_ver[8][spec_tap] += cong_Ycom_ver[phase][j+luma_ver_tap/2-1];
                }
                else
                {
                    if(l >= D_ver_bak - 1)
                    {
                        cong_Ycom_ver[8][spec_tap + 2] += cong_Ycom_ver[phase][j+luma_ver_tap/2-1];
                    }
                }
            }
            acc += D_ver_bak;
        }
    }


    // UV vertical ................ //
    if(1/*(2*I_ver_bak_uv <= D_ver_bak_uv)*/)
        // scaling down and ratio<=1/2 using scaling down scaler,
        //otherwise use scaling up scaler,
        //scaling up doesn't use this special edge phase
    {
        int32     phase_temp[9]     ;
        int32     acc, i_sample_cnt ;
        int32     l                 ;
        uint8     phase, spec_tap   ;

        for (j = 0; j <= 8; j++)
            phase_temp[j] = j * I_ver_bak_uv / 8;

        acc = 0         ;
        i_sample_cnt = 0;
        phase = 0       ;

        for(i = 0; i < I_ver_bak_uv; i++)
        {
            spec_tap = i % 2;
            while (acc >= I_ver_bak_uv)
            {
                acc -= I_ver_bak_uv;
                i_sample_cnt++;
            }
            for (j = 0; j < 8; j++)
            {
                if (acc >= phase_temp[j] && acc < phase_temp[j + 1])
                {
                    phase = (uint8) j;
                    break;
                }
            }

            for (j = 1 - chroma_ver_tap/2; j <= chroma_ver_tap/2; j++) // hongbo,2012.8.13
            {
                l = i_sample_cnt + j;
                if(l <= 0)
                {
                    cong_UVcom_ver[8][spec_tap] += cong_UVcom_ver[phase][j+chroma_ver_tap/2-1];
                }
                else
                {
                    if(l >= D_ver_bak - 1)
                    {
                        cong_UVcom_ver[8][spec_tap + 2] += cong_UVcom_ver[phase][j+chroma_ver_tap/2-1];
                    }
                }
            }
            acc += D_ver_bak_uv;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //.............out ...............//

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            scaler_coef->y_hor_coef[i][j] = cong_Ycom_hor[i][j];
        }
    }

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            scaler_coef->c_hor_coef[i][j] = cong_UVcom_hor[i][j];
        }
    }

    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 16 ; j++)
        {
            //          scaler_coef->y_ver_down_coef[i][j] = cong_Ycom_ver[i][j];
            scaler_coef->y_ver_coef[i][j] = cong_Ycom_ver[i][j];
        }
    }


    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 16; j++)
        {
            //          scaler_coef->c_ver_down_coef[i][j] = cong_UVcom_ver[i][j];
            scaler_coef->c_ver_coef[i][j] = cong_UVcom_ver[i][j];
        }
    }

}


/* cal Y model */
int16 CalY_ScalingCoef(int16    tap ,       /*lint !e578 */
    float   D   ,       // src
    float   I   ,       // dst
    int16   *y_coef_data_ptr,
    int16   dir         //1:x; other:y
    )
{
    const int16 phase = 8;
    int16 coef_lenght;
    /*coef_lenght = (int16) (tap * phase);
    if (D > I)  // scaling down
    {
    CalYmodelCoef(coef_lenght, y_coef_data_ptr, I, D);
    }
    else   // scaling up
    {
    CalYmodelCoef(coef_lenght, y_coef_data_ptr, I, D);
    }*/
    if(1 == dir) // x direction
    {
        coef_lenght = (int16) (tap * phase);
        CalYmodelCoef(coef_lenght, y_coef_data_ptr, I, D);
    }
    else
    {
        if(D > I)   //down
        {
            coef_lenght = (int16) (tap * phase);
            CalYmodelCoef(coef_lenght, y_coef_data_ptr, I, D);
        }
        else        //up
        {
            coef_lenght = (int16) (4 * phase);
            CalYmodelCoef(coef_lenght, y_coef_data_ptr, I, D);
        }
    }

    return coef_lenght;
}

/* cal UV model */
int16 CalUV_ScalingCoef(int16   tap ,       /*lint !e578 */
    float   D   ,
    float   I   ,
    int16   *uv_coef_data_ptr,
    int16   dir
    )
{

    int16   uv_coef_lenght;

    if (dir == 1)  // x direction
    {
        uv_coef_lenght = (int16) (tap * 8);
        CalYmodelCoef(uv_coef_lenght, uv_coef_data_ptr, I, D);
    }
    else  // y direction
    {
        if (D > I)
            uv_coef_lenght = (int16) (tap * 8);
        else
            uv_coef_lenght = (int16) (4 * 8);
        CalYmodelCoef(uv_coef_lenght, uv_coef_data_ptr, I, D);
    }

    return uv_coef_lenght;
}



int16 CalYmodelCoef(int16 coef_length,      /*lint !e578 */
    int16 *coef_data_ptr,   //out
    float N,                //out size for scaling
    float M                 //in size for scaling
    )
{
    int8    mount;
    int16   i, mid_i, kk, j, sum_val;
    double  d_filter[COUNT] = {0};
    //double    delta = coef_length/5;

    mid_i           = coef_length >> 1;

    //generate coef
    d_filter[mid_i] = N/MAX(M,N);
    for (i = 0; i < mid_i; i++)
    {
        d_filter[mid_i + i + 1]   =
            sin(PI / MAX(M,N) * (i + 1)*N/8)
            / (M * sin(PI / (M * N) * (i + 1)*N/8));
        d_filter[mid_i - (i + 1)] =
            d_filter[mid_i + i + 1];
    }
    //add window : to anti-aliasing?
    for (i = -1; i < mid_i; i++)
    {

        //d_filter[mid + i + 1] *= (exp(-(i+1)*(i+1)/(2*delta*delta))/sqrt(2*pi*delta*delta)); //add gaussian window at spatial to reduce ringing
        //d_filter[mid + i + 1] *= (1 - cos(2*pi*(mid-i-1)/coef_lenght)); //add hanning window
        d_filter[mid_i + i + 1] *= (0.54 - 0.46 * cos(2*PI*(mid_i-i-1)/coef_length)); //add hamming window
        //d_filter[mid + i + 1] *= (0.42 - 0.5 * cos(2*pi*(mid-i-1)/coef_lenght) + 0.08 * cos(4*pi*(mid-i-1)/coef_lenght)); //add blackman window
        //d_filter[mid_i + i + 1] *= (0.36 - 0.49 * cos(2*pi*(mid_i-i-1)/coef_length) + 0.14 * cos(4*pi*(mid_i-i-1)/coef_length) - 0.013 * cos(6*pi*(mid_i-i-1)/coef_length));
        d_filter[mid_i - (i + 1)] =
            d_filter[mid_i + i + 1];
    }
    // normalize and adjust
    for(i = 0; i < 8; i++)
    {
        double d_tmp_filter[COUNT] = {0};
        int16  normalized_filter[COUNT]   = {0};

        mount = 0;
        for (j = i; j < coef_length; j+= 8)
        {
            d_tmp_filter[mount] = d_filter[j];
            mount++;
        }

        normalize_inter(d_tmp_filter, normalized_filter, (int8)mount);
        sum_val = sum_fun(normalized_filter, mount);
        if (256 != sum_val)
        {
            adjust_filter_inter(normalized_filter, mount);
        }

        mount = 0 ;
        for(kk = i; kk < coef_length; kk += 8)
        {
            coef_data_ptr[kk] = normalized_filter[mount];
            mount++;
        }
    }

    return 0;
}


// for Y_U_V
void convertYUV422ToYUV420(
    uint8   *pYUV422Data,
    uint8   **ppYUV420Data,
    uint16  iWidth,
    uint16  iHeight
    )
{
    uint16 i;
    uint8  *pConvertedData=NULL;
    uint16 width = iWidth, height = iHeight;

    assert(pYUV422Data);
    assert(ppYUV420Data);
    assert(width%2 ==0 && height%2 == 0);

    pConvertedData = (uint8*)malloc(width*height*3/2);

    //copy y
    memcpy(pConvertedData,pYUV422Data,width*height);

    for(i=0; i<height/2; i++)
    {
        //copy u
        memcpy(pConvertedData + width*height + i* width/2,
            pYUV422Data + width*height + (i*2)* width/2,
            width/2);

        //copy v
        memcpy(pConvertedData + width*height + (width/2)*(height/2)+ i* width/2,
            pYUV422Data + width*height + (width/2)*(height) + (i*2)* width/2,
            width/2);
    }

    if(ppYUV420Data)
        *ppYUV420Data = pConvertedData;
    else
        SAFE_FREE(pConvertedData);
}

void convertYUV422PToYUV422SP(
    uint8   *pYUV422Data,
    uint16  width,
    uint16  height
    )
{
    uint16 i,j;
    uint8  *pTempData=NULL;
    assert(pYUV422Data);
    assert(width%2 ==0);

    pTempData = (uint8*)malloc(width*height);

    for(i=0; i<height; i++)
    {
        for(j=0; j<width/2; j++)
        {
            //copy u
            pTempData[i*width + 2*j + 0] = pYUV422Data[width*height + i*width/2 + j];

            //copy v
            pTempData[i*width + 2*j + 1] = pYUV422Data[width*height + (width/2)*height + i*width/2 + j];
        }
    }

    memcpy(pYUV422Data + width*height, pTempData, width*height*sizeof(uint8));

    SAFE_FREE(pTempData);
}

void convertYUV420PToYUV420SP(
    uint8   *pYUV420Data,
    uint16  width,
    uint16  height
    )
{
    uint16 i,j;
    uint8  *pTempData=NULL;
    assert(pYUV420Data);
    assert(width%2 ==0 && height%2 == 0);

    pTempData = (uint8*)malloc(width*height/2);

    //copy y
    for(i=0; i<height/2; i++)
    {
        for(j=0; j<width/2; j++)
        {
            //copy u
            pTempData[i* width + 2*j]=pYUV420Data [ width*height + i* width/2 + j];

            //copy v
            pTempData[i* width + 2*j + 1]=pYUV420Data[width*height + (width/2)*(height/2) + i* width/2 + j];
        }
    }

    memcpy(pYUV420Data+width*height, pTempData, width*height/2*sizeof(uint8));

    SAFE_FREE(pTempData);
}


