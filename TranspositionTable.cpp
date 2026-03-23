//
// Created by theoa on 21/03/2026.
//

#include "TranspositionTable.h"



// function to save current position to the transposition table (using depth replacement scheme)
void save(ZobristHash zobristHash, int depth, int evaluation, Bound bound, Move bestMove)
{
    // bitwise AND operation trick for powers of two

    int index = zobristHash & TTSizeMinusOne;


    TTEntry entry = transpositionTable[index];


    // depth replacement scheme (replace entries if the entry's depth is lower than current depth)
    if (entry.entryDepth < depth)
    {
        //std::cout << "SAVING ENTRY" << std::endl;
        entries++;
        TTEntry newEntry = TTEntry(zobristHash, depth, evaluation, bound, bestMove);
        //std::cout << "NEW ENTRY ZOBRIST:" << newEntry.entryZobristKey << std::endl;
        transpositionTable[index] = newEntry;
    }
}


// function to search for an entry, returns evaluation or alpha and beta to cause cutoffs in search
// a best move variable is passed by reference to be updated inside the probe in case of a hit
int probe(int depth, ZobristHash zobristHash, int beta, int alpha, Move& bestMove)
{
    // bitwise AND operation trick for powers of two
    int index = zobristHash & TTSizeMinusOne;



    //std::cout << "INDEX:" << index << std::endl;
    TTEntry entry = transpositionTable[index];


    //ZobristHash entryZobr = entry.entryZobristKey;
    //if (entryZobr)
    //{
    //    std::cout << "ENTRY ZOBRIST: " << entryZobr << std::endl;
    //    std::cout << "MATCHED ZOBRIST:" << zobristHash << std::endl;
    //}



    if (entry.entryZobristKey == zobristHash)
    {
        //std::cout << "MATCHED ZOBRIST" << std::endl;
        bestMove = entry.entryBestMove;
        if (entry.entryDepth >= depth)
        {
            // exact flag
            if (entry.entryBound == Bound::BOUND_EXACT)
            {
                return entry.entryEvaluation;
            }
            // Alpha flag
            if (entry.entryBound == Bound::BOUND_ALPHA && entry.entryEvaluation <= alpha)
            {
                return alpha;
            }
            // beta flag
            if (entry.entryBound == Bound::BOUND_BETA && entry.entryEvaluation >= beta)
            {
                return beta;
            }
        }
    } else
    {
        //std::cout << "ENTRY ZOBRIST: " << entry.entryZobristKey << std::endl;
        //std::cout << "CURRENT ZOBRIST" << zobristHash << std::endl;
    }

    return NO_HASH_ENTRY;

}