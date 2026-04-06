//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

class Position;

inline int MAX_DEPTH = 15;
//constexpr int MAX_PLY = 30;

//extern Move previousMoves[MAX_PLY];

class MovePicker
{
public:
    Move nextMove();
    MovePicker() = default;
private:
    Move moves[256];
    Move ttMove;
};


extern int precomputedLMR[32][256];

inline void initLMR() {
    for (int d = 0; d < 32; d++) {
        for (int i = 0; i < 256; i++) {
            precomputedLMR[d][i] = 1 + std::log(d) * std::log(i) / 2.0;
        }
    }
}


Move findBestMove(Position pos);