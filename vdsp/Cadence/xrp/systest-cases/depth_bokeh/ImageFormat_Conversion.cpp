#include "ImageFormat_Conversion.h"

void YCbCr2RGB(int *Y, int *Cb, int *Cr,  int nWidth, int nHeight)
{
	int i,j;

	for (j = 0;j < nHeight; j ++)
	{
		for (i = 0; i < nWidth; i ++)
		{
			int loc = j * nWidth + i;
			int Yval = Y[loc];
			int Cbval = Cb[loc];
			int Crval = Cr[loc];
			Y[loc] = (int)(CLIP(floor((Yval + 1.402 * (Crval-128.0))),0,255)); //R
			Cb[loc] = (int)(CLIP(floor((Yval - 0.3441 * (Cbval-128.0) - 0.71414 *(Crval-128.0))),0,255)); //G
			Cr[loc] = (int)(CLIP(floor((Yval + 1.772 * (Cbval-128.0))),0,255)); //B


			//*(rgb_buf + len * j + i*3)      = Y[loc];  // 
			//*(rgb_buf + len * j + i*3 + 1)  = Cb[loc];  //G
			//*(rgb_buf + len * j + i*3 + 2)  = Cr[loc] ;	//

		}
	}
}

void ReadoutYUV420(char inputfilename[256],int nWidth, int nHeight, unsigned char *RGBBuffer, 
	int flip_flag, int mirror_flag)
{
	FILE *fp;
	unsigned char *YUV_buffer;
	int i,j;
	int ii,jj;
	int *R,*G,*B;

	R = (int *)calloc(nWidth * nHeight,sizeof(int));
	G = (int *)calloc(nWidth * nHeight,sizeof(int));
	B = (int *)calloc(nWidth * nHeight,sizeof(int));
	YUV_buffer = (unsigned char *)calloc(nWidth * nHeight * 3/2,sizeof(unsigned char));
	fp = fopen(inputfilename,"r");
	fread(YUV_buffer,nWidth * nHeight * 3/2,sizeof(unsigned char),fp);
	fclose(fp);

	for (i = 0; i < nHeight; i ++)
	{
		for (j = 0; j < nWidth; j ++)
		{
			R[i * nWidth + j] = (int)(YUV_buffer[i * nWidth + j]);//(int)readval;
			G[i * nWidth + j] = (int)(YUV_buffer[nHeight * nWidth + ((i/2) * (nWidth/2) + (j/2))*2]);
			B[i * nWidth + j] = (int)(YUV_buffer[nHeight * nWidth + ((i/2) * (nWidth/2) + (j/2))*2 + 1]);
		}
	}
	YCbCr2RGB(R,G,B,nWidth,nHeight);

	for (i = 0; i < nHeight; i ++)
	{
		for (j = 0; j < nWidth; j ++)
		{
			ii = i;
			jj = j;
			if (flip_flag == 1)
				jj = nWidth - 1 - jj;
			if (mirror_flag == 1)
				ii = nHeight - 1 - ii;
			RGBBuffer[(i * nWidth + j) *3] = (unsigned char)(R[ii * nWidth + jj]);
			RGBBuffer[(i * nWidth + j) *3 +  1] = (unsigned char)(G[ii * nWidth + jj]);
			RGBBuffer[(i * nWidth + j) *3 +  2] = (unsigned char)(B[ii * nWidth + jj]);
		}
	}
	free(YUV_buffer);
	free(R);
	free(G);
	free(B);
}


BOOL ReadYUV420File(char inputfilename[256], int nWidth, int nHeight, uint8 *yuv_image)
{
	FILE *fp;
	uint8 *u_plane, *v_plane;
	uint8 *uv_plane;
	int32 i, j;
	int32 index;

	fp = fopen(inputfilename, "rb");
	if(NULL == fp) {
		printf("Can't open yuv file: %s\n", inputfilename);
		return FALSE;
	}
	uv_plane = (uint8*)malloc(nWidth * nHeight / 2);

	fread(yuv_image, nWidth * nHeight, sizeof(unsigned char), fp);
	fread(uv_plane,  nWidth * nHeight / 2, sizeof(unsigned char), fp);
	fclose(fp);

	u_plane = yuv_image + nWidth * nHeight;
	v_plane = u_plane + nWidth * nHeight / 4;
	index = 0;
	for(i = 0; i < nHeight/2; i++) {
		for(j = 0; j < nWidth/2; j++) {
			*u_plane++ = uv_plane[index++];
			*v_plane++ = uv_plane[index++];
		}
	}

	free(uv_plane);
	return TRUE;
}
BOOL ReadNV21File(char inputfilename[256], int nWidth, int nHeight, uint8 *yuv_image)
{
	FILE *fp;
	uint8 *u_plane, *v_plane;
	uint8 *uv_plane;
	int32 i, j;
	int32 index;

	fp = fopen(inputfilename, "rb");
	if(NULL == fp) {
		printf("Can't open yuv file: %s\n", inputfilename);
		return FALSE;
	}

	fread(yuv_image, nWidth * nHeight *3/2, sizeof(unsigned char), fp);

	return TRUE;
}

BOOL ReadYUV422File(char inputfilename[256], int nWidth, int nHeight, uint8 *yuv_image)
{
	FILE *fp;
	//uint8 *u_plane, *v_plane;
	//uint8 *uv_plane;
	//int32 i, j;
	//int32 index;

	fp = fopen(inputfilename, "rb");
	if(NULL == fp) {
		printf("Can't open yuv file: %s\n", inputfilename);
		return FALSE;
	}
	//uv_plane = (uint8*)malloc(nWidth * nHeight / 2);

	fread(yuv_image, nWidth * nHeight * 2, sizeof(unsigned char), fp);
	fclose(fp);

	return TRUE;
}

void ImageYUV420ToRGB3Planar(uint8* yuv420_image, int32 image_width, int32 image_height, uint8* rgb_image)
{
	uint8 *y_plane, *u_plane, *v_plane;
	uint8 *r_plane, *g_plane, *b_plane;
	int32 i, j;
	uint8 pixel_y, pixel_u, pixel_v;
	int32 temp;

	y_plane = yuv420_image;
	u_plane = y_plane + image_width * image_height;
	v_plane = u_plane + image_width * image_height / 4;
	r_plane = rgb_image;
	g_plane = r_plane + image_width * image_height;
	b_plane = g_plane + image_width * image_height;
	for(i = 0; i < image_height; i++) {
		for(j = 0; j < image_width; j++) {
			pixel_y = y_plane[i * image_width + j];
			pixel_u = u_plane[i/2 * (image_width/2) + (j/2)];
			pixel_v = v_plane[i/2 * (image_width/2) + (j/2)];

			temp = (int32)(pixel_y + (1.370705 * (pixel_v - 128)));
			temp = CLIP(temp, 0, 255);
			b_plane[i * image_width + j] = (uint8)temp;
			//r_plane[i * image_width + j] = (uint8)temp;

			temp = (int32)(pixel_y - (0.698001 * (pixel_v - 128)) - (0.337633 * (pixel_u - 128)));
			temp = CLIP(temp, 0, 255);
			g_plane[i * image_width + j] = (uint8)temp;

			temp = (int32)(pixel_y + (1.732446 * (pixel_u - 128)));
			temp = CLIP(temp, 0, 255);
			r_plane[i * image_width + j] = (uint8)temp;
			//			b_plane[i * image_width + j] = (uint8)temp;
		}
	}
}

void ImageYUV420_flip(uint8* yuv420_image, int32 image_width, int32 image_height, uint8* yuv420_image_flipped, BOOL flip_flag, BOOL mirror_flag)
{
	uint8 *y_plane, *u_plane, *v_plane;
	uint8 *y_plane_flipped, *u_plane_flipped, *v_plane_flipped;
	int32 i, j;
	int32 ii, jj;
	//uint8 pixel_y, pixel_u, pixel_v;
	//int32 temp;

	y_plane = yuv420_image;
	u_plane = y_plane + image_width * image_height;
	v_plane = u_plane + image_width * image_height / 4;
	y_plane_flipped = yuv420_image_flipped;
	u_plane_flipped = y_plane_flipped + image_width * image_height;
	v_plane_flipped = u_plane_flipped + image_width * image_height / 4;
	for(i = 0; i < image_height; i++) {
		for(j = 0; j < image_width; j++) {
			ii = i;
			jj = j;
			if (flip_flag == 1)
				jj = image_width - 1 - jj;
			if (mirror_flag == 1)
				ii = image_height - 1 - ii;
			y_plane_flipped[i * image_width + j] = y_plane[ii * image_width + jj];
		}
	}

	for(i = 0; i < image_height/2; i++) {
		for(j = 0; j < image_width/2; j++) {
			ii = i;
			jj = j;
			if (flip_flag == 1)
				jj = image_width/2 - 1 - jj;
			if (mirror_flag == 1)
				ii = image_height/2 - 1 - ii;
			u_plane_flipped[i * image_width/2  + j] = u_plane[ii * image_width/2  + jj];
		}
	}

	for(i = 0; i < image_height/2; i++) {
		for(j = 0; j < image_width/2; j++) {
			ii = i;
			jj = j;
			if (flip_flag == 1)
				jj = image_width/2 - 1 - jj;
			if (mirror_flag == 1)
				ii = image_height/2 - 1 - ii;
			v_plane_flipped[i * image_width/2 + j] = v_plane[ii * image_width/2 + jj];
		}
	}
}
