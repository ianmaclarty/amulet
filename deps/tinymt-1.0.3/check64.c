/**
 * @file check64.c
 *
 * @brief a little check program for tinymt64
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
#include "tinymt64.h"

int main(int argc, char * argv[]) {
    if (argc < 4) {
	printf("%s mat1 mat2 tmat [seed]\n", argv[0]);
	return -1;
    }
    tinymt64_t tinymt;
    tinymt.mat1 = strtoul(argv[1], NULL, 16);
    tinymt.mat2 = strtoul(argv[2], NULL, 16);
    tinymt.tmat = strtoull(argv[3], NULL, 16);
    int seed = 1;
    uint64_t seed_array[5];
    if (argc >= 5) {
	seed = strtol(argv[4], NULL, 0);
    }
    printf("tinymt64 0x%08"PRIx32, tinymt.mat1);
    printf(" 0x%08"PRIx32, tinymt.mat2);
    printf(" 0x%016"PRIx64, tinymt.tmat);
    printf(" seed = %d\n", seed);
    tinymt64_init(&tinymt, seed);
    printf("64-bit unsigned integers r, where 0 <= r < 2^64\n");
    for (int i = 0; i < 10; i++) {
	for (int j = 0; j < 3; j++) {
	    printf("%20"PRIu64" ", tinymt64_generate_uint64(&tinymt));
	}
	printf("\n");
    }
    printf("init_by_array {%d}\n", seed);
    seed_array[0] = seed;
    tinymt64_init_by_array(&tinymt, seed_array, 1);
    printf("double numbers r, where 0.0 <= r < 1.0\n");
    for (int i = 0; i < 12; i++) {
	for (int j = 0; j < 4; j++) {
	    printf("%.15f ", tinymt64_generate_double(&tinymt));
	}
	printf("\n");
    }
    printf("double numbers r, where 1.0 <= r < 2.0\n");
    for (int i = 0; i < 12; i++) {
	for (int j = 0; j < 4; j++) {
	    printf("%.15f ", tinymt64_generate_double12(&tinymt));
	}
	printf("\n");
    }
    printf("double numbers r, where 0.0 < r <= 1.0\n");
    for (int i = 0; i < 12; i++) {
	for (int j = 0; j < 4; j++) {
	    printf("%.15f ", tinymt64_generate_doubleOC(&tinymt));
	}
	printf("\n");
    }
    printf("double numbers r, where 0.0 <= r < 1.0\n");
    for (int i = 0; i < 12; i++) {
	for (int j = 0; j < 4; j++) {
	    printf("%.15f ", tinymt64_generate_doubleOO(&tinymt));
	}
	printf("\n");
    }
}

