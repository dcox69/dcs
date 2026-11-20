/**
 * @file bcs_variant.h
 * @author dcox69 (David GOMEZ) <dcox69@proton.me>   
 * @date 2026-04-07
 * @brief this header file defines the bcs_variant_t type, which is a union that can hold various types 
 * of data. It is used in the event system to allow messages to carry different types of data without 
 * needing a separate structure for each message type. The union includes fields for common data types 
 * such as integers, floats, pointers, and also includes specific structures for representing runtime 
 * and datetime information. By using a union, it allows for efficient memory usage while still 
 * providing flexibility in the types of data that can be associated with events in the BCS project.
 */

#pragma once 

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include <ml_datetime.h>

typedef union /*__attribute__((packed)) */ ml_variant_u
{
    char     c;
    short    w;
    bool     b;
    int      i;
    uint     u;  
    uint8_t  u8;
    int8_t   i8;
    uint16_t u16;
    int16_t  i16;
    uint32_t u32;
    int32_t  i32;
    uint64_t u64;
    int64_t  i64;
    size_t   sz;
    float    f;
    double   d;
    void    *p;
    char    *s;
    ml_uptime_t uptime;
    ml_datetime_t datetime;    
} ml_variant_t;

_Static_assert(sizeof(ml_variant_t) == 8, "ml_variant_t union size mismatch !");
