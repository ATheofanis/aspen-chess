//
// Created by theoa on 21/03/2026.
//

#include "TranspositionTable.h"
#include <bit>


// function to save current position to the transposition table (using depth and generation (age) replacement scheme)
void save(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, Bound bound, int entryGeneration, int isPv, int ply)
{
    // get the index based on the zobrist hash
    int index = zobristHash & (TTSize - 1);


    TTEntry entry = transpositionTable[index];

    // adjust evaluation to handle checkmate correctly
    if (evaluation < -CHECKMATE + 500) evaluation -= ply;
    if (evaluation > CHECKMATE - 500) evaluation += ply;

    // Preserve the best move if the node failed-low (bestMove is NO_MOVE)
    if (bestMove == NO_MOVE && entry.entryZobristKey == zobristHash)
    {
        bestMove = entry.entryBestMove;
    }

    // replace the old entry if its depth is lower than the depth of the current entry
    // also replace if the old entry's generation is not the same as the current generation meaning it is from a prior search
    if (entry.entryDepth < depth || entry.entryGen != generation)
    {
        transpositionTable[index] = TTEntry(zobristHash, bestMove, evaluation, staticEval, depth, (uint8_t)bound, entryGeneration, isPv);
    }
}


// searches for an entry inside the transposition table
std::tuple<bool, TTData> probe(ZobristHash zobristHash)
{
    size_t index = TTIndex(zobristHash);

    TTEntry entry = transpositionTable[index];

    // if it finds an entry it returns the entry's data by converting the TTEntry into a TTData struct
    // it also returns a bool which is true if it did find an entry and false if it did not
    if (entry.entryZobristKey == zobristHash)
    {
        return {true, entry.read()};
    }

    // no entry was found
    return {false, TTData(VALUE_NONE, VALUE_NONE, VALUE_NONE, NO_MOVE, Bound::BOUND_NONE, 0)};
}


// Resizes the transposition table and clears it
void resizeTranspositionTable(int megabytes)
{
    if (transpositionTable != nullptr)
    {
        delete[] transpositionTable;
    }

    // Set the megabytes to a positive value
    megabytes = std::max(1, megabytes);

    // The new TT size is the total megabytes divided by the size of the transposition table entry
    size_t newTTSize = megabytes * 1024ULL * 1024ULL / sizeof(TTEntry);

    // Set the number of entries to the closest power of two for fast index calculations
    newTTSize = std::bit_floor((uint32_t)newTTSize);
    TTSize = newTTSize;

    transpositionTable = new TTEntry[TTSize];

    clearTranspositionTable();
}
