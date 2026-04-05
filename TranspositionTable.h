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

// the size of the pawn structure transposition table
constexpr int pawnTableSize = 1 << 16;
constexpr int pawnTableSizeMinusOne = pawnTableSize - 1;

// Hashtable entry for pawns
struct pawnHashEntry
{
    ZobristHash pawnZobristHash = 0;
    int16_t pawnStructureEval = 0;

    pawnHashEntry() = default;

    pawnHashEntry(ZobristHash pawnZobrist, int pawnEval)
    {
        pawnZobristHash = pawnZobrist;
        pawnStructureEval = pawnEval;
    }
};


// global transposition table for pawns
inline pawnHashEntry pawnHashTable[pawnTableSize];


// save a position to the tt, or overwrite an existing hash entry
void savePawnHash(ZobristHash pawnZobristHash, int pawnsEvaluation);

// looks up a position in the transposition table and returns the entry if it exists
int probePawnHash(ZobristHash pawnZobrist);

// function to clear pawn transposition table
inline void clearPawnTranspositionTable()
{
    memset(pawnHashTable, 0, sizeof(pawnHashEntry) * pawnTableSize);
}


// the probe function returns the data extracted from the tt entry in a TTData struct that contains only the info we need for the search later on
struct TTData
{
    int evaluation;
    int staticEval;
    int depth;
    Move bestMove;
    Bound bound;

    TTData() = default;

    TTData(int eval, int staticEv, int dpth, int bMove, Bound bnd)
    {
        evaluation = eval;
        staticEval = staticEv;
        depth = dpth;
        bestMove = bMove;
        bound = bnd;
    }
};


// the transposition table contains hash entries that have specific information stored about a position
struct TTEntry
{
    ZobristHash entryZobristKey = 0; // the position's zobrist key
    Move entryBestMove = 0;          // the best move of the position
    int16_t entryEvaluation = 0;     // the evaluation of the position
    int16_t entryStaticEval = 0;
    uint8_t entryDepth = -1;              // the current depth when the position was searched
    uint8_t entryBound : 2;
    uint8_t entryGen : 6;

    TTEntry() = default; // default constructor

    TTEntry(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, int bound, int entryGeneration)
    {
        entryZobristKey = zobristHash;
        entryBestMove = bestMove;
        entryEvaluation = evaluation;
        entryStaticEval = staticEval;
        entryDepth = depth;
        entryBound = bound;
        entryGen = entryGeneration;
    }

    TTData read() const
    {
        return TTData(entryEvaluation, entryStaticEval, entryDepth, entryBestMove, (Bound)entryBound);
    }
};



// the size of the transposition table
constexpr int TTSize = 1 << 21;

// the TTSize is a power of 2. therefore in order to calculate the index of an entry which is zobrist % TTSize we can use bitwise AND like so:
// index = zobrist & TTSize-1
constexpr int TTSizeMinusOne = TTSize - 1;


// global transposition table
inline TTEntry transpositionTable[TTSize];

// save a position to the tt, or overwrite an existing hash entry
void save(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, Bound bound, int generation, int ply);

// looks up a position in the transposition table and returns the entry if it exists
std::tuple<bool, TTData> probe(ZobristHash zobristHash);


// function to clear transposition table
inline void clearTranspositionTable()
{
    memset(transpositionTable, 0, sizeof(TTEntry) * TTSize);
}




