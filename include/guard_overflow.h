//
//  include/guard_overflow.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef GUARD_OVERFLOW_H
#define GUARD_OVERFLOW_H

#include <stdint.h>

#define guard_overflow_add(left_in, right) _Generic((left_in), \
    uint32_t *: guard_overflow_add_uint32, \
    uint64_t *: guard_overflow_add_uint64 \
) (left_in, right)

#define guard_overflow_mul(left_in, right) _Generic((left_in), \
    uint32_t *: guard_overflow_mul_uint32, \
    uint64_t *: guard_overflow_mul_uint64 \
) (left_in, right)

int guard_overflow_add_uint32(uint32_t *left_in, uint32_t right);
int guard_overflow_add_uint64(uint64_t *left_in, uint64_t right);

int guard_overflow_mul_uint32(uint32_t *left_in, uint32_t right);
int guard_overflow_mul_uint64(uint64_t *left_in, uint64_t right);

#endif /* GUARD_OVERFLOW_H */
