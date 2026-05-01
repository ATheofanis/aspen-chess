//
// Created by theoa on 21/03/2026.
//

#include "TranspositionTable.h"


// function to save current position to the transposition table (using depth and generation (age) replacement scheme)
void save(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, Bound bound, int entryGeneration, int ply)
{
    // get the index based on the zobrist hash
    int index = zobristHash & TTSizeMinusOne;


    TTEntry entry = transpositionTable[index];

    // adjust evaluation to handle checkmate correctly
    if (evaluation < -CHECKMATE + 500) evaluation -= ply;
    if (evaluation > CHECKMATE - 500) evaluation += ply;

    // replace the old entry if its depth is lower than the depth of the current entry
    // also replace if the old entry's generation is not the same as the current generation meaning it is from a prior search
    if (entry.entryDepth < depth || entry.entryGen != generation)
    {
        entries++;
        transpositionTable[index] = TTEntry(zobristHash, bestMove, evaluation, staticEval, depth, (uint8_t)bound, entryGeneration);
    }
}


// searches for an entry inside the transposition table
std::tuple<bool, TTData> probe(ZobristHash zobristHash)
{
    int index = zobristHash & TTSizeMinusOne;

    TTEntry entry = transpositionTable[index];

    // if it finds an entry it returns the entry's data by converting the TTEntry into a TTData struct
    // it also returns a bool which is true if it did find an entry and false if it did not
    if (entry.entryZobristKey == zobristHash)
    {
        return {true, entry.read()};
    }

    // no entry was found
    return {false, TTData(VALUE_NONE, VALUE_NONE, VALUE_NONE, NO_MOVE, Bound::BOUND_NONE)};
}


// ***** Pawn hash table functions *****


// save a pawn structure's evaluation to the pawn hash table
void savePawnHash(ZobristHash pawnZobristHash, int pawnsEvaluation)
{
    int index = pawnZobristHash & pawnTableSizeMinusOne;

    pawnHashTable[index] = pawnHashEntry(pawnZobristHash, pawnsEvaluation);
}


// probe pawn hashtable and return the eval of the pawn structure
int probePawnHash(ZobristHash pawnZobristHash)
{
    int index = pawnZobristHash & pawnTableSizeMinusOne;

    pawnHashEntry entry = pawnHashTable[index];
    if (entry.pawnZobristHash == pawnZobristHash)
    {
        return entry.pawnStructureEval;
    }

    return NO_HASH_ENTRY;
}