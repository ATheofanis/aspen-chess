//
// Created by theoa on 18/03/2026.
//

#include "Zobrist.h"

#include <random>

#include "Position.h"


ZobristHash zobristPieces[64][12];
ZobristHash zobristCastleRights[4];
ZobristHash zobristEnpassantFile[8];
ZobristHash zobristBlackToMove;


ZobristHash zobristRandom(const std::mt19937_64& prng)
{


   // return (prng.);
}


// initialize 64 squares for every piece type (12) with a random 64bit number, same for :
// 4 numbers to indicate castling rights one for each type (WK, WQ, BK, BQ)
// 8 numbers for every file to indicate that enpassant square is in that file
// 1 number to indicate that it is black's turn to move
void initZobrist()
{
    //std::mt19937_64 prng;
    //prng.seed(1151462515);
//
    //for (int i = 0; i < 64; i++)
    //{
    //    for (int j = 0; j < 12; j++)
    //    {
    //        zobristPieces[i][j] = std::mt19937_64();
    //    }
    //}
//
    //for (int i = 0; i < 4; i++)
    //{
    //    zobristCastleRights[i] = std::mt19937_64();
    //}
//
    //for (int i = 0; i < 8; i++)
    //{
    //    zobristEnpassantFile[i] = std::mt19937_64();
    //}
//
    //zobristBlackToMove = std::mt19937_64();
}


