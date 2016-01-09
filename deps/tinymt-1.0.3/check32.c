/**
 * @file check32.c
 *
 * @brief Simple check program for tinymt32
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2011 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and University of Tokyo.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include "tinymt32.h"

int main(int argc, char * argv[]) {
    if (argc < 4) {
	printf("%s mat1 mat2 tmat [seed]\n", argv[0]);
	return -1;
    }
    tinymt32_t tinymt;
    tinymt.mat1 = strtoul(argv[1], NULL, 16);
    tinymt.mat2 = strtoul(argv[2], NULL, 16);
    tinymt.tmat = strtoul(argv[3], NULL, 16);
    int seed = 1;
    uint32_t seed_array[5];
    if (argc >= 5) {
	seed = strtol(argv[4], NULL, 0);
    }
    printf("tinymt32 0x%08"PRIx32, tinymt.mat1);
    printf(" 0x%08"PRIx32, tinymt.mat2);
    printf(" 0x%08"PRIx32, tinymt.tmat);
    printf(" seed = %d\n", seed);
    tinymt32_init(&tinymt, seed);
    printf("32-bit unsigned integers r, where 0 <= r < 2^32\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%10"PRIu32" ", tinymt32_generate_uint32(&tinymt));
	}
	printf("\n");
    }
    printf("init_by_array {%d}\n", seed);
    seed_array[0] = seed;
    tinymt32_init_by_array(&tinymt, seed_array, 1);
    printf("float numbers r, where 0.0 <= r < 1.0\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%.7f ", tinymt32_generate_float(&tinymt));
	}
	printf("\n");
    }
    printf("float numbers r, where 1.0 <= r < 2.0\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%.7f ", tinymt32_generate_float12(&tinymt));
	}
	printf("\n");
    }

    printf("float numbers r, where 0.0 < r <= 1.0\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%.7f ", tinymt32_generate_floatOC(&tinymt));
	}
	printf("\n");
    }

    printf("float numbers r, where 0.0 < r < 1.0\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%.7f ", tinymt32_generate_floatOO(&tinymt));
	}
	printf("\n");
    }

    printf("32-bit precision double numbers r, where 0.0 <= r < 1.0\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 5; j++) {
	    printf("%.7f ", tinymt32_generate_32double(&tinymt));
	}
	printf("\n");
    }
}
