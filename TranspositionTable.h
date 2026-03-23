//
// Created by theoa on 21/03/2026.
//

#pragma once
#include <cstring>

#include "Types.h"

inline int entries = 0;

constexpr int NO_HASH_ENTRY = -1000000;

// the transposition table contains hash entries that have specific information stored about a position
struct TTEntry
{
    ZobristHash entryZobristKey = 0; // the position's zobrist key
    int entryDepth = -1;              // the current depth when the position was searched
    int entryEvaluation = 0;         // the evaluation of the position
    Bound entryBound = Bound::BOUND_EXACT;              // EXACT - BOUND_EXACT(0)  /  UPPERBOUND - BOUND_ALPHA(1)  /  LOWERBOUND - BOUND_BETA(2)
    Move entryBestMove = 0;          // the best move of the position

    TTEntry() = default; // default constructor

    TTEntry(ZobristHash zobristHash, int depth, int evaluation, Bound bound, Move bestMove)
    {
        entryZobristKey = zobristHash;
        entryDepth = depth;
        entryEvaluation = evaluation;
        entryBound = bound;
        entryBestMove = bestMove;
    }
};


// the size of the transposition table
constexpr int TTSize = 1 << 20;

// the TTSize is a power of 2. therefore in order to calculate the index of an entry which is zobrist % TTSize we can use bitwise AND like so:
// index = zobrist & TTSize-1
constexpr int TTSizeMinusOne = TTSize - 1;


// global transposition table
inline TTEntry transpositionTable[TTSize];

// save a position to the tt, or overwrite an existing hash entry
void save(ZobristHash zobristHash, int depth, int evaluation, Bound bound, Move bestMove);

// looks up a position in the transposition table and returns the entry if it exists
int probe(int depth, ZobristHash zobristHash, int beta, int alpha, Move& bestMove);


// function to clear transposition table
inline void clearTranspositionTable()
{
    memset(transpositionTable, 0, sizeof(TTEntry) * TTSize);
}



