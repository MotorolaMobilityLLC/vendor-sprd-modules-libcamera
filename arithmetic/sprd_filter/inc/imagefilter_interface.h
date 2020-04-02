#ifndef __IMAGEFILTER_INTERFACE_H__
#define __IMAGEFILTER_INTERFACE_H__

#ifdef WIN32
#define JNIEXPORT
#else
#include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *IFENGINE;
#define IFLUT64_SIZE 512

#define SIMPLIFIED_VERSION

typedef enum { // optional params
#ifndef SIMPLIFIED_VERSION
    NoneFilter = 0,
    ColorMapFilter = 1, // param1:IFMap256
    PixelFilter = 2,    // param1:IFSize
    InvertColorFilter = 3,
    MonoFilter = 4,
    MonoChromeFilter = 5,
    ColorMatrixFilter = 6, // param1:IFMatrix3x3
    GrayFilter = 7,
    NostalgiaFilter = 8,
    LookupFilter = 9, // param1:IFLUT64
    Strip2Filter = 10,
    Strip3Filter = 11,
    BerylFilter = 12,
    BrannanFilter = 13,
    CrispWarmFilter = 14,
    FallColorsFilter = 17,
    GothamFilter = 19,
    HefeFilter = 20,
    InkwellFilter = 22,
    LomofiFilter = 23,
    LordKelvinFilter = 24,
    SutroFilter = 27,
    TealOrangePlusContrastFilter = 28,
    TensionGreenFilter = 29,
    ToasterFilter = 30,
    WaldenFilter = 31,
    XProIIFilter = 32,
    CalciteFilter = 34,
    ColdLBB25Filter = 35,
    ColdLBB50Filter = 36,
    ColdLBB75Filter = 37,
    ColdQing25Filter = 38,
    ColdQing50Filter = 39,
    ColdQing75Filter = 40,
    WarmLBA25Filter = 41,
    WarmLBA50Filter = 42,
#endif
    CrispWinterFilter = 15,
    EarlybirdFilter = 16,
    Filmstock50Filter = 18,
    HorrorBlueFilter = 21,
    NashvilleFilter = 25,
    SoftWarmingFilter = 26,
    BismuthFilter = 33,
    WarmLBA75Filter = 43,
    HistoryFilter = 44,
    HaloFilter = 45,
    WaterReflectionFilter = 46,
    LightFilter = 47
} IFFilterType;

typedef enum {
    NV21,
    RGB888,
} IFImageType;

typedef struct {
    IFImageType imageType;
    int imageWidth;
    int imageHeight;
    char *paramPath;
} IFInitParam;

typedef struct {
    void *param1;
    void *param2;
} IFFilterParam;

typedef struct {
    int orientation;
    int flip_on;
    int is_front;
    int filter_version;
} FilterParam_t;

typedef struct { unsigned char val[3][3]; } IFMatrix3x3;

typedef struct {
    unsigned char R[256];
    unsigned char G[256];
    unsigned char B[256];
} IFMap256;

typedef struct {
    int R;
    int G;
    int B;
} IFVec3;

typedef struct {
    int x;
    int y;
} IFSize;

typedef struct {
    // B-Quad G-Row R-Col
    unsigned char val[IFLUT64_SIZE][IFLUT64_SIZE][3]; // 512 x 512 RGB texture
} IFLUT64;

typedef struct {
    void *c1;
    void *c2;
    void *c3;
} IFImageData;

JNIEXPORT IFENGINE ImageFilterCreateEngine(IFInitParam *initParam);
JNIEXPORT int ImageFilterRun(IFENGINE engine, IFImageData *input,
                             IFImageData *output,IFFilterType filterType, FilterParam_t *param,
                             IFFilterParam *filterParam);
JNIEXPORT int ImageFilterDestroyEngine(IFENGINE engine);

#ifdef __cplusplus
}
#endif

#endif
