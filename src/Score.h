//
// Created by theoa on 10/03/2026.
//

#pragma once
#include "Position.h"
#include "Bitboard.h"
#include "nnue/Accumulator.h"

class Position;
class Accumulator;
class MoveSearcher;

extern int pawnHashHIT;
extern int pawnHashMISS;


constexpr int CHECKMATE = 32000;
constexpr int STALEMATE = 0;



constexpr int gamephaseInc[12] = {
// wp, wN, wB, wR, wQ, wK
    0,  1,  1,  2,  4, 0,
// bp, bN, bB, bR, bQ, bK
    0,  1,  1,  2,  4, 0
};



int scoreBoardNNUE(const Position& pos, Accumulator accumulator);

int scoreBoard(const Position& pos);