//
// Created by theoa on 10/03/2026.
//

#include "Score.h"
#include "TranspositionTable.h"


// NNUE evaluation of the position
int scoreBoardNNUE(const Position& pos, Accumulator accumulator)
{
    // Call the NNUE evaluation function
    int NNUEscore = evaluateNNUE(accumulator, pos.isWhiteToMove() ? White : Black);

    return NNUEscore;
}
