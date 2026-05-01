//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

class Position;

inline int MAX_DEPTH = 64;



// Move picker class - to be continued
class MovePicker
{
public:
    Move nextMove();
    MovePicker() = default;
private:
    Move moves[256];
    Move ttMove;
};



extern int precomputedLMR[64][256];


// the LMR formula is computed once at the beginning of the program to avoid calling std::log millions of times in the search
inline void initLMR() {
    // for every depth up to 64
    for (int d = 0; d < 64; d++) {
        // for every move up to 256
        for (int i = 0; i < 256; i++) {
            precomputedLMR[d][i] = 1 + std::log(d) * std::log(i) / 2.0;
        }
    }
}


Move findBestMove(Position pos);