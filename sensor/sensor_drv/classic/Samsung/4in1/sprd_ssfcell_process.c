/* This sample code only act as a guide on how to use the sdk APIs properly.
* It will not work properly in real software system, unless all parameters are
set correctly */
#include "sprd_fcell_ss.h" 
#include <pthread.h>


//int main(int argc, char **argv) {
int ss4c_init(ssfcell_init init){

  remosaic_init(init.width, init.height,init.bayer_order, init.pedestal);
  remosaic_gainmap_gen(init.xtalk, init.xtalk_len);

    return 0;
}
struct mThreadParam
{
    uint8_t *pIn;
    uint8_t *pOut;
    uint32_t count;
};
void *raw_process(void *data) {
    struct mThreadParam *param = (struct mThreadParam *)data;
    uint32_t count = param->count;
    uint8_t *pIn = param->pIn;
    uint8_t *pOut = param->pOut;
    for (uint32_t i = 0; i < count; i = i+5) {
        *pOut = ((*(pIn+4)) & 0b00000011) |  ((*pIn & 0b00111111)<<2);
        pOut ++;
        *pOut = ((*pIn) & 0b11000000) >> 6;
        pOut ++;
        *pOut = (((*(pIn+4)) & 0b00001100) >> 2) |  ((*(pIn+1) & 0b00111111)<<2);
        pOut ++;
        *pOut = ((*(pIn+1)) & 0b11000000) >> 6;
        pOut ++;
        *pOut = (((*(pIn+4)) & 0b00110000) >> 4) |  ((*(pIn+2) & 0b00111111)<<2);
        pOut ++;
        *pOut = ((*(pIn+2)) & 0b11000000) >> 6;
        pOut ++;
        *pOut = (((*(pIn+4)) & 0b11000000) >> 6) |  ((*(pIn+3) & 0b00111111)<<2);
        pOut ++;
        *pOut = ((*(pIn+3)) & 0b11000000) >> 6;
        pOut ++;
        pIn = pIn + 5;
    }
    return NULL;
}
#if 1
void *raw_process1(void *data) {
    struct mThreadParam *param = (struct mThreadParam *)data;
    uint32_t count = param->count;
    uint8_t *pIn = param->pIn;
    uint8_t *pOut = param->pOut;
    for (uint32_t i = 0; i < count; i = i+4) {
        *pOut = *pIn >> 2 | ((*(pIn+1)& 0b00000011) << 6);
        *(pOut+1) = *(pIn+2) >> 2 | ((*(pIn+3)& 0b00000011) << 6);
        *(pOut+2) = *(pIn+4) >> 2 | ((*(pIn+5)& 0b00000011) << 6);
        *(pOut+3) = *(pIn+6) >> 2 | ((*(pIn+7)& 0b00000011) << 6);
         *(pOut+4) = ((*(pIn+6)& 0b00000011) <<6) |((*(pIn+4)& 0b00000011) <<4) |((*(pIn+2)& 0b00000011) <<2) |(*(pIn)& 0b00000011) ;
         pIn = pIn + 8;
         pOut = pOut+5;
    }
    return NULL;
}
int ss4c_raw_process1(unsigned short *src, unsigned short *dst, uint32_t input_width, uint32_t input_height){

///////////call raw_process example///////////
    struct mThreadParam iparams[4];
    pthread_t ths[4];
    uint32_t aFrameLen10 = (input_width*input_height*5/4)/4;
    uint32_t aFrameLen16 = (input_width*input_height)/4;

    unsigned char *m_pInputBuffer = (unsigned char *)src;//(unsigned char *)pFcellImage;//src;
    uint16_t *inputRaw16 = (uint16 *)dst;//malloc(input_width*input_height);
    for(int i=0; i<4; i++) {
        iparams[i].count = aFrameLen16;
        iparams[i].pOut = (uint8_t *)(m_pInputBuffer+aFrameLen10*i);
        iparams[i].pIn = (uint8_t *)(inputRaw16+aFrameLen16*i);
        pthread_create(&ths[i], NULL, raw_process1, (void *)&iparams[i]);
    }
    for(int i=0; i<4; i++) {
        pthread_join(ths[i], NULL);
    }

/*prcessed raw10 is : inputRaw16*/
    return 0;
}

int ss4c_raw_process(unsigned short *src, unsigned short *dst, uint32_t input_width, uint32_t input_height){

///////////call raw_process example///////////
    struct mThreadParam iparams[4];
    pthread_t ths[4];
    uint32_t aFrameLen10 = (input_width*input_height*5/4)/4;
    uint32_t aFrameLen16 = (input_width*input_height)/4;

    unsigned char *m_pInputBuffer = (unsigned char *)src;//(unsigned char *)pFcellImage;//src;
    uint16_t *inputRaw16 = (uint16 *)dst;//malloc(input_width*input_height);
    for(int i=0; i<4; i++) {
        iparams[i].count = aFrameLen10;
        iparams[i].pIn = (uint8_t *)(m_pInputBuffer+aFrameLen10*i);
        iparams[i].pOut = (uint8_t *)(inputRaw16+aFrameLen16*i);
        pthread_create(&ths[i], NULL, raw_process, (void *)&iparams[i]);
    }
    for(int i=0; i<4; i++) {
        pthread_join(ths[i], NULL);
    }

/*prcessed raw16 is : inputRaw16*/
    return 0;
}
/////////////////////
#endif

int ss4c_process(unsigned short *src,unsigned short *dst, 
				struct st_remosaic_param* p_param, uint32_t input_width, uint32_t input_height) {
    // Step 3: Process Fcell.
 //   struct timeval start, end;
#if 1
    int imgsize = input_width * input_height * 5 / 4;
    uint16 *pFcellImage = (uint16 *)dst;//malloc(imgsize);

  //  ss4c_raw_process(src, pFcellImage, IMG_WIDTH, IMG_HEIGHT);

#if 0
 FILE *fp1 = fopen("/data/vendor/cameraserver/4c_proc.raw", "wb");
 if (fp1) {
	 fwrite(src, imgsize, 1, fp1);
	 fclose(fp1);
 }
#endif
 remosaic_process_param_set(p_param);

 remosaic_process((int32_t)(void *)src, imgsize, (int32_t)(void *)dst, imgsize *8/5);

  ss4c_raw_process1(src, dst, input_width, input_height);
#endif
//    gettimeofday(&end, NULL);
/*    fprintf(stdout, "fcell process finish, time: %ld ms.\n",
            1000 * (end.tv_sec - start.tv_sec) +
                (end.tv_usec - start.tv_usec) / 1000);*/
    // Step 4: Save output
#if 0
    FILE *fp1 = fopen("/data/vendor/cameraserver/4c_proc.raw", "wb");
    if (fp1) {
        fwrite(dst, imgsize*8/5, 1, fp1);
        fclose(fp1);
    }
#endif
     /*   free(pFcellImage);
    free(pOutImage);*/
    return 0;
}
int ss4c_release() {    
    remosaic_deinit();
    return 0;
}

