#ifndef __VERSION_H__
#define __VERSION_H__

#define API_STRINGIFY(s)    API_TOSTRING(s)
#define API_TOSTRING(s)     #s

#define API_VERSION_DOT(a,b,c,d)    a "." b "." c "." d
#define API_VERSION(a,b,c,d)        API_VERSION_DOT(API_STRINGIFY(a),API_STRINGIFY(b),API_STRINGIFY(c),API_STRINGIFY(d))

/* major is responsible for interface-compatible*/
#define BEAUTY_ADA_VERSION_MAJOR      1
/* increased along with each big update, e.g. for function extension*/
#define BEAUTY_ADA_VERSION_MINOR      0
/* increased alone with each small update, e.g. for bug fix or optimization */
#define BEAUTY_ADA_VERSION_MICRO      1
/* increased along with each tiny update */
#define BEAUTY_ADA_VERSION_NANO       0

#define BEAUTY_ADA_VERSION            API_VERSION(BEAUTY_ADA_VERSION_MAJOR,BEAUTY_ADA_VERSION_MINOR,BEAUTY_ADA_VERSION_MICRO,BEAUTY_ADA_VERSION_NANO)

#endif /*__VERSION_H__*/
