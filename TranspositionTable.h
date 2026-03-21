//
// Created by theoa on 21/03/2026.
//

#pragma once
#include "Types.h"

constexpr long long TTSize = 1 << 20;


struct TTEntry
{
    ZobristHash entryZobristKey;
    int entryDepth;
    int entryAge;
    Move entryBestMove;
    Bound entryBound;
};



