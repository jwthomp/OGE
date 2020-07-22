#ifndef __CORE_TYPES_H_
#define __CORE_TYPES_H_

typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef signed long long int64;
typedef signed long int32;
typedef signed short int16;
typedef signed char int8;

// Do not assume that this is a 32 bit float, could be double
typedef float real;

// Use these when precision must be 32 or 64 bit
typedef float real32;
typedef double real64;


#endif /* __CORE_TYPES_H_ */

