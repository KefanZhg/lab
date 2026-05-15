#ifndef DATA_H
#define DATA_H

#include "stdint.h"

typedef struct data_3d_float_s {
    uint64_t utime;
    float x;
    float y;
    float z;
} data_3d_float_t;

typedef struct data_3d_double_s {
    uint64_t utime;
    double x;
    double y;
    double z;
} data_3d_double_t;

typedef struct data_3d_uint16_s {
    uint64_t utime;
    uint16_t x;
    uint16_t y;
    uint16_t z;
} data_3d_uint16_t;


typedef struct data_3d_int16_s {
    uint64_t utime;
    int16_t x;
    int16_t y;
    int16_t z;
} data_3d_int16_t;

typedef struct data_3d_int32_s {
    uint64_t utime;
    int32_t x;
    int32_t y;
    int32_t z;
} data_3d_int32_t;

typedef struct data_3d_uint32_s {
    uint64_t utime;
    uint32_t x;
    uint32_t y;
    uint32_t z;
} data_3d_uint32_t;

typedef struct data_3d_int64_s {
    uint64_t utime;
    int64_t x;
    int64_t y;
    int64_t z;
} data_3d_int64_t;

typedef struct data_3d_uint64_s {
    uint64_t utime;
    uint64_t x;
    uint64_t y;
    uint64_t z;
} data_3d_uint64_t;

#endif // DATA_H