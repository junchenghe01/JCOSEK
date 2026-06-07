#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H
#include <stdbool.h>
#include <stdio.h>

#ifndef TRUE
#define TRUE ((boolean)1u)
#endif

#ifndef FALSE
#define FALSE ((boolean)0u)
#endif
typedef unsigned char boolean; /* for use with TRUE/FALSE        */

typedef signed char        sint8;  /*        -128 .. +127            */
typedef unsigned char      uint8;  /*           0 .. 255             */
typedef signed short       sint16; /*      -32768 .. +32767          */
typedef unsigned short     uint16; /*           0 .. 65535           */
typedef signed long        sint32; /* -2147483648 .. +2147483647     */
typedef unsigned long      uint32; /*           0 .. 4294967295      */
typedef signed long long   sint64; /* \brief 64-bit unsigned integer */
typedef unsigned long long uint64;
typedef float              float32;
typedef double             float64;
typedef unsigned int       uintptr; /* \brief Type large enough to hold a pointer */
#endif                              /* PLATFORM_TYPES_H */
