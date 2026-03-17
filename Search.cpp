//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include "MoveGen.h"
#include "Score.h"

int nodes = 0;


int quiescence(Position& pos, int alpha, int beta)
{
    nodes++;
    // stand pat
    int standPat = scoreBoard(pos);

    if (standPat >= beta)
    {
        return beta;
    }
    if (standPat > alpha)
    {
        alpha = standPat;
    }

    Move captures[128];
    int numOfCaps = 0;


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

    // FOR LATER: ------------------
    // maybe later: if king in check gen legal, otherwise gen captures and change capture generator condition so that it doesnt have the if king in check condtion
    // FOR LATER: ------------------

    // generate capture moves using previously calculated legality info
    generateCaptures(info, pos, captures, numOfCaps);

    //int maxScore = -10000;
    //int moveScores[numOfCaps];
//
    //for (int i = 0; i < numOfCaps; i++)
    //{
    //    moveScores[i] = scoreQuiescenceMove(captures[i], pos);
    //}
    std::sort(captures, captures + numOfCaps, [&pos](Move a, Move b)
    {
        return scoreQuiescenceMove(a, pos) > scoreQuiescenceMove(b, pos);
    });

    for (int i = 0; i < numOfCaps; i++)
    {


        // insertion sort -----------================================


        // insertion sort -----------================================




        pos.makeCapture(captures[i]);
        int score = -quiescence(pos, -beta, -alpha);
        pos.unmakeCapture();


        if (score >= beta)
        {
            return beta;
        }

        if (score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}





// takes a position, alpha and beta for pruning, current depth, keeps track of best move found, and ply for stuff such as killer moves, checkmate detection and for cleaner design
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth)
{
    // call quie at leaf nodes
    nodes++;

    if (depth == 0)
    {
        //return scoreBoard(pos);
        return quiescence(pos, alpha, beta);
    }


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

    // sort moves
    std::sort(moves, moves + numOfMoves, [&pos, &bestMove](Move a, Move b)
    {
        return scoreMove(a, pos, bestMove) > scoreMove(b, pos, bestMove);
    });


    for (int i = 0; i < numOfMoves; i++)
    {

        pos.makeMove(moves[i]);
        int score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth);
        pos.unmakeMove();
        //std::cout << "score: " << score << std::endl;

        if (score >= beta)
        {
            return beta; // hard beta cutoff
        }


        if (score > alpha)
        {
            alpha = score;
            if (depth == rootDepth)
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
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        negaMaxAlphaBeta(pos, -CHECKMATE, CHECKMATE, depth, bestMove, 0, depth);
    }

    std::cout << "Nodes: " << nodes << std::endl;
    return bestMove;

}