//
// Created by theoa on 06/03/2026.
//
#pragma once
#include <bit>

#include "Types.h"

// files (columns)
constexpr Bitboard fileA = 0x0101010101010101ULL;
constexpr Bitboard fileB = 0x0202020202020202ULL;
constexpr Bitboard fileC = 0x0404040404040404ULL;
constexpr Bitboard fileD = 0x0808080808080808ULL;
constexpr Bitboard fileE = 0x1010101010101010ULL;
constexpr Bitboard fileF = 0x2020202020202020ULL;
constexpr Bitboard fileG = 0x4040404040404040ULL;
constexpr Bitboard fileH = 0x8080808080808080ULL;

// file combinations for knight attacks table
constexpr Bitboard fileGH = 0xC0C0C0C0C0C0C0C0ULL;
constexpr Bitboard fileAB = 217020518514230019;

// ranks (rows)
constexpr Bitboard rank1 = 0x00000000000000FFULL;
constexpr Bitboard rank2 = 0x000000000000FF00ULL;
constexpr Bitboard rank3 = 0x0000000000FF0000ULL;
constexpr Bitboard rank4 = 0x00000000FF000000ULL;
constexpr Bitboard rank5 = 0x000000FF00000000ULL;
constexpr Bitboard rank6 = 0x0000FF0000000000ULL;
constexpr Bitboard rank7 = 0x00FF000000000000ULL;
constexpr Bitboard rank8 = 0xFF00000000000000ULL;


constexpr Bitboard wkRookMask = 128;
constexpr Bitboard wqRookMask = 1;
constexpr Bitboard bkRookMask = 0x8000000000000000ULL;
constexpr Bitboard bqRookMask = 0x100000000000000ULL;


constexpr Bitboard wkRookMoveMask = 160;
constexpr Bitboard wqRookMoveMask = 9;
constexpr Bitboard bkRookMoveMask = 0xA000000000000000ULL;
constexpr Bitboard bqRookMoveMask = 0x900000000000000ULL;

constexpr Bitboard e1Mask = 16;
constexpr Bitboard e8Mask = 1152921504606846976;


// for legality checks, returns a bitboard which marks every square between two given squares, including the squares themselves
extern Bitboard lineBetween[64][64];


// returns how many 1's are in a bitboard
constexpr int bitCount(Bitboard bb)
{
    return __builtin_popcountll(bb);
}

// returns the index of least valuable bit without modifying the bitboard
constexpr int lsbIndex(Bitboard bb)
{
    return __builtin_ctzll(bb);
}

// returns the index of least valuable bit and pops it from the bitboard 
//constexpr int popLsbAndReturnIndex(Bitboard &b)
//{
//    int index = std::countr_zero(b);
//    b &= b - 1;
//    return index;
//}

// returns the index of least valuable bit and pops it from the bitboard -- FOR COMPILERS WITHOUT C++23
constexpr int popLsbAndReturnIndex(Bitboard &b)
{
    int index = __builtin_ctzll(b);

    b &= b - 1;
    
    return index;
}



// returns true if bit is 1 and false if 0
constexpr bool getBit(Bitboard bb, int square)
{
    return (bb & (1ULL << square));
}

// set a bit to 1
constexpr void setBit(Bitboard& bb, int square)
{
    bb |= (1ULL << square);
}

// removes a bit (sets to 0) if it is 1
constexpr void popBit(Bitboard& bb, int square)
{
    if (getBit(bb, square))
    {
        bb ^= (1ULL << square);
    }
}


void initLineBetween();


void printBitboard(Bitboard bb);