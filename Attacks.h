//
// Created by theoa on 06/03/2026.
//

#pragma once
#include <cassert>

#include "MagicNumbers.h"
#include "Position.h"

// initialize pawn attacks table
extern Bitboard pawnAttacks[2][64];
// initialize knight attacks table
extern Bitboard knightAttacks[64];
// initialize king attacks table
extern Bitboard kingAttacks[64];

// bishop attack masks
extern Bitboard bishopMasks[64];

// rook attack masks
extern Bitboard rookMasks[64];


// bishop attacks table [square][occupancies]
extern Bitboard bishopAttacksTable[64][512];

// rook attacks table [square][occupancies]
extern Bitboard rookAttacksTable[64][4096];


// on the fly , for magics
Bitboard bishopAttacks(int square, Bitboard block);

// get bishop attacks based on board occupancy
[[nodiscard]] inline Bitboard getBishopAttacks(int square, Bitboard occupancy)
{

    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopRelevantBits[square];



    return bishopAttacksTable[square][occupancy];
}

Bitboard rookAttacksMask(int square);

// on the fly , for magics
Bitboard rookAttacks(int square, Bitboard block);

// get rook attacks based on board occupancy
[[nodiscard]] inline Bitboard getRookAttacks(int square, Bitboard occupancy)
{

    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookRelevantBits[square];


    return rookAttacksTable[square][occupancy];
}

// to get the queen attacks just combine rook and bishop magic
[[nodiscard]] inline Bitboard getQueenAttacks(int square, Bitboard occupancy)
{
    return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
}



// initialize all possible bishop attacks/moves for every possible occupancy variation
void initBishopAttacks();

Bitboard bishopAttacksMask(int square);




// initialize all possible rook attacks/moves for every possible occupancy variation
void initRookAttacks();

Bitboard rookAttacksMask(int square);






Bitboard setOccupancy(int index, int bitCountInMask, Bitboard attackMask);



// precompute pawn attacks
void initPawnAttacks();

// precompute knight attacks
void initKnightAttacks();

// precompute king attacks
void initKingAttacks();

// check if a given square is under attack by the pieces of the attacking side in a given position
bool squareUnderAttack(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupancy);

// only checks if square is under attack by rook piece (rook, queen) for fast en passant legality checks
bool squareUnderAttackByRookPiece(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupied);

// to check legality of en passant when there is a single check
bool squareUnderAttackBySliderPiece(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupied);

