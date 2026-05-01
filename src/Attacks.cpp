//
// Created by theoa on 06/03/2026.
//

#include "Attacks.h"
#include "Position.h"

#include <cassert>

// precompute pawn attacks for each side
Bitboard pawnAttacks[2][64];
// precompute knight attacks for each square
Bitboard knightAttacks[64];
// precompute king attacks for each square
Bitboard kingAttacks[64];

// bishop attack masks
Bitboard bishopMasks[64];

// rook attack masks
Bitboard rookMasks[64];


// bishop attacks table [square][occupancies]
Bitboard bishopAttacksTable[64][512];

// rook attacks table [square][occupancies]
Bitboard rookAttacksTable[64][4096];


// call in main to initialize all pawn attacks from every location (square)
void initPawnAttacks()
{
    for (int sq = 0; sq < 64; sq++)
    {
        Bitboard pawnPos = 1ULL << sq;

        // white pawn
        Bitboard whiteLeftCapture = (pawnPos & ~fileA) << 7;
        Bitboard whiteRightCapture = (pawnPos & ~fileH) << 9;
        pawnAttacks[White][sq] = whiteLeftCapture | whiteRightCapture;

        // black pawn
        Bitboard blackLeftCapture = (pawnPos & ~fileA) >> 9;
        Bitboard blackRightCapture = (pawnPos & ~fileH) >> 7;
        pawnAttacks[Black][sq] = blackLeftCapture | blackRightCapture;
    }
}

// call in main to init knight attacks for the precomputed knight attacks table
void initKnightAttacks()
{
    for (int sq = 0; sq < 64; sq++)
    {
        Bitboard knightPos = 1ULL << sq;

        // compute all knight attacks from current square
        Bitboard ktAttacks = 0ULL;

        // NORTH-------------------------------

        //noWeWe
        ktAttacks |= (knightPos & ~fileAB) << 6;
        //noNoWe
        ktAttacks |= (knightPos & ~fileA) << 15;
        //noNoEa
        ktAttacks |= (knightPos & ~fileH) << 17;
        //noEaEa
        ktAttacks |= (knightPos & ~fileGH) << 10;

        // SOUTH------------------------------

        // soEaEa
        ktAttacks |= (knightPos & ~fileGH) >> 6;
        //soSoEa
        ktAttacks |= (knightPos & ~fileH) >> 15;
        // soSoWe
        ktAttacks |= (knightPos & ~fileA) >> 17;
        //soWeWe
        ktAttacks |= (knightPos & ~fileAB) >> 10;

        knightAttacks[sq] = ktAttacks;
    }
}

// call in main to init king attacks for the precomputed king attacks table
void initKingAttacks()
{
    for (int sq = 0; sq < 64; sq++)
    {
        Bitboard kingPos = 1ULL << sq;

        // compute all king attacks from current square
        Bitboard kngAttacks = 0ULL;


        // NORTH-------------------------------

        //noWe
        kngAttacks |= (kingPos & ~fileA) << 7;
        //noCenter
        kngAttacks |= kingPos << 8;
        //noEa
        kngAttacks |= (kingPos & ~fileH) << 9;

        // MIDDLE-------------------------------

        // west
        kngAttacks |= (kingPos & ~fileA) >> 1;
        // east
        kngAttacks |= (kingPos & ~fileH) << 1;


        // SOUTH------------------------------

        //soWe
        kngAttacks |= (kingPos & ~fileA) >> 9;
        //soCenter
        kngAttacks |= kingPos >> 8;
        //soEa
        kngAttacks |= (kingPos & ~fileH) >> 7;

        kingAttacks[sq] = kngAttacks;
    }
}

// for bishop magic bitboards without blockers or edges of the board
Bitboard bishopAttacksMask(int square)
{
    Bitboard attacks = 0ULL;

    // initialize ranks and files
    int rank, file;

    // initialize target rank and files
    int targetRank = square / 8;
    int targetFile = square % 8;

    // mask relevant bishop occupancy squares
    for (rank = targetRank + 1, file = targetFile + 1; rank <= 6 && file <= 6; rank++, file++)
    {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (rank = targetRank - 1, file = targetFile + 1; rank >= 1 && file <= 6; rank--, file++)
    {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (rank = targetRank + 1, file = targetFile - 1; rank <= 6 && file >= 1; rank++, file--)
    {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (rank = targetRank - 1, file = targetFile - 1; rank >= 1 && file >= 1; rank--, file--)
    {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;
}

// for rook magic bitboards without blockers or edges of the board
Bitboard rookAttacksMask(int square)
{
    Bitboard attacks = 0ULL;

    // initialize ranks and files
    int rank, file;

    // initialize target rank and files
    int targetRank = square / 8;
    int targetFile = square % 8;

    // mask relevant bishop occupancy squares
    for (rank = targetRank + 1; rank <= 6; rank++)
    {
        attacks |= (1ULL << (rank * 8 + targetFile));
    }
    for (rank = targetRank - 1; rank >= 1; rank--)
    {
        attacks |= (1ULL << (rank * 8 + targetFile));
    }
    for (file = targetFile + 1; file <= 6; file++)
    {
        attacks |= (1ULL << (targetRank * 8 + file));
    }
    for (file = targetFile - 1; file >= 1; file--)
    {
        attacks |= (1ULL << (targetRank * 8 + file));
    }

    return attacks;
}


// bishop attack with blocker (on the fly)
Bitboard bishopAttacks(int square, Bitboard block)
{
    Bitboard attacks = 0ULL;

    // initialize ranks and files
    int rank, file;

    // initialize target rank and files
    int targetRank = square / 8;
    int targetFile = square % 8;

    // generate bishop attacks
    for (rank = targetRank + 1, file = targetFile + 1; rank <= 7 && file <= 7; rank++, file++)
    {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & block) break;
    }
    for (rank = targetRank - 1, file = targetFile + 1; rank >= 0 && file <= 7; rank--, file++)
    {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & block) break;
    }
    for (rank = targetRank + 1, file = targetFile - 1; rank <= 7 && file >= 0; rank++, file--)
    {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & block) break;
    }
    for (rank = targetRank - 1, file = targetFile - 1; rank >= 0 && file >= 0; rank--, file--)
    {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & block) break;
    }

    return attacks;
}

// rook attack with blocker (on the fly)
Bitboard rookAttacks(int square, Bitboard block)
{
    Bitboard attacks = 0ULL;

    // initialize ranks and files
    int rank, file;

    // initialize target rank and files
    int targetRank = square / 8;
    int targetFile = square % 8;

    // generate rook attacks
    for (rank = targetRank + 1; rank <= 7; rank++)
    {
        attacks |= (1ULL << (rank * 8 + targetFile));
        if ((1ULL << (rank * 8 + targetFile)) & block) break;
    }

    for (rank = targetRank - 1; rank >= 0; rank--)
    {
        attacks |= (1ULL << (rank * 8 + targetFile));
        if ((1ULL << (rank * 8 + targetFile)) & block) break;
    }


    for (file = targetFile + 1; file <= 7; file++)
    {
        attacks |= (1ULL << (targetRank * 8 + file));
        if ((1ULL << (targetRank * 8 + file)) & block) break;
    }

    for (file = targetFile - 1; file >= 0; file--)
    {
        attacks |= (1ULL << (targetRank * 8 + file));
        if ((1ULL << (targetRank * 8 + file)) & block) break;
    }
    return attacks;
}


// sets the occupancy used in the making of magic bitboards
Bitboard setOccupancy(int index, int bitCountInMask, Bitboard attackMask)
{
    // Initialize the occupancy map
    Bitboard occupancy = 0ULL;



    for (int count = 0; count < bitCountInMask; count++)
    {

        int square = popLsbAndReturnIndex(attackMask);
        if (index & (1 << count))
        {
            occupancy |= (1ULL << square);
        }
    }


    return occupancy;
}


// initialize the bishop attacks table using magic numbers
void initBishopAttacks()
{
    for (int sq = 0; sq < 64; sq++)
    {
        bishopMasks[sq] = bishopAttacksMask(sq);
        int relevantBits = bitCount(bishopMasks[sq]);
        int numOccupancies = 1 << relevantBits;

        for (int index = 0; index < numOccupancies; index++)
        {
            Bitboard occupancy = setOccupancy(index, relevantBits, bishopMasks[sq]);
            uint64_t magicIndex = ((occupancy * bishopMagicNumbers[sq]) >> (64-bishopRelevantBits[sq]));
            bishopAttacksTable[sq][magicIndex] = bishopAttacks(sq, occupancy);
        }
    }
}

// initialize the rook attacks table using magic numbers
void initRookAttacks()
{
    for (int sq = 0; sq < 64; sq++)
    {
        rookMasks[sq] = rookAttacksMask(sq);
        int relevantBits = bitCount(rookMasks[sq]);
        int numOccupancies = 1 << relevantBits;

        assert(relevantBits == rookRelevantBits[sq]);

        for (int index = 0; index < numOccupancies; index++)
        {
            assert(relevantBits == rookRelevantBits[sq]);

            Bitboard occupancy = setOccupancy(index, relevantBits, rookMasks[sq]);
            uint64_t magicIndex = ((occupancy * rookMagicNumbers[sq]) >> (64-rookRelevantBits[sq]));
            rookAttacksTable[sq][magicIndex] = rookAttacks(sq, occupancy);
        }
    }
}


// checks if a given square is under attack
bool squareUnderAttack(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupied)
{

    // in some cases the correct occupied bitboard differs from pos.getOccupiedBitboard,
    // for example the previous position of the king remains in occupied squares causing bugs, so we manually pass the argument


    // delta is 6 for black and 0 for white for piece indecies
    int attackingSideDelta = (attackingSideColor == White) ? 0 : 6;

    Bitboard attackingQueen = pos.getPieceBitboard(attackingSideDelta + 4);


    // check rook and queen
    if ( getRookAttacks(targetSquare, occupied)  &  (pos.getPieceBitboard(attackingSideDelta + 3) | attackingQueen) )
    {
        return true;
    }
    // check bishop and queen
    if ( getBishopAttacks(targetSquare, occupied)  &  (pos.getPieceBitboard(attackingSideDelta + 2) | attackingQueen) )
    {
        return true;
    }
    // check knights
    if ( knightAttacks[targetSquare]  &  pos.getPieceBitboard(attackingSideDelta + 1) )
    {
        return true;
    }
    // for enemy pawns to check if a white piece is under attack we can check the attacking squares of a white pawn that would be in the target square's position
    // then if a black pawn is in that position the square is under attack
    if ( pawnAttacks[(attackingSideColor == White) ? Black : White][targetSquare] &  pos.getPieceBitboard(attackingSideDelta))
    {
        return true;
    }


    if ( kingAttacks[targetSquare]  &  pos.getPieceBitboard(attackingSideDelta + 5) )
    {
        return true;
    }


    return false;
}


// this function checks wether or not a square is under attack by a rook (after enpassant for faster legality checks)
bool squareUnderAttackByRookPiece(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupied)
{

    int attackingSideDelta = (attackingSideColor == White) ? 0 : 6;


    // check rook and queen
    if ( getRookAttacks(targetSquare, occupied)  &  (pos.getPieceBitboard(attackingSideDelta + 3) | (pos.getPieceBitboard(attackingSideDelta + 4))))
    {
        return true;
    }

    return false;
}

// this function checks wether or not a square is under attack by a slider piece (after enpassant for faster legality checks)
bool squareUnderAttackBySliderPiece(int targetSquare, Color attackingSideColor, const Position& pos, Bitboard occupied)
{

    int attackingSideDelta = (attackingSideColor == White) ? 0 : 6;


    Bitboard attackingQueen = pos.getPieceBitboard(attackingSideDelta + 4);


    // check rook and queen
    if ( getRookAttacks(targetSquare, occupied)  &  (pos.getPieceBitboard(attackingSideDelta + 3) | attackingQueen) )
    {
        return true;
    }
    // check bishop and queen
    if ( getBishopAttacks(targetSquare, occupied)  &  (pos.getPieceBitboard(attackingSideDelta + 2) | attackingQueen) )
    {
        return true;
    }

    return false;
}
