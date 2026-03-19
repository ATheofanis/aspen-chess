//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cstring>

#include "MoveGen.h"
#include "Score.h"


int nodes = 0;


int quiescence(Position& pos, int alpha, int beta, Move bestMove, int ply)
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
    int moveScores[numOfCaps];

    for (int i = 0; i < numOfCaps; i++)
    {
        moveScores[i] = scoreQuiescenceMove(captures[i], pos, bestMove);
    }


    for (int i = 0; i < numOfCaps; i++)
    {


        // insertion sort -----------================================
        for (int j = i + 1; j < numOfCaps; j++)
        {
            if (moveScores[j] > moveScores[i])
            {
                std::swap(moveScores[i], moveScores[j]);
                std::swap(captures[i], captures[j]);
            }
        }

        // insertion sort -----------================================




        pos.makeCapture(captures[i]);
        int score = -quiescence(pos, -beta, -alpha, bestMove, ply + 1);
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
        return quiescence(pos, alpha, beta, bestMove, ply);
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
    int moveScores[numOfMoves];

    for (int i = 0; i < numOfMoves; i++)
    {
        moveScores[i] = scoreMove(moves[i], pos, bestMove, ply);
    }






    // ***************************************************************
    // PVS: first search the first move at full window length, the rest will be searched at null window length

    int maxScore = moveScores[0];
    int firstMoveIndex = 0;
    for (int i = 0; i < numOfMoves; i++)
    {
        if (moveScores[i] > maxScore)
        {
            maxScore = moveScores[i];
            firstMoveIndex = i;
        }
    }
    std::swap(moveScores[0], moveScores[firstMoveIndex]);
    std::swap(moves[0], moves[firstMoveIndex]);

    Move firstMove = moves[0];

    pos.makeMove(firstMove);
    // full window for first move
    int score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth);
    pos.unmakeMove();

    if (score >= beta)
    {
        // store killer move
        if ((((firstMove >> 12) & 0x3F) & 4))
        {
            return beta;
        }
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = firstMove;


        return beta; // hard beta cutoff
    }


    if (score > alpha)
    {
        alpha = score;
        if (!(((firstMove >> 12) & 0x3F) & 4))
        {
            // store history move
            int fromSquare = firstMove & 0x3F;
            int toSquare = (firstMove >> 6) & 0x3F;

            historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);
        }


        if (depth == rootDepth)
        {
            bestMove = firstMove;
        }
    }


    // ***************************************************************




    // start from i = 1 because we already searched the first move
    for (int i = 1; i < numOfMoves; i++)
    {
        // insertion sort (huge speed increase), swap move and score if found a better move so that we make that move immediately
        for (int j = i + 1; j < numOfMoves; j++)
        {
            if (moveScores[j] > moveScores[i])
            {
                std::swap(moveScores[i], moveScores[j]);
                std::swap(moves[i], moves[j]);
            }
        }
        //Move move = moves[i];

        pos.makeMove(moves[i]);
        // null window to first check if the move is good enough to warrant a full window search which happens when the move's score is above alpha
        int score = -negaMaxAlphaBeta(pos, -alpha-1, -alpha, depth - 1, bestMove, ply+1, rootDepth);

        // research if move score stayed within the window
        if ((score > alpha) && (score < beta))
        {
            score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth);
        }
        pos.unmakeMove();

        Move move = moves[i];
        if (score >= beta)
        {
            // store killer move
            if ((((move >> 12) & 0x3F) & 4))
            {
                return beta;
            }
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;


            return beta; // hard beta cutoff
        }


        if (score > alpha)
        {
            alpha = score;
            if (!(((move >> 12) & 0x3F) & 4))
            {
                // store history move
                int fromSquare = move & 0x3F;
                int toSquare = (move >> 6) & 0x3F;

                historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);
            }


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
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(killerMoves, 0, sizeof(killerMoves));
    Move bestMove = 0;
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        negaMaxAlphaBeta(pos, -CHECKMATE, CHECKMATE, depth, bestMove, 0, depth);
    }

    std::cout << "Nodes: " << nodes << std::endl;
    return bestMove;

}