#ifndef _ALGO_MACRO_H
#define _ALGO_MACRO_H

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define PNULL (NULL)

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#define SAFE_FREE(x)    do{if(x){free(x);x = NULL;}}while(0)

#define MAX(a,b)    ((a)>(b)?(a):(b))

#define MIN(a,b)    ((a)>(b)?(b):(a))

#define MAX3(a,b,c) MAX(MAX(a,b),c)

#define MIN3(a,b,c) MIN(MIN(a,b),c)

#define MAX_FOUR(a, b, c, d) (MAX(MAX(a,b), MAX(c,d)))

#define MIN_FOUR(a, b, c, d) (MIN(MIN(a,b), MIN(c,d)))

#define ARRAY_ELEMS(a)  (sizeof(a)/sizeof((a)[0]))

#define CLIP(v_,v_max,v_min)    do{if((v_) > (v_max)){(v_) = (v_max);}if((v_) < (v_min)){(v_) = (v_min);}}while(0)

#define ABS(v)      ((v)>0?(v):(-(v)))

#define PIX_SORT(a, b) {if (a > b) PIX_SWAP(a, b);}
#define PIX_SWAP(a, b) {uint16 temp = a; a = b; b = temp;}


#endif
