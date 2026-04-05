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

int quiescence(Position& pos, int alpha, int beta, Move ttBestMove, int ply)
{
    //if (ply > 0 && pos.checkRepetition(pos.getZobristHash())) return 0;

    ZobristHash zobrist = pos.getZobristHash();
    int bestValue;
    int staticValue;


    /*
    auto [ttHit, ttData] = probe(zobrist);



    // probe TT
    if (ply > 0 && (bestValue = probe(0, zobrist, beta, alpha, , ply)) != NO_HASH_ENTRY)
    {
        transpositionCutoffs++;
        return bestValue;
    }
    */


    // probe TT
    auto [ttHit, ttData] = probe(zobrist);

    if (ttHit)
    {
        staticValue = ttData.staticEval;
        ttBestMove = ttData.bestMove;
    } else
    {
        staticValue = scoreBoard(pos);
    }


    // check for tt cutoff
    if (ttHit && ttData.depth >= Q_DEPTH && // add not a PV node check when template is added
        ( ( ttData.bound == Bound::BOUND_EXACT ) ||
        ( ttData.bound == Bound::BOUND_ALPHA && ttData.evaluation <= alpha ) ||
        ( ttData.bound == Bound::BOUND_BETA && ttData.evaluation >= beta )  ) )
    {
        int eval = ttData.evaluation;

        if (eval < -CHECKMATE + 500) eval += ply;
        if (eval > CHECKMATE - 500) eval -= ply;

        return eval;
    }


    Bound hashFlag = Bound::BOUND_ALPHA;


    quieNodes++;

    // stand pat
    bestValue = staticValue;
    bool inCheck = pos.sideToMoveIsInCheck();

    if (!inCheck)
    {
        // Delta pruning
        if (bestValue < alpha - 980)  // average queen value is 980
        {
            deltaPrunes++;
            return bestValue;
        }

        if (bestValue >= beta)
        {
            if (ttBestMove != NO_MOVE)
                save(zobrist, ttBestMove, bestValue, staticValue, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
            return bestValue;
        }
    }


    if (bestValue > alpha)
    {
        //hashFlag = Bound::BOUND_EXACT;
        alpha = bestValue;
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
        moveScores[i] = scoreQuiescenceMove(captures[i], pos, ttBestMove);
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
        int fromSquare = capture & 0x3F;
        int toSquare = (capture >> 6) & 0x3F;


        // delta pruning - considerable speed increase in many positions - slow in others

        Piece capturedPiece = pos.getPieceFromBoard(toSquare);
        int capturedPieceScore = averagePieceScore[0];

        if (capturedPiece != NO_PIECE)
        {
            capturedPieceScore = averagePieceScore[capturedPiece];
        }


        if (bestValue + capturedPieceScore + 200 < alpha)
        {
            deltaPrunes++;
            continue;
        }


        // SEE pruning
        int attacker = pos.getPieceFromBoard(fromSquare);
        if (averagePieceScore[attacker] > averagePieceScore[capturedPiece])
        {
            if (pos.SEE(toSquare, capturedPiece, fromSquare, attacker) < 0) continue;
        }





        // delta pruning



        pos.makeCapture(capture);
        int score = -quiescence(pos, -beta, -alpha, NO_MOVE, ply + 1);
        pos.unmakeCapture();



        //if (score > alpha)
        //{
        //    hashFlag = Bound::BOUND_EXACT;
        //    ttBestMove = capture;
        //    alpha = score;
        //    if (score >= beta)
        //    {
        //        save(zobrist, 0, beta, Bound::BOUND_BETA, ttBestMove, ply);
        //        return beta;
        //    }
        //}


        if (score > bestValue)
        {
            bestValue = score;

            if (score > alpha)
            {
                hashFlag = Bound::BOUND_EXACT;
                ttBestMove = capture;
                if (score < beta)
                {
                    // Update alpha here!
                    alpha = score;
                }
                else
                {
                    save(zobrist, ttBestMove, score, staticValue, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
                    break;  // Fail high
                }

            }
        }


    }
    save(zobrist, ttBestMove, bestValue, staticValue, Q_DEPTH, hashFlag, generation, ply);
    return bestValue;
}








// takes a position, alpha and beta for pruning, current depth, keeps track of best move found, and ply for stuff such as killer moves, checkmate detection and for cleaner design
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove)
{
    nodes++;
    if (ply > 0 && pos.checkRepetition(pos.getZobristHash())) return 0;

    bool isInCheck = pos.sideToMoveIsInCheck();


    if (depth == 0)
    {
        if (isInCheck)
        {
            depth = 1;
        } else
        {
            int qVal =  quiescence(pos, alpha, beta, NO_MOVE, ply);
            //save(posZobrist, depth, qVal, Bound::BOUND_EXACT, NO_MOVE, ply);
            return qVal;
        }

    }

    ZobristHash posZobrist = pos.getZobristHash();
    Move ttBestMove = 0;
    Bound hashFlag = Bound::BOUND_ALPHA;


    // probe TT
    auto [ttHit, ttData] = probe(posZobrist);
    int staticValue;
    if (ttHit)
    {
        staticValue = ttData.staticEval;
        ttBestMove = ttData.bestMove;
    } else
    {
        staticValue = scoreBoard(pos);
        ttBestMove = NO_MOVE;
    }


    // check for tt cutoff
    if (ttHit && ttData.depth >= depth && // add not a PV node check when template is added
        ( ( ttData.bound == Bound::BOUND_EXACT ) ||
        ( ttData.bound == Bound::BOUND_ALPHA && ttData.evaluation <= alpha ) ||
        ( ttData.bound == Bound::BOUND_BETA && ttData.evaluation >= beta )  ) )
    {

        int eval = ttData.evaluation;

        if (eval < -CHECKMATE + 500) eval += ply;
        if (eval > CHECKMATE - 500) eval -= ply;

        if (ply == 0) bestMove = ttData.bestMove;
        return eval;
    }




    // null move pruning

    if (allowNullMove && !isInCheck && depth > 3 && pos.getGamePhase()) // NMP Conditions
    {
        if (beta == alpha + 1) // not a PV node
        {
            int r = std::min(depth, 3 + depth / 3); // NMP Reduction
            int epSq = pos.getEnpassantSquare();
            pos.makeNullMove(epSq);
            int v = -negaMaxAlphaBeta(pos, -beta, -(beta - 1), std::max(1, depth - r), bestMove, ply+1, rootDepth, false);
            pos.unmakeNullMove(epSq);
            if (v >= beta)
                return v;
        }
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
        if (isInCheck)
        {
            return ply - CHECKMATE;
        }

        return 0;
    }

    // set futility pruning flag
    bool canFutilityPrune = false;
    bool canExtendedFutilityPrune = false;
    canFutilityPrune = (depth == 1 && !isInCheck && staticValue + 200 <= alpha);
    canExtendedFutilityPrune = (depth == 2 && !isInCheck && staticValue + 500 <= alpha);

    // sort moves
    int moveScores[numOfMoves];


    for (int i = 0; i < numOfMoves; i++)
    {
        moveScores[i] = scoreMove(moves[i], pos, ttBestMove, ply);
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
    int firstMoveFlag = firstMove >> 12 & 0x3F;

    if (score > alpha)
    {
        // TT:
        hashFlag = Bound::BOUND_EXACT;
        alpha = score;

        ttBestMove = firstMove;

        if (ply == 0)
        {
            bestMove = firstMove;
        }

        // beta cutoff
        if (score >= beta)
        {
            // TT:
            save(posZobrist, ttBestMove, score, staticValue, depth, Bound::BOUND_BETA, generation, ply);
            // store killer move
            if (firstMoveFlag & 4)
            {
                return beta;
            }
            historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);

            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = firstMove;


            return beta; // hard beta cutoff
        }

    }
    else
    {
        if (!(firstMoveFlag & 4))
        {
            // store history move
            historyMoves[fromSquare][toSquare] -= depth * depth / 2.0;
        }
    }
    // ***************************************************************




    // start from i = 1 because we already searched the first move
    for (int i = 1; i < numOfMoves; i++)
    {
        int nextMoveIndex = i;
        // insertion sort (huge speed increase), swap move and score if found a better move so that we make that move immediately
        for (int j = i + 1; j < numOfMoves; j++)
        {
            if (moveScores[j] > moveScores[nextMoveIndex])
            {
                nextMoveIndex = j;
            }
        }

        if (nextMoveIndex != i)
        {
            std::swap(moveScores[i], moveScores[nextMoveIndex]);
            std::swap(moves[i], moves[nextMoveIndex]);
        }


        Move move = moves[i];
        int moveFlag = move >> 12 & 0x3F;


        // futility pruning



        if (canFutilityPrune || canExtendedFutilityPrune)
        {
            if (!(moveFlag & 4 || moveFlag & 8)) continue;
        }




        // futility pruning


        //Move move = moves[i];
        pos.makeMove(moves[i]);

        // LMR: set the depth reduction based on move index, current depth and if king is in check reduce depth by less
        int depthReduction = 1;

        toSquare = (move >> 6) & 0x3F;
        // we start from i = 1, so (1) i = 1, (2) i = 2, (3) i = 3 - so atleast 3  moves searched before LMR plus the hash move so 3 + 1
        if (!(moveFlag & 4 || moveFlag & 8))
        {
            if (depth > 3 && i > 3)
            {
                if (! (info.numOfChecks)  ) // don't reduce if we are in check
                {
                    //if (!(move == killerMoves[ply][0] || move == killerMoves[ply][1]))
                        depthReduction = 1 + std::log(depth) * std::log(i) / 2.0;
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



        fromSquare = move & 0x3F;
        if (score > alpha)
        {
            hashFlag = Bound::BOUND_EXACT;
            alpha = score;


            ttBestMove = move;

            if (ply == 0)
            {
                bestMove = move;
            }


            // beta cutoff
            if (score >= beta)
            {
                save(posZobrist, ttBestMove, score, staticValue, depth, Bound::BOUND_BETA, generation, ply);
                // store killer move
                if (!(moveFlag & 4))
                {
                    historyMoves[fromSquare][toSquare] = std::min(historyMoves[fromSquare][toSquare] + depth * depth, 800);
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                }


                return beta; // hard beta cutoff
            }
        }
        else
        {
            if (!(moveFlag & 4))
            {
                // store history move
                historyMoves[fromSquare][toSquare] -= depth * depth / 2.0;
            }
        }


    }
    save(posZobrist, ttBestMove, alpha, staticValue, depth, hashFlag, generation, ply);
    return alpha;
}




Move findBestMove(Position pos)
{
    generation++;
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
        std::cout << "PAWN HASH HITS: " << pawnHashHIT << std::endl;
        std::cout << "PAWN HASH MISSES: " << pawnHashMISS << std::endl;
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