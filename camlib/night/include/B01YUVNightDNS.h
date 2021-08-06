#ifndef __BST_YUVNIGHTDDNS_H__
#define __BST_YUVNIGHTDDNS_H__

#ifdef __cplusplus
extern "C" {
#endif

	/* Image data type defination */
#define BST_IMAGE_TYPE_NV12                 0
#define BST_IMAGE_TYPE_NV21                 1
#define BST_IMAGE_TYPE_GRAY                 2
#define BST_IMAGE_TYPE_YV12                 3

	/* SDK error code */
#define BST_YUVNIGHTDNS_SUCCESS                     0            /* success */
#define BST_YUVNIGHTDNS_FAILURE                     -1           /* failure */

	/* SDK debug flags definition */
#define BST_YUVNIGHTDNS_FEATURE_LOG                  0x00000000  /* system log TAG : BST_3DDNS */

	/* SDK Config type definition */
#define BST_YUVNIGHTDNS_CONFIG_TYPE_FILE             0           /* file config */ 
#define BST_YUVNIGHTDNS_CONFIG_TYPE_BUFFER           1           /* buffer for config */

#define _In_
#define _In_opt_
#define _Inout_
#define _Out_

	/* init configuration defination */
	typedef struct
	{
		union
		{
			const char*     pConfigFile;     /* 3DDNS configuration file */
			struct
			{
				const void*     pBuffer;   /* 3DDNS configuration buffer */
				unsigned  int   nSize;
			}configBuffer;
		}cfg;
		int  nConfigType;

	}BSTYUVNIGHTDNSConfig;

	/* image rotation */
	typedef enum
	{
		BST_ROTATION_0 = 0,                 /* landscape, home button on the right */
		BST_ROTATION_90 = 90,                /* portrait, home button on the bottom */
		BST_ROTATION_180 = 180,               /* landscape, home button on the left  */
		BST_ROTATION_270 = 270                /* portrait, home button on the top */
	} BSTRotation;

	/* version information defination */
	typedef struct
	{
		unsigned int codeBase;		          /* codebase version number */
		unsigned int major;		              /* major version number */
		unsigned int minor;		              /* minor version number */
		unsigned int build;		              /* Build version number */
		const char*  versionString;           /* version in string form */
		const char*  buildDate;	              /* latest build Date */
		const char* configVersion;           /* config file version */
	} BSTYUVNIGHTDNSVersion;

	/* image data defination */
	typedef struct
	{
		unsigned int nFormat;			       /* image format */
		unsigned int nWidth;			       /* image width */
		unsigned int nHeight;			       /* image height */
		void*        pPlanePtr[4];		       /* image plane data ptr */
		unsigned int nRowPitch[4];		       /* row pitch for each plane */
	}BSTFrameData;

	/* point */
	typedef struct
	{
		int x;
		int y;
	} BPOINT, *PBPOINT;

	/* camera data defination */
	typedef struct
	{
        int nISO;                               /* camera ISO value */
        int nExpo;                               /* camera exposure time */
		BSTRotation         nRotation;     /* default set 0.image rotation: 0, 90, 180, 270*/

		unsigned int  reserved[16];            /* reserved */
	}BSTImageMetaData;

	/* camera data defination */
	typedef struct
	{
		BSTFrameData       imgData;				/* image data */
		BSTImageMetaData   metaData;
	}BSTImage;



	/* Retrive 3DDNS Lib Version */
	const BSTYUVNIGHTDNSVersion* bstYUVNIGHTDNSGetVersion();

	/* Initialize BST 3DDNS library */
	int bstYUVNIGHTDNSInit(
		_In_ BSTYUVNIGHTDNSConfig config,             /* config parameters */
		_In_ const unsigned char *AISegstr,
		_In_ int nRefFrameNum,                  /* RefFrame Number. e.g. 8 input frames, nRefFrameNum is 7 */
		_In_ int nExpNum
	);

	/* Un-initialize BST 3DDNS library */
	int bstYUVNIGHTDNSUninit();

	int bstYUVNIGHTDNSExitNotify();

	/* Process multiframes to 3DDNS image */
	int bstYUVNIGHTDNSAddFrame(
		_In_   int        nImageIndex,       /* input image index */
		_In_   BSTImage*  pInImage          /* input image*/

	);

	/* Process multiframes to 3DDNS image */
	int bstYUVNIGHTDNSRender(
		_Out_  BSTImage*  pOutImage         /* output image */
	);

#ifdef __cplusplus
}
#endif


void bstYUVNIGHTDNSDumpInputImage(
	_In_ BSTImage* pInImage,		/* input image Data */
	_In_ const char* dumpCfgPath,		/* path to read dump cfg file */
	_In_ const char* dumpPath			/* path to save dump data*/
);

void bstYUVNIGHTDNSDumpOutputImage(
	_In_ BSTImage* pInImage,		/* input image Data */
	_In_ const char* dumpCfgPath,		/* path to read dump cfg file */
	_In_ const char* dumpPath			/* path to save dump data*/
);
#endif // ifndef __BST_3DDNS_H__
