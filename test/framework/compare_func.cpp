#include "compare_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define LOG_TAG "IT-Compare"

/*
YUV420format:Support I420,YV12,NV12,NV21 only in Y region
Support NV21  in Y region, U region,V region.
Comparing images requires the same YUV image size
*/

int compare_rengion(compareInfo_t &com,int rengion)
{
    int32_t ret=IT_OK;
    int64_t size= (com.w) * (com.h);
    int64_t diff=0;
    int64_t num=0;
    double cmp_ratio=0.0;
    char reng;
    //full threshold value
    float threshold=0.0;
    //full diff mean
    double full_mean=0;
    double area_mean=0.0;
    float part_threshold=0.0;
    double part_mean=0.0;
    int64_t pxl_start=0;
    int64_t pxl_end=0;
    int32_t pxl_span=0;
    int32_t temp=0;

    if (rengion==Y_RENGION){
        reng='Y';
        pxl_start=0;
        pxl_end=size-1;
        pxl_span=1;
        threshold=8;
    }
    if (rengion==V_RENGION){
        reng='V';
        pxl_start=size;
        pxl_end=size*3/2-2;
        pxl_span=2;
        threshold=3;
    }
    if (rengion==U_RENGION){
        reng='U';
        pxl_start=size+1;
        pxl_end=size*3/2-1;
        pxl_span=2;
        threshold=3;
    }
    for(int64_t i=pxl_start; i<pxl_end; i=i+pxl_span){
        temp=abs(com.test_img[i]-com.golden_img[i]);
        if (temp>0){
            diff=diff+temp;
            num++;
        }
    }
    if (diff<=0){
        IT_LOGD("Similarity ratio in %c region is 100%%.",reng);
        ret=IT_OK;
        cmp_ratio=100.0;
    }else{
        full_mean=(double)diff/((pxl_end-pxl_start)/pxl_span);
        area_mean=(double)diff/num;
        if(area_mean<threshold-3){
            part_threshold=area_mean+3;
        }else{
            part_threshold=threshold;}
        diff=0;
        num=0;
        for(int64_t i=pxl_start; i<pxl_end; i=i+pxl_span){
            temp=abs(com.test_img[i]-com.golden_img[i]);
            if (temp>part_threshold){
                diff=diff+temp;
                num++;
            }
        }
        if(num>0){
            part_mean=(double)diff/num;
        }
        cmp_ratio=100.0-full_mean-(area_mean/threshold)-(part_mean/threshold);
        if(cmp_ratio<0.0){
            cmp_ratio=0.0;
        }
        IT_LOGD("Similarity ratio in %c region is %.4lf%%.",reng,cmp_ratio);
    }
    if(cmp_ratio>=100){
        ret=IT_OK;
    }else if((full_mean<threshold)&&((part_mean/threshold)<=2)){
        ret=IT_OK;
        }else{
            ret=IT_ERR;
            }
    if(full_mean>=threshold){
        IT_LOGD("Mean diff exceeds threshold in %c range.",reng);
    }
    if((part_mean/threshold)>2){
        IT_LOGD("Part diff exceeds threshold in %c range.",reng);
    }
    return ret;
}

int compare_yuv(compareInfo_t &compin){
    int32_t ret=IT_ERR;

    if(!compin.test_img||!compin.golden_img){
        IT_LOGD("Compare image error.");
    }else{
        if (compare_rengion(compin,Y_RENGION)==0){
            if(compare_rengion(compin,V_RENGION)==0){
                if(compare_rengion(compin,U_RENGION)==0){
                   ret=IT_OK;
                   IT_LOGD("CameraIT compare pass.");
                }
            }
        }
    }
    return ret;
}

//determine if the format is correct
int compare_Image(compareInfo_t &compinfo){
    int32_t ret=IT_OK;
    if(compinfo.format==IT_IMG_FORMAT_YUV){
        ret=compare_yuv(compinfo);
    }
    else{
        ret=IT_OK;
        IT_LOGD("CameraIT supports only nv21 comparisons for now.");
    }
    if(ret==IT_ERR){
        IT_LOGD("CameraIT compare fail.");
    }
    IT_LOGD("CameraIT compare complete.");
    return ret;
}