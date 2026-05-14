//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

#include "nnue/Accumulator.h"

class Position;
class Accumulator;

inline int MAX_DEPTH = 64;

inline uint64_t MAX_NODES = std::numeric_limits<uint64_t>::max();

inline bool DataGenFlag = false;


// Search Stack to pass search-related information - work in progress
struct SearchStack
{
    Accumulator accumulator;
};


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



extern int precomputedLMR[128][256];


// the LMR formula is computed once at the beginning of the program to avoid calling std::log millions of times in the search
inline void initLMR() {
    // for every depth up to 128
    for (int d = 0; d < 128; d++) {
        // for every move up to 256
        for (int i = 0; i < 256; i++) {
            precomputedLMR[d][i] = 1 + std::log(d) * std::log(i) / 2.0;
        }
    }
}


Move findBestMove(Position pos, int& posEval);