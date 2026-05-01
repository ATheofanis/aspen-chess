//
// Created by theoa on 19/03/2026.
//

#pragma once
#include <cstdint>

// xoshiro256** pseudo random number generator to initialize zobrist hash
uint64_t next();

// seeding for xoshiro256** (s[4])
void seedingForXoshiro256aa();
