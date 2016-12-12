#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned char      u8 ;
typedef signed char        s8;
typedef unsigned short     u16;
typedef signed short       s16;
typedef int                s32;
typedef unsigned int       u32;
typedef long long          s64;
typedef unsigned long long u64;
typedef u32                u24;

typedef u32                UTCTime;
typedef u8                 status_t;
typedef u32                arg_t;
typedef u32                rst_t;

#ifndef NULL
#define NULL               ((void*)0)
#endif

#ifndef TRUE
#define TRUE               1
#endif

#ifndef FALSE
#define FALSE              0
#endif


#define BUILD_UINT16(loByte, hiByte) \
          ((u16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#endif // __TYPES_H__
