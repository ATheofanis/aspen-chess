//
// Created by theoa on 21/03/2026.
//

#include "TranspositionTable.h"


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



// function to save current position to the transposition table (using depth replacement scheme)
void save(ZobristHash zobristHash, Move bestMove, int evaluation, int staticEval, int depth, Bound bound, int entryGeneration, int ply)
{
    // bitwise AND operation trick for powers of two

    int index = zobristHash & TTSizeMinusOne;


    TTEntry entry = transpositionTable[index];

    if (evaluation < -CHECKMATE + 500) evaluation -= ply;
    if (evaluation > CHECKMATE - 500) evaluation += ply;


    if (entry.entryDepth < depth || entry.entryGen != generation)
    {
        entries++;
        //TTEntry newEntry = TTEntry(zobristHash, depth, evaluation, bound, bestMove);
        transpositionTable[index] = TTEntry(zobristHash, bestMove, evaluation, staticEval, depth, (uint8_t)bound, entryGeneration);
    }
}



std::tuple<bool, TTData> probe(ZobristHash zobristHash)
{
    int index = zobristHash & TTSizeMinusOne;

    TTEntry entry = transpositionTable[index];

    if (entry.entryZobristKey == zobristHash)
    {
        return {true, entry.read()};
    }

    return {false, TTData(VALUE_NONE, VALUE_NONE, VALUE_NONE, NO_MOVE, Bound::BOUND_NONE)};
}


/*
// function to search for an entry, returns evaluation or alpha and beta to cause cutoffs in search
// a best move variable is passed by reference to be updated inside the probe in case of a hit
std::tuple<bool, TTData> probe(int depth, ZobristHash zobristHash, int beta, int alpha, Move& bestMove, int ply)
{
    // bitwise AND operation trick for powers of two
    int index = zobristHash & TTSizeMinusOne;



    //std::cout << "INDEX:" << index << std::endl;
    TTEntry entry = transpositionTable[index];



    if (entry.entryZobristKey == zobristHash)
    {
        //std::cout << "MATCHED ZOBRIST" << std::endl;
        bestMove = entry.entryBestMove;
        if (entry.entryDepth >= depth)
        {
            int eval = entry.entryEvaluation;
            // retrieve score independent of the actual path
            if (eval < -CHECKMATE + 500) eval += ply;
            if (eval > CHECKMATE - 500) eval -= ply;

            // exact flag
            if ((Bound)entry.entryBound == Bound::BOUND_EXACT)
            {
                return eval;
            }
            // Alpha flag
            if ((Bound)entry.entryBound == Bound::BOUND_ALPHA && eval <= alpha)
            {
                return alpha;
            }
            // beta flag
            if ((Bound)entry.entryBound == Bound::BOUND_BETA && eval >= beta)
            {
                return beta;
            }
        }
    }

    return NO_HASH_ENTRY;

}
*/