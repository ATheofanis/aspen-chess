//
// Created by theoa on 19/03/2026.
//

#pragma once
#include <cstdint>

// Xoshiro256** pseudo random number generator for Zobrist Hash initialization
// Check out: https://prng.di.unimi.it/xoshiro256starstar.c
uint64_t next();

// Seeding function for xoshiro256**
void seedingForXoshiro256aa();
