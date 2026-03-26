//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cmath>
#include <cstring>

#include "MoveGen.h"
#include "Score.h"
#include "TranspositionTable.h"


int nodes = 0;
int transpositionCutoffs = 0;
int quieNodes = 0;
int deltaPrunes = 0;

int quiescence(Position& pos, int alpha, int beta, Move hashMove, int ply)
{
    //if (ply > 0 && pos.checkRepetition(pos.getZobristHash())) return 0;

    ZobristHash zobrist = pos.getZobristHash();
    int value;
    Move ttBestMove = 0;

    // probe TT
    if (ply > 0 && (value = probe(0, zobrist, beta, alpha, hashMove, ply)) != NO_HASH_ENTRY)
    {
        transpositionCutoffs++;
        return value;
    }

    Bound hashFlag = Bound::BOUND_ALPHA;

    quieNodes++;

    // stand pat
    int standPat = scoreBoard(pos);

    // Delta pruning
    if (standPat < alpha - 980)  // average queen value is 980
    {
        deltaPrunes++;
        return alpha;
    }

    if (standPat >= beta)
    {
        save(zobrist, 0, beta, Bound::BOUND_BETA, ttBestMove, ply);
        return beta;
    }

    if (standPat > alpha)
    {
        //hashFlag = Bound::BOUND_EXACT;
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
        moveScores[i] = scoreQuiescenceMove(captures[i], pos, hashMove);
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

        Move capture = captures[i];

        // delta pruning - considerable speed increase in many positions - slow in others



        Piece capturedPiece = pos.getPieceFromBoard(capture >> 6 & 0x3F);
        int capturedPieceScore = averagePieceScore[0];

        if (capturedPiece != NO_PIECE)
        {
            capturedPieceScore = averagePieceScore[capturedPiece];
        }


        if (standPat + capturedPieceScore + 800 < alpha)
        {
            deltaPrunes++;
            continue;
        }




        // delta pruning



        pos.makeCapture(capture);
        int score = -quiescence(pos, -beta, -alpha, hashMove, ply + 1);
        pos.unmakeCapture();


        if (score >= beta)
        {
            save(zobrist, 0, beta, Bound::BOUND_BETA, ttBestMove, ply);
            return beta;
        }

        if (score > alpha)
        {
            hashFlag = Bound::BOUND_EXACT;
            ttBestMove = capture;
            alpha = score;
        }
    }
    save(zobrist, 0, alpha, hashFlag, ttBestMove, ply);
    return alpha;
}








// takes a position, alpha and beta for pruning, current depth, keeps track of best move found, and ply for stuff such as killer moves, checkmate detection and for cleaner design
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove)
{
    if (ply > 0 && pos.checkRepetition(pos.getZobristHash())) return 0;

    Move ttBestMove = 0;

    // call quie at leaf nodes
    nodes++;
    Bound hashFlag = Bound::BOUND_ALPHA;
    ZobristHash posZobrist = pos.getZobristHash();
    //std::cout << "ZOBRIST KEY:" << pos.getZobristHash() << std::endl;

    int value;
    Move hashMove = 0;

    // probe TT
    if (ply > 0 && (value = probe(depth, posZobrist, beta, alpha, hashMove, ply)) != NO_HASH_ENTRY)
    {
        transpositionCutoffs++;
        return value;
    }

    if (depth == 0)
    {
        // if king square is under attack set depth to 1 - check extension

        //return scoreBoard(pos);
        int qVal =  quiescence(pos, alpha, beta, hashMove, ply);
        save(posZobrist, depth, qVal, Bound::BOUND_EXACT, ttBestMove, ply);
        return qVal;
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


    // null move pruning

    if (allowNullMove) // NMP Conditions
    {
        if (info.numOfChecks == 0) // not in check
        {
            if (depth > 3) // depth more than 3
            {
                if (pos.getGamePhase()) // game phase is 0 when no major pieces are left (only pawns) - for zugzwang
                {
                    if (beta == alpha + 1) // not a PV node
                    {
                        int r = 3; // NMP Reduction
                        int epSq = pos.getEnpassantSquare();
                        pos.makeNullMove(epSq);
                        int v = -negaMaxAlphaBeta(pos, -beta, -(beta - 1), std::max(1, depth - r), bestMove, ply+1, rootDepth, false);
                        pos.unmakeNullMove(epSq);
                        if (v >= beta)
                            return v;
                    }
                }
            }
        }
    }

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
        moveScores[i] = scoreMove(moves[i], pos, hashMove, ply);
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
    int score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true);
    pos.unmakeMove();



    int fromSquare = firstMove & 0x3F;
    int toSquare = (firstMove >> 6) & 0x3F;
    if (score > alpha)
    {
        // TT:
        hashFlag = Bound::BOUND_EXACT;
        alpha = score;

        if (!(((firstMove >> 12) & 0x3F) & 4))
        {
            // store history move
            historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);
        }
        ttBestMove = firstMove;

        if (depth == rootDepth)
        {
            bestMove = firstMove;
        }

        // beta cutoff
        if (score >= beta)
        {
            // TT:
            save(posZobrist, depth, beta, Bound::BOUND_BETA, ttBestMove, ply);
            // store killer move
            if ((((firstMove >> 12) & 0x3F) & 4))
            {
                return beta;
            }

            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = firstMove;


            return beta; // hard beta cutoff
        }

    }
    //else
    //{
    //    if (!(((firstMove >> 12) & 0x3F) & 4))
    //    {
    //        // subtract score if move didnt cause cutoff
    //        historyMoves[fromSquare][toSquare] -= depth * depth;
    //    }
    //}

    // PVS ABOVE
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
        // LMR: set the depth reduction based on move index, current depth and if king is in check reduce depth by less
        int depthReduction = 1;
        Move move = moves[i];

        toSquare = (move >> 6) & 0x3F;
        // we start from i = 1, so (1) i = 1, (2) i = 2, (3) i = 3 - so atleast 3  moves searched before LMR plus the hash move so 3 + 1
        if (((move >> 12) & 0x3F) < 4)
        {
            if (depth > 3 && i > 3)
            {
                if (! (info.numOfChecks)  ) // don't reduce if we are in check
                {
                    //if (!(move == killerMoves[ply][0] || move == killerMoves[ply][1]))
                        depthReduction = 1 + log(depth) * std::log(i) / 2.0;
                }
            }

        }


        int reducedDepth = std::max(0, depth - depthReduction);

        // null window to first check if the move is good enough to warrant a full window search which happens when the move's score is above alpha
        score = -negaMaxAlphaBeta(pos, -alpha-1, -alpha, reducedDepth, bestMove, ply+1, rootDepth, true);

        // research if move score stayed within the window
        if ((score > alpha) && (score < beta))
        {
            score = -negaMaxAlphaBeta(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true);
        }
        pos.unmakeMove();


        //if (score >= beta)
        //{
        //    save(posZobrist, depth, beta, Bound::BOUND_BETA, ttBestMove, ply);
        //    // store killer move
        //    if ((((move >> 12) & 0x3F) & 4))
        //    {
        //        return beta;
        //    }
        //    killerMoves[ply][1] = killerMoves[ply][0];
        //    killerMoves[ply][0] = move;
//
//
        //    return beta; // hard beta cutoff
        //}


        int fromSquare = move & 0x3F;
        if (score > alpha)
        {
            hashFlag = Bound::BOUND_EXACT;
            alpha = score;
            if (!(((move >> 12) & 0x3F) & 4))
            {
                // store history move
                historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);
            }

            ttBestMove = move;

            if (depth == rootDepth)
            {
                bestMove = move;
            }


            // beta cutoff
            if (score >= beta)
            {
                save(posZobrist, depth, beta, Bound::BOUND_BETA, ttBestMove, ply);
                // store killer move
                if ((((move >> 12) & 0x3F) & 4))
                {
                    return beta;
                }
                killerMoves[ply][1] = killerMoves[ply][0];
                killerMoves[ply][0] = move;


                return beta; // hard beta cutoff
            }
        }
        else
        {
            if (!(((move >> 12) & 0x3F) & 4))
            {
                // store history move
                historyMoves[fromSquare][toSquare] -= depth * depth / 2;
            }
        }


    }
    save(posZobrist, depth, alpha, hashFlag, ttBestMove, ply);
    return alpha;
}




Move findBestMove(Position pos)
{
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(killerMoves, 0, sizeof(killerMoves));
    Move bestMove = 0;

    int alpha = -CHECKMATE;
    int beta = CHECKMATE;

    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        nodes = 0;
        quieNodes = 0;
        transpositionCutoffs = 0;
        deltaPrunes = 0;
        auto startTime = std::chrono::high_resolution_clock::now();
        int score = negaMaxAlphaBeta(pos, alpha, beta, depth, bestMove, 0, depth, false);
        auto endTime = std::chrono::high_resolution_clock::now();

        long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Depth: " << depth << std::endl;
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Nodes in quiescence search:" << quieNodes << std::endl;
        std::cout << "info string milliseconds: " << milliseconds << std::endl;
        std::cout << "Transpositions: " << transpositionCutoffs << std::endl;
        std::cout << "Transposition Table Entries: " << entries << std::endl;
        std::cout << "Delta prunes: " << deltaPrunes << std::endl;
        std::cout << "\n";


        if ((score <= alpha || score >= beta))
        {
            alpha = -CHECKMATE;
            beta = CHECKMATE;


            score = negaMaxAlphaBeta(pos, alpha, beta, depth, bestMove, 0, depth, false);
        }
        alpha = score - 50;
        beta = score + 50;

    }

    //std::cout << "TT entries: " << entries << std::endl;

    return bestMove;

}