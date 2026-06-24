//
// Created by theoa on 21/03/2026.
//

#pragma once
#include <cstring>

#include "Score.h"
#include "Types.h"

inline int entries = 0;

constexpr int NO_HASH_ENTRY = -64000;

inline int generation = 0;


// the probe function returns the data extracted from the tt entry in a TTData struct that contains only the info we need for the search later on
struct TTData
{
    int evaluation;
    int staticEval;
    int depth;
    Move bestMove;
    Bound bound;
    int isPv;

    TTData() = default;

    TTData(int eval, int staticEv, int dpth, int bMove, Bound bnd, int pv)
    {
        evaluation = eval;
        staticEval = staticEv;
        depth = dpth;
        bestMove = bMove;
        bound = bnd;
        isPv = pv;
    }
};


// the transposition table contains hash entries that have specific information stored about a position
struct alignas(16) TTEntry
{
    ZobristHash entryZobristKey = 0; // the position's zobrist key
    Move entryBestMove = 0;          // the best move of the position
    int16_t entryEvaluation = 0;     // the evaluation of the position
    int16_t entryStaticEval = 0;     // the static eval of the position
    uint8_t entryDepth = 0;         // the current depth when the position was searched
    uint8_t entryBound : 2;          // 2 bits for the bound of this entry
    uint8_t entryGen : 5;            // the other 6 bits for the entry's generation (age)
    uint8_t entryIsPv : 1;           // Flag for PV TT nodes

    TTEntry() = default; // default constructor

    TTEntry(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, int bound, int entryGeneration, int isPv)
    {
        entryZobristKey = zobristHash;
        entryBestMove = bestMove;
        entryEvaluation = evaluation;
        entryStaticEval = staticEval;
        entryDepth = depth;
        entryBound = bound;
        entryGen = entryGeneration;
        entryIsPv = isPv;
    }

    // returns a TTData struct from the current TTEntry
    TTData read() const
    {
        return TTData(entryEvaluation, entryStaticEval, entryDepth, entryBestMove, (Bound)entryBound, entryIsPv);
    }
};



// The size of the transposition table
// Default size is set at 64 MB
inline size_t TTSize = (64 * 1024 * 1024) / sizeof(TTEntry);

// global transposition table
inline TTEntry* transpositionTable = nullptr;

// save a position to the tt, or overwrite an existing hash entry
void save(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, Bound bound, int generation, int isPv, int ply);

// looks up a position in the transposition table and returns the entry if it exists
std::tuple<bool, TTData> probe(ZobristHash zobristHash);


// function to clear transposition table
inline void clearTranspositionTable()
{
    memset(transpositionTable, 0, sizeof(TTEntry) * TTSize);
}


void resizeTranspositionTable(int megabytes);


inline size_t TTIndex(ZobristHash zobrist)
{
    return zobrist & (TTSize - 1);
}


inline void TTPrefetch(ZobristHash zobrist) {
    __builtin_prefetch(&transpositionTable[TTIndex(zobrist)]);
}