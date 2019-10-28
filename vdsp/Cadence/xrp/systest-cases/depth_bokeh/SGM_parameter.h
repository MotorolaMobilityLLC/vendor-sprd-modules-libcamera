#ifndef _SGM_PARAMETER_H_
#define _SGM_PARAMETER_H_

struct SGMParamStruct
{

         unsigned char SensorDirection;
	 unsigned char DepthScaleMin;
	 unsigned char DepthScaleMax; 

	unsigned char CalibInfiniteZeroPt;  //The Calibration Zero Point is Infinite or Not, it is from Calibration File
	int SearchRange; // Search Range
	int MinDExtendRatio; // Min Disparity Search Value Adjust Ratio 
	int inDistance;
	int inRatio;
	int outDistance;
	int outRatio;

	float m_sigma;
	int use_post_processing;
	int maxd_sobel_ratio; //Used to calculate max_dist, MAX 64 ~1.0, for example 16 ~ 0.25; 
	int block_sad_ratio;  // ratio used to combine SAD and Census Difference: MAX 256
	int grad_dist_ratio; // ratio used to combine grad difference and block difference:MAX 4
	int max_sobel;    //gradient is calculated by sobel operator. The gradient is clipped by max_sobel
	int mind_sobel;   //minimum cost calculated by gradient difference
	int maxd_sobel;   //maximum cost calculated by gradient difference
	int mind;			//minimum cost calculated by pixel intensity difference. It has two components: SAD and census transform
	int maxd;         //maximum cost calculated by pixel intensity difference

};

#endif
