//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include "MoveGen.h"
#include "Score.h"

int nodes = 0;

// takes a position, alpha and beta for pruning, current depth, keeps track of best move found, and ply for stuff such as killer moves, checkmate detection and for cleaner design
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply)
{
    // call quie at leaf nodes
    nodes++;

    if (depth == 0) return scoreBoard(pos);


    // generate the moves by first extracting the legality info
    Move moves[256];
    int numOfMoves = 0;


    Color allyColor;
    int kingSquare;
    // store ally color and king location for legality info
    if (pos.isWhiteToMove())
    {
        allyColor = White;
        kingSquare = lsbIndex(pos.getPieceBitboard(wK));
    }
    else
    {
        allyColor = Black;
        kingSquare = lsbIndex(pos.getPieceBitboard(bK));
    }
    legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);


    // generate legal moves using previously calculated legality info
    generateLegalMoves(info, pos, moves, numOfMoves);

    if (numOfMoves == 0)
    {
        if (info.numOfChecks)
        {
            return ply - CHECKMATE;
        }

        return 0;
    }

    for (int i = 0; i < numOfMoves; i++)
    {

        pos.makeMove(moves[i]);
        int score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1);
        pos.unmakeMove();
        //std::cout << "score: " << score << std::endl;

        if (score >= beta)
        {
            return beta; // hard beta cutoff
        }


        if (score > alpha)
        {
            alpha = score;
            if (depth == MAX_DEPTH)
            {
                bestMove = moves[i];
            }
        }


    }
    return alpha;
}


Move findBestMove(Position& pos)
{
    Move bestMove = 0;
    negaMaxAlphaBeta(pos, -CHECKMATE, CHECKMATE, MAX_DEPTH, bestMove, 0);

    std::cout << "Nodes: " << nodes << std::endl;
    return bestMove;

}