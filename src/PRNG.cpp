//
// Created by theoa on 19/03/2026.
//

#include "PRNG.h"

// xoshiro256** pseudo random number generator. Code from https://prng.di.unimi.it/xoshiro256starstar.c:



/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

static uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}


static uint64_t s[4];

uint64_t next() {
	const uint64_t result = rotl(s[1] * 5, 7) * 9;

	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}


// splitmix64 pseudo code: https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64

// for seeding we use splitmix64
static uint64_t splitmix64State = 12587915872;

// use splitmix64 generator for seeding (to fill s - state)
uint64_t splitmix64()
{
	splitmix64State += 0x9e3779b97f4a7c15;

	uint64_t z = splitmix64State;

	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9; /* xor the variable with the variable right bit shifted 30 then multiply by a constant */
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb; /* xor the variable with the variable right bit shifted 27 then multiply by a constant */

	return z ^ (z >> 31);					  /* return the variable xor'ed with itself right bit shifted 31 */
}


// seeding for xoshiro256** (s[4])
void seedingForXoshiro256aa()
{
	for (int i = 0; i < 4; i++)
	{
		s[i] = splitmix64();
	}
}

