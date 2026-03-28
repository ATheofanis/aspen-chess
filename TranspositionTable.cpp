//
// Created by theoa on 21/03/2026.
//

#include "TranspositionTable.h"



// function to save current position to the transposition table (using depth replacement scheme)
void save(ZobristHash zobristHash, int depth, int evaluation, Bound bound, Move bestMove, int ply)
{
    // bitwise AND operation trick for powers of two

    int index = zobristHash & TTSizeMinusOne;


    TTEntry entry = transpositionTable[index];

    if (evaluation < -CHECKMATE + 500) evaluation -= ply;
    if (evaluation > CHECKMATE - 500) evaluation += ply;


    if (entry.entryDepth < depth)
    {
        entries++;
        //TTEntry newEntry = TTEntry(zobristHash, depth, evaluation, bound, bestMove);
        transpositionTable[index] = TTEntry(zobristHash, depth, evaluation, bound, bestMove);
    }



}


// function to search for an entry, returns evaluation or alpha and beta to cause cutoffs in search
// a best move variable is passed by reference to be updated inside the probe in case of a hit
int probe(int depth, ZobristHash zobristHash, int beta, int alpha, Move& bestMove, int ply)
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
            if (entry.entryBound == Bound::BOUND_EXACT)
            {
                return eval;
            }
            // Alpha flag
            if (entry.entryBound == Bound::BOUND_ALPHA && eval <= alpha)
            {
                return alpha;
            }
            // beta flag
            if (entry.entryBound == Bound::BOUND_BETA && eval >= beta)
            {
                return beta;
            }
        }
    }

    return NO_HASH_ENTRY;

}