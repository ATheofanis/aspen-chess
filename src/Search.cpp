//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cmath>
#include <cstring>

#include "MoveGen.h"
#include "Score.h"
#include "TranspositionTable.h"
#include "Time.h"

//Move previousMoves[MAX_PLY];


int nodes = 0;
int transpositionCutoffs = 0;
int quieNodes = 0;
int deltaPrunes = 0;

void printInfo(int depth)
{
    std::cout << "info string | Current Depth:" << depth << std::endl;
    std::cout << "info string | Negamax Nodes:" << nodes << std::endl;
    std::cout << "info string | Qsearch Nodes:" << quieNodes << std::endl;
    std::cout << "\n";
}


int precomputedLMR[32][256];

template<NodeType nodeType>
int quiescence(Position& pos, int alpha, int beta, Move ttBestMove, int ply)
{
    if ((nodes & 2047) == 0 && tm.maximumExpired())
    {
        tm.stopSearch();
        return 0;
    }

    constexpr bool isPv = nodeType == NodeType::PV;



    ZobristHash zobrist = pos.getZobristHash();
    int bestValue;
    int staticValue;



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


    // check for tt cutoff at non-PV nodes
    if (!isPv && ttHit && ttData.depth >= Q_DEPTH && // add not a PV node check when template is added
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

    if (inCheck)
    {
        bestValue = -CHECKMATE;
    }
    else
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
        int nextMoveIndex = i;
        // insertion sort (huge speed increase), swap move and score if found a better move so that we make that move immediately
        for (int j = i + 1; j < numOfCaps; j++)
        {
            if (moveScores[j] > moveScores[nextMoveIndex])
            {
                nextMoveIndex = j;
            }
        }

        if (nextMoveIndex != i)
        {
            std::swap(moveScores[i], moveScores[nextMoveIndex]);
            std::swap(captures[i], captures[nextMoveIndex]);
        }



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



        pos.makeCapture(capture);
        int score = -quiescence<nodeType>(pos, -beta, -alpha, NO_MOVE, ply + 1);
        pos.unmakeCapture();



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
template<NodeType nodeType>
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove)
{
    if ((nodes & 2047) == 0 && tm.maximumExpired())
    {
        tm.stopSearch();
        return 0;
    }

    constexpr bool isPv = (nodeType != NodeType::NonPV);
    constexpr bool rootNode = (nodeType == NodeType::Root);

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
            int qVal =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply);
            //save(posZobrist, depth, qVal, Bound::BOUND_EXACT, NO_MOVE, ply);
            return qVal;
        }

    }

    ZobristHash posZobrist = pos.getZobristHash();
    Move ttBestMove = 0;
    Bound hashFlag = Bound::BOUND_ALPHA;


    // probe TT
    auto [ttHit, ttData] = probe(posZobrist);


    // check for tt cutoff
    if (ttHit && !isPv && ttData.depth >= depth &&
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

    // get the static evaluation of the position
    int staticValue;
    bool ttMoveIsCapture = false;
    if (ttHit) // if we got a transposition table hit we can store the previously calculated static evaluation as well as the best move of the position
    {
        staticValue = ttData.staticEval;

        ttBestMove = ttData.bestMove;

        ttMoveIsCapture =  (bool)((ttBestMove >> 12 & 0x3F) & 4);

    } else // otherwise manually calculate the static evaluation
    {
        staticValue = scoreBoard(pos);
        ttBestMove = NO_MOVE;
    }


    // reverse futility pruning before move generation to cut the whole branch
    if (!isPv && depth <= 7 && !isInCheck && !ttMoveIsCapture)
    {
        int futilityMargin = 150 * depth;
        if (staticValue >= beta + futilityMargin)
            return staticValue;
    }


    // razoring
    if (!isPv && depth <= 3 && !isInCheck && pos.getGamePhase() && (alpha == beta - 1))
    {
        int margin = 100 + depth * 200;
        if (staticValue + margin < beta)
        {
            int qValue =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply);
            if (qValue < beta) return qValue;
        }
    }


    // null move pruning
    if (allowNullMove && !isPv && !isInCheck && depth > 3 && pos.getGamePhase()) // NMP Conditions
    {
        int r = std::min(depth, 3 + depth / 3); // NMP Reduction
        int epSq = pos.getEnpassantSquare();
        pos.makeNullMove(epSq);
        int v = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -beta, -(beta - 1), std::max(1, depth - r), bestMove, ply+1, rootDepth, false);
        pos.unmakeNullMove(epSq);
        if (v >= beta)
        {
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

    if constexpr (isPv)
    {
        canFutilityPrune = (depth == 1 && !isInCheck && staticValue + 200 <= alpha);
        canExtendedFutilityPrune = (depth == 2 && !isInCheck && staticValue + 500 <= alpha);
    }


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
    //previousMoves[ply] = firstMove;
    // full window for first move
    int score = -negaMaxAlphaBeta<nodeType>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true);
    pos.unmakeMove();


    int firstMovefromSquare = firstMove & 0x3F;
    int firstMovetoSquare = (firstMove >> 6) & 0x3F;
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
            if (!(firstMoveFlag & 4))
            {
                historyMoves[firstMovefromSquare][firstMovetoSquare] = std::min(historyMoves[firstMovefromSquare][firstMovetoSquare] + depth * depth, 800);

                killerMoves[ply][1] = killerMoves[ply][0];
                killerMoves[ply][0] = firstMove;
                //if (ply > 0 && previousMoves[ply-1] != NO_MOVE)
                //{
                //    Move prevMove = previousMoves[ply-1];
                //    int prevMoveFromSquare = prevMove & 0x3F;
                //    int prevMoveToSquare = (prevMove >> 6) & 0x3F;
                //    counterMoves[prevMoveFromSquare][prevMoveToSquare] = firstMove;
                //}
            }


            return beta; // hard beta cutoff
        }

    }
    else
    {
        if (!(firstMoveFlag & 4))
        {
            // store history move
            historyMoves[firstMovefromSquare][firstMovetoSquare] -= depth * depth / 2.0;
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

        int fromSquare = move & 0x3F;
        int toSquare = (move >> 6) & 0x3F;
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

        // we start from i = 1, so (1) i = 1, (2) i = 2, (3) i = 3 - so atleast 3  moves searched before LMR plus the hash move so 3 + 1
        if (depth > 3 && i > 3 && !isInCheck && !(moveFlag & 4 || moveFlag & 8))
        {
            depthReduction = precomputedLMR[depth][i];
            //if ((ttBestMove >> 12 & 0x3F) & 4) depthReduction++; // idea from stockfish , reduce more if tt move is a capture
        }


        int reducedDepth = std::max(0, depth - depthReduction);

        //previousMoves[ply] = move;

        // null window to first check if the move is good enough to warrant a full window search which happens when the move's score is above alpha
        score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, reducedDepth, bestMove, ply+1, rootDepth, true);

        // research if move score stayed within the window
        if constexpr (isPv)
        {
            if ((score > alpha) && (rootNode || score < beta))
            {
                score = -negaMaxAlphaBeta<NodeType::PV>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true);
            }
        }
        pos.unmakeMove();



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

                    //if (ply > 0 && previousMoves[ply-1] != NO_MOVE)
                    //{
                    //    Move prevMove = previousMoves[ply-1];
                    //    int prevMoveFromSquare = prevMove & 0x3F;
                    //    int prevMoveToSquare = (prevMove >> 6) & 0x3F;
                    //    counterMoves[prevMoveFromSquare][prevMoveToSquare] = move;
                    //}
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
    //memset(previousMoves, 0, sizeof(previousMoves));
    //memset(counterMoves, 0, sizeof(counterMoves));
    Move bestMove = 0;

    int alpha = -CHECKMATE;
    int beta = CHECKMATE;

    int previousScore = VALUE_NONE;
    Move previousMove = NO_MOVE;

    // iterative deepening loop
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        nodes = 0;
        quieNodes = 0;
        transpositionCutoffs = 0;
        deltaPrunes = 0;

        Move bmDummy = bestMove;

        int score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false);


        if (tm.getShouldStopFlag()) break;


        if ((score <= alpha || score >= beta))
        {
            alpha -= 50;
            beta += 50;

            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false);
        }

        if (tm.getShouldStopFlag()) break;

        if ((score <= alpha || score >= beta))
        {
            alpha = -CHECKMATE;
            beta = CHECKMATE;

            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false);
        }

        printInfo(depth);

        if (tm.getShouldStopFlag()) break;

        alpha = score - 50;
        beta = score + 50;


        if (tm.optimumExpired())
        {
            if ((previousMove != NO_MOVE && previousMove != bestMove) || (previousScore != VALUE_NONE && ((previousScore - 50) > score)))
            {
                continue;
            }
            break;
        }


        if (tm.elapsedTime() * 3 > tm.optimum()) break;

        bestMove = bmDummy;

        previousScore = score;
        previousMove = bestMove;
    }

    //std::cout << "TT entries: " << entries << std::endl;

    return bestMove;

}
