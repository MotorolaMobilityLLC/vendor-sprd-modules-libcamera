/* This sample code only act as a guide on how to use the sdk APIs properly.
* It will not work properly in real software system, unless all parameters are
set correctly */
#include "sprd_fcell.h" 
#include <pthread.h>


//int main(int argc, char **argv) {
int ov4c_init(ovfcell_init init){//unsigned char *xtalk_otp, unsigned char *otp_dpc){//unsigned short *xtalk_data, unsigned short *otp_data, int otpdpc_len) {

   printf("fcellGetVersion: %08x\n", ov_fcell_getversion());
    if (E_OV_FCD_OK != ov_fcell_init(&init, (fcell_allocator)allocator,
                                     (fcell_deallocator)deallocator)) {
        printf("ov_fcell_init fails!!!\n");
        return -1;
    }
//    free(pFcellXtalk);
 //   free(pFCellOtpdpc);
    // Step 2: Prepare parameters to call Fcell library

    control.pattern = FCELL_PATTERN;
    control.gain_camera = CAMERA_GAIN;
    control.xtk_blc = XTALK_BLC;
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
int ov4c_raw_process1(unsigned short *src, unsigned short *dst, uint32_t input_width, uint32_t input_height){

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

int ov4c_raw_process(unsigned short *src, unsigned short *dst, uint32_t input_width, uint32_t input_height){

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

int ov4c_process(unsigned short *src,unsigned short *dst) {
    // Step 3: Process Fcell.
 //   struct timeval start, end;
#if 1
  //  int imgsize = IMG_WIDTH * IMG_HEIGHT * 2;
    uint16 *pFcellImage = (uint16 *)dst;//malloc(imgsize);

    ov4c_raw_process(src, pFcellImage, IMG_WIDTH, IMG_HEIGHT);

    if (E_OV_FCD_OK != ov_fcell_process((unsigned short *)pFcellImage,
                                        (unsigned short *)dst,//pOutImage,
                                        &control)) {
        printf("ov_fcell_process fails!!!\n");
        return -1;
    }
    ov4c_raw_process1(src, dst, IMG_WIDTH, IMG_HEIGHT);
#endif
//    gettimeofday(&end, NULL);
/*    fprintf(stdout, "fcell process finish, time: %ld ms.\n",
            1000 * (end.tv_sec - start.tv_sec) +
                (end.tv_usec - start.tv_usec) / 1000);*/
    // Step 4: Save output
 /*   fp1 = fopen(outImgFile, "wb");
    if (fp1) {
        fwrite(pOutImage, imgsize, 1, fp1);
        fclose(fp1);
    }*/
     /*   free(pFcellImage);
    free(pOutImage);*/
    return 0;
}
int ov4c_release() {    
    ov_fcell_release();
    return 0;
}

