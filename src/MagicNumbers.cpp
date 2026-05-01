//
// Created by theoa on 06/03/2026.
//

#include "MagicNumbers.h"

#include <cstdlib>
#include <cstring>

#include "Attacks.h"

// https://www.chessprogramming.org/Looking_for_Magics
// Tom Romstad's proposal to find magics:

uint32_t randomState = 1804289383;

uint32_t random_uint32()
{
    uint32_t number = randomState;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    randomState = number;

    return randomState;
}


uint64_t random_uint64()
{
    uint64_t n1, n2, n3, n4;

    n1 = (uint64_t)(random_uint32()) & 0xFFFF;
    n2 = (uint64_t)(random_uint32()) & 0xFFFF;
    n3 = (uint64_t)(random_uint32()) & 0xFFFF;
    n4 = (uint64_t)(random_uint32()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}



uint64_t findMagicNumber(int square, int relevantBits, int bishop)
{
    // initialize occupancies
    uint64_t occupancies[4096];

    // initialize attack tables
    uint64_t attacks[4096];

    // initialize used attacks
    uint64_t used[4096];

    uint64_t attackMask = bishop ? bishopAttacksMask(square) : rookAttacksMask(square);

    int occupancyIndecies = 1 << relevantBits;

    for (int index = 0; index < occupancyIndecies; index++)
    {
        occupancies[index] = setOccupancy(index, relevantBits, attackMask);

        attacks[index] = bishop ? bishopAttacks(square, occupancies[index]) : rookAttacks(square, occupancies[index]);
    }

    for (int randomCount = 0; randomCount < 100000000; randomCount++)
    {

        // generate magic number candidate
        uint64_t magicNumber = generateMagicNumber();

        // skip inappropriate numbers
        if (bitCount((attackMask * magicNumber) & 0xFF00000000000000) < 6) continue;

        // init used attacks
        memset(used, 0ULL, sizeof(used));

        // init index and fail flag
        int index, fail;

        // test magic index loop
        for (index = 0, fail = 0; !fail && index < occupancyIndecies; index++)
        {
            int magicIndex = (int)((occupancies[index] * magicNumber) >> (64 - relevantBits));

            // if magic index works
            if (used[magicIndex] == 0ULL)
            {
                // initialize used attacks
                used[magicIndex] = attacks[index];
            }
            else if (used[magicIndex] != attacks[index])
            {
                fail = 1;
            }
        }
        // if magic number works
        if (!fail) return magicNumber;
    }
    // if magic number doesnt work
    std::cout << "Magic number fails\n";
    return 0ULL;
}

// init magic numbers
void initMagicNumbers()
{
    std::cout << "ROOK:\n";
    // loop over 64 board squares
    for (int sq = 0; sq < 64 ; sq++)
    {
        // init rook
        if (sq % 4 == 0)
        {
            std::cout << "\n";
        }
        std::cout << "0x" << std::hex << findMagicNumber(sq, rookRelevantBits[sq], rook) << "ULL, ";
    }

    std::cout << "\n\n";
    std::cout << "BISHOP:\n";
    // loop over 64 board squares
    for (int sq = 0; sq < 64 ; sq++)
    {
        // init bishop
        if (sq % 4 == 0)
        {
            std::cout << "\n";
        }
        std::cout << "0x" << std::hex << findMagicNumber(sq, bishopRelevantBits[sq], bishop) << "ULL, ";
    }
}


