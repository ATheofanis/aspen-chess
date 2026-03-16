//
// Created by theoa on 10/03/2026.
//

#include "Score.h"

int scoreBoard(const Position& pos)
{
    int score = 0;
    score += pos.getTotalPSTAndMaterialScore();
    return pos.isWhiteToMove() ? score : -score;
}