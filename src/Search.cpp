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


int nodes = 0;
int transpositionCutoffs = 0;
int quieNodes = 0;
int deltaPrunes = 0;

// prints current depth and number of nodes searched for negamax and qsearch for the position
void printInfo(int depth, int score)
{
    std::cout << "info depth " << depth << " score cp " << score << " nodes " << nodes << std::endl;
}


int precomputedLMR[64][256];


// QSearch with TT probing (non-PV nodes only). This qsearch implementation only searches for captures
template<NodeType nodeType>
int quiescence(Position& pos, int alpha, int beta, Move ttBestMove, int ply, SearchStack* ss)
{

    // first check if the maximum time limit has been exceeded
    if (tm.isTimeEnabled() && (((nodes & 2047) == 0 && tm.maximumExpired()) || (nodes >= MAX_NODES)))
    {
        // set the stop search flag of the timer manager to true so that we know not to trust the score and the bestMove found in this depth
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
        staticValue = scoreBoardNNUE(pos, ss->accumulator);
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
        // Delta pruning for the whole branch. if capturing a queen fails to raise alpha then cut the whole branch
        if (bestValue < alpha - 980)
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

    // generate capture moves using previously calculated legality info
    generateCaptures(info, pos, captures, numOfCaps);


    int moveScores[numOfCaps];

    // get the score of every move for move ordering
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

        // delta pruning for each move, skip the move if it cant raise alpha
        if (bestValue + capturedPieceScore + 200 < alpha)
        {
            deltaPrunes++;
            continue;
        }


        // SEE pruning
        int attacker = pos.getPieceFromBoard(fromSquare);
        // SEE is expensive so i limit it to moves where the attacker's score is higher than the victim's
        if (averagePieceScore[attacker] > averagePieceScore[capturedPiece])
        {
            // skip move if SEE returns less than 0 meaning losing series of captures
            if (pos.SEE(toSquare, capturedPiece, fromSquare, attacker) < 0) continue;
        }

        (ss+1)->accumulator = ss->accumulator;
        (ss+1)->accumulator.makeMove(capture, pos);

        // make move, search it then unmake to get its score from qsearch
        pos.makeCapture(capture);
        int score = -quiescence<nodeType>(pos, -beta, -alpha, NO_MOVE, ply + 1, ss+1);
        pos.unmakeCapture();


        if (score > bestValue)
        {
            bestValue = score;

            if (score > alpha)
            {
                hashFlag = Bound::BOUND_EXACT;
                ttBestMove = capture;
                if (score < beta) // raise alpha only if score < beta - (from stockfish)
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








/* Negamax search with alpha-beta pruning
 * Principal Variation Search (PVS)
 * Transposition Table (TT) probing
 * Null Move Pruning (NMP)
 * Futility Pruning
 * Extended Futility Pruning
 * Reverse Futility Pruning
 * Razoring
 * Late Move Reduction (LMR)
 * Move ordering:
 *      - TT move
 *      - Killer moves
 *      - History heuristic
 *      - Static Exchange Evaluation (SEE)
 */
template<NodeType nodeType>
int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove, SearchStack* ss)
{
    if (tm.isTimeEnabled() && (((nodes & 2047) == 0 && tm.maximumExpired()) || (nodes >= MAX_NODES)))
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
            int qVal =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
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

        ttMoveIsCapture =  (bool)((ttBestMove >> 12 & 0xF) & 4);

    } else // otherwise manually calculate the static evaluation
    {
        staticValue = scoreBoardNNUE(pos, ss->accumulator);
        ttBestMove = NO_MOVE;
    }


    // reverse futility pruning before move generation to cut the whole branch
    if (!isPv && depth <= 7 && !isInCheck && !ttMoveIsCapture)
    {
        int futilityMargin = 150 * depth;
        if (staticValue >= beta + futilityMargin)
            return staticValue;
    }


    // razoring - move directly to qsearch if we are far below beta
    if (!isPv && depth <= 3 && !isInCheck && pos.getGamePhase() && (alpha == beta - 1))
    {
        int margin = 100 + depth * 200;
        if (staticValue + margin < beta)
        {
            int qValue =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
            if (qValue < beta) return qValue;
        }
    }


    // null move pruning
    if (allowNullMove && !isPv && !isInCheck && depth > 3 && pos.getGamePhase()) // NMP Conditions
    {
        int r = std::min(depth, 3 + depth / 3); // NMP Reduction
        int epSq = pos.getEnpassantSquare();

        (ss+1)->accumulator = ss->accumulator;
        pos.makeNullMove(epSq);
        int v = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -beta, -(beta - 1), std::max(1, depth - r), bestMove, ply+1, rootDepth, false, ss+1);
        pos.unmakeNullMove(epSq);
        if (v >= beta)
        {
            return v;
        }
    }


    // generate the moves by first extracting the legality info of the position
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


    // if no moves were generated then check for CHECKMATE or STALEMATE
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

    // only prune if we are not at a PV node
    if constexpr (!isPv)
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

    // Update the accumulator for the next ply
    (ss+1)->accumulator = ss->accumulator;
    (ss+1)->accumulator.makeMove(firstMove, pos);
    pos.makeMove(firstMove);

    // full window for first move
    int score = -negaMaxAlphaBeta<nodeType>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
    pos.unmakeMove();


    int firstMovefromSquare = firstMove & 0x3F;
    int firstMovetoSquare = (firstMove >> 6) & 0x3F;
    int firstMoveFlag = firstMove >> 12 & 0xF;

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
    // End of PVS




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
        int moveFlag = move >> 12 & 0xF;


        // futility and extended futility prune
        if (canFutilityPrune || canExtendedFutilityPrune)
        {
            if (!(moveFlag & 4 || moveFlag & 8)) continue;
        }


        (ss+1)->accumulator = ss->accumulator;
        (ss+1)->accumulator.makeMove(moves[i], pos);
        pos.makeMove(moves[i]);

        // LMR: Reduce the search depth of a move based on current depth and move index
        // Later moves are usually worse so reduce their depth for more
        int depthReduction = 1;

        // LMR Conditions:
        // - reduce depth for moves at sufficient depth
        // - don't reduce depth of the first 3 moves since they are ordered from best to worst
        // - don't reduce captures/promotions, or when in check
        if (depth > 3 && i > 3 && !isInCheck && !(moveFlag & 4 || moveFlag & 8))
        {
            depthReduction = precomputedLMR[depth][i];
        }

        // Make sure the depth reduction does not lead to a depth less than zero
        int reducedDepth = std::max(0, depth - depthReduction);


        // null window to first check if the move is good enough to warrant a full window search which happens when the move's score is above alpha
        score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, reducedDepth, bestMove, ply+1, rootDepth, true, ss+1);

        // re-search if move score stayed within the null window
        if constexpr (isPv)
        {
            if ((score > alpha) && (rootNode || score < beta))
            {
                score = -negaMaxAlphaBeta<NodeType::PV>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
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



// This function initiates the search using iterative deepening and aspiration windows
Move findBestMove(Position pos, int& posEval)
{
    // Increment generation for transposition table aging replacement scheme
    generation++;

    // Intialize killer and history moves arrays
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(killerMoves, 0, sizeof(killerMoves));

    Move bestMove = 0;

    // Initialize search window (Originally set to a full search window)
    int alpha = -CHECKMATE;
    int beta = CHECKMATE;

    int previousScore = VALUE_NONE;
    Move previousMove = NO_MOVE;

    SearchStack ss[MAX_PLY];
    ss[0].accumulator.initializeAccumulator(pos);

    // Iterative deepening loop
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        // Reset search statistics from earlier depths
        nodes = 0;
        quieNodes = 0;
        transpositionCutoffs = 0;
        deltaPrunes = 0;

        Move bmDummy = bestMove;

        int score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);
        posEval = score;

        // If the time management flag is set to true, it means that the search was interrupted.
        // The results of an interrupted search are discarded since they are incomplete
        if (tm.isTimeEnabled() && tm.getShouldStopFlag()) break;

        // If the score found is outside of the alpha and beta bounds then we expand the search window and search again
        if ((score <= alpha || score >= beta))
        {
            alpha -= 50;
            beta += 50;

            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);
        }

        if (tm.isTimeEnabled() && tm.getShouldStopFlag()) break;

        posEval = score;

        // If re-searching with a wider window still results in an out of bounds score, then search again with a full window
        if ((score <= alpha || score >= beta))
        {
            alpha = -CHECKMATE;
            beta = CHECKMATE;

            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);
        }

        bestMove = bmDummy;

        previousScore = score;
        previousMove = bestMove;

        // Print search statistics (Negamax nodes, QSearch nodes and current search depth)
        if (!DataGenFlag) printInfo(depth, score); // only print info if we are not generating self play data

        if (tm.isTimeEnabled() && tm.getShouldStopFlag()) break;
        posEval = score;

        // Set the new aspiration window based on the score we found at this depth
        // This way the window for the next depth will be narrow and will lead to more cutoffs than if it were a full search window
        alpha = score - 50;
        beta = score + 50;


        // If the optimum time limit that the time manager set has expired
        if (tm.isTimeEnabled() && tm.optimumExpired())
        {
            // If we found a different best move at this depth or the score suddenly dropped then we need to give the engine more time to keep searching to resolve the instability
            if ((previousMove != NO_MOVE && previousMove != bestMove) || (previousScore != VALUE_NONE && ((previousScore - 50) > score)) && !tm.maximumExpired())
            {
                continue;
            }
            break;
        }

        // Sometimes the engine goes into a deeper search only to be interrupted after a few seconds.
        // That means the engine will spend time to find a move that will be discarded anyways.
        // To prevent that, we only continue to a deeper search if the time that has elapsed is less than 3 times the optimum time limit
        // For example if we are at depth 16 then a depth 17 search will likely need 3x more time to be fully searched.
        // Therefore we attempt to predict if we have enough time left to finish that search, if not then we stop the search here
        if (tm.isTimeEnabled() && (tm.elapsedTime() * 3 > tm.optimum())) break;

    }


    return bestMove;

}