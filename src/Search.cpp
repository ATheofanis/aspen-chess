//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cmath>
#include <cstring>

#include "LegalMoveGen.h"
#include "Score.h"
#include "TranspositionTable.h"
#include "Time.h"


int precomputedLMR[128][256];


// prints current depth and number of nodes searched for negamax and qsearch for the position
void MoveSearcher::printInfo(int depth, int score)
{
    std::cout << "info depth " << depth
              << " score cp " << score
              << " nodes "    << nodes
              << " nps "      << (tm.elapsedTime() > 0 ? nodes * 1000LL / tm.elapsedTime() : 0)
              << " time "     << tm.elapsedTime()
              << " pv ";

    for (int count = 0; count < pvLength[0]; count++)
    {
        // Print PV
        printMove(pvTable[0][count]);
        std::cout << " ";
    }
    std::cout << std::endl;
}



//    |===============================================================================================|
//    |  Quiescent Search : Once the search reaches a depth of 0, it hits the standard search limit.  |
//    |   However if we were to return the score as-is, it would be prone to the most common horizon  |
//    |     effect - specifically missing captures and promotions. Quiescent search continues         |
//    |         the search, limiting itself to captures and promotions. This ensures that             |
//    |          the final leaf nodes are stable, quiet and most importantly trustworthy              |
//    |===============================================================================================|
template<NodeType nodeType>
int MoveSearcher::quiescence(Position& pos, int alpha, int beta, Move ttBestMove, int ply, SearchStack* ss)
{

    //    |====================================================================|
    //    |  Time / Node Limit : Every 2048 nodes searched, check if we have   |
    //    |   exceeded the maximum time limit, or the maximum node limit       |
    //    |====================================================================|
    if ((tm.isTimeEnabled() && (((nodes & 2047) == 0 && tm.maximumExpired()))) || (nodes >= MAX_NODES))
    {
        // Update the 'STOP' flag of the timer manager
        tm.stopSearch();
        return 0;
    }

    constexpr bool isPv = nodeType == NodeType::PV;

    // Get the zobrist hash of the position in order to probe the Transposition Table for this position
    ZobristHash zobrist = pos.getZobristHash();

    // Best Value is the final value that quiescence search will return
    int bestValue;

    // We also calculate the static evaluation of the position
    // If we are lucky it is already stored inside the transposition table, saving us a lot of time
    int staticValue;


    // |=================================================================================================|
    // |  Transposition Table Probing : Before beginning the search, we probe the transposition table    |
    // |   to see if it has previously stored information about this specific position. If the probe     |
    // |     was successful, it returns the evaluation of the position as it was calculated earlier.     |
    // |   The data returned is very useful as it can lead to cutoffs, saving us entire search trees.    |
    // |    Even if a cutoff condition is not met, the probe is still useful as it often returns the     |
    // |   best move stored in the table's entry. We utilize this by searching that move first,          |
    // |   greatly increasing search speed.                                                              |
    // |=================================================================================================|

    auto [ttHit, ttData] = probe(zobrist);

    // Check for TT cutoff
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

    // If the probe was a hit we have access to the entry's static evaluation and the best move it has stored
    if (ttHit)
    {
        staticValue = ttData.staticEval; // Static Evaluation

        ttBestMove = ttData.bestMove; // Best Move
    }
    // Otherwise we need to manually calculate the static evaluation
    else
    {
        staticValue = scoreBoardNNUE(pos, ss->accumulator);
    }

    Bound hashFlag = Bound::BOUND_ALPHA;


    // stand pat
    bestValue = staticValue;
    bool inCheck = pos.sideToMoveIsInCheck();


    // If we are in check then we can't trust the static evaluation to determine the score of the position
    if (inCheck)
    {
        bestValue = -CHECKMATE;
    }
    else
    {
        // |==========================================================================================|
        // | Delta Pruning (Before the search loop) : If our best score (stand pat) is so far below   |
        // |    alpha that even capturing a Queen wouldn't raise our score above alpha,               |
        // |  this node is doomed to fail-low. We can safely prune the entire branch right now        |
        // |           instead of generating and testing captures in the loop.                        |
        // |==========================================================================================|
        if (bestValue < alpha - 980)
        {
            return bestValue;
        }

        // |================================================================================================|
        // | Stand-Pat Cutoff : The static evaluation serves as a lower bound. If the stand-pat score is    |
        // |   already >= beta, the position is "too good" and the opponent would have deviated earlier.    |
        // | We return the evaluation immediately (fail-high) without generating or searching any captures. |
        // |================================================================================================|
        if (bestValue >= beta)
        {
            // Store the data we have found in the transposition table, only if a valid best move has been found
            if (ttBestMove != NO_MOVE)
            {
                save(zobrist, ttBestMove, bestValue, staticValue, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
            }
            return bestValue;
        }
    }

    nodes++;

    if (bestValue > alpha)
    {
        alpha = bestValue;
    }

    Move captures[128];
    int numOfCaps = 0;


    // |=================================================================================================|
    // |  Legality Information : The engine generates every legal move at once using a legality struct   |
    // |   that contains information about the position. This info is then used during move generation   |
    // |     to exclude illegal moves. For this purpose we call a function to find every pinned piece,   |
    // |   pinner, checker, legal square for the king etc., allowing for very fast legal move generation |
    // |=================================================================================================|
    Color allyColor = pos.isWhiteToMove() ? White : Black;
    int kingSquare = pos.isWhiteToMove() ? lsbIndex(pos.getPieceBitboard(wK)) : lsbIndex(pos.getPieceBitboard(bK));

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

        // Pick the next best capture
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

                // Only update alpha if the score is within the window (score < beta)
                if (score < beta)
                {
                    // Update alpha here
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
int MoveSearcher::negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove, SearchStack* ss)
{
    // Increment nodes counter
    nodes++;

    //    |====================================================================|
    //    |  Time / Node Limit : Every 2048 nodes searched, check if we have   |
    //    |   exceeded the maximum time limit, or the maximum node limit       |
    //    |====================================================================|

    if ((tm.isTimeEnabled() && ((nodes & 2047) == 0) && tm.maximumExpired()) || nodes >= MAX_NODES)
    {
        // Update the 'STOP' flag of the timer manager
        tm.stopSearch();
        return 0;
    }

    // Set the PV length to current ply for triangular PV table tracking
    pvLength[ply] = ply;

    // Check if the current node is PV or Root based on the nodeType template argument
    constexpr bool isPv = (nodeType != NodeType::NonPV);
    constexpr bool rootNode = (nodeType == NodeType::Root);
    constexpr auto nextNodeType = isPv ? NodeType::PV : NodeType::NonPV;

    // Return 0 for draw if 3-fold repetition is detected
    if (ply > 0 && pos.checkRepetition(pos.getZobristHash())) return 0;

    // Find out if the side to move is currently in check
    bool isInCheck = pos.sideToMoveIsInCheck();


    //    |===============================================================================================|
    //    |  Quiescent Search : Once the search reaches a depth of 0, it hits the standard search limit.  |
    //    |   However if we were to return the score as-is, it would be prone to the most common horizon  |
    //    |     effect - specifically missing captures and promotions. Quiescent search continues         |
    //    |         the search, limiting itself to captures and promotions. This ensures that             |
    //    |          the final leaf nodes are stable, quiet and most importantly trustworthy              |
    //    |===============================================================================================|

    if (depth <= 0)
    {
        // Do not drop to quiescent search if we are in check
        if (isInCheck)
        {
            // Search for one more depth to reach a non-check position before quiescent search starts
            depth = 1;
        } else
        {
            // Drop into quiescent search - we are not in check
            int qVal =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
            return qVal;
        }
    }

    // Get the zobrist hash of the position to probe the hash table
    ZobristHash posZobrist = pos.getZobristHash();
    Move ttBestMove = NO_MOVE;
    Bound hashFlag = Bound::BOUND_ALPHA;


    // |=================================================================================================|
    // |  Transposition Table Probing : Before beginning the search, we probe the transposition table    |
    // |   to see if it has previously stored information about this specific position. If the probe     |
    // |     was successful, it returns the evaluation of the position as it was calculated earlier.     |
    // |   The data returned is very useful as it can lead to cutoffs, saving us entire search trees.    |
    // |    Even if a cutoff condition is not met, the probe is still useful as it often returns the     |
    // |   best move stored in the table's entry. We utilize this by searching that move first,          |
    // |   greatly increasing search speed.                                                              |
    // |=================================================================================================|

    auto [ttHit, ttData] = probe(posZobrist);

    // Check for TT cutoff
    if (ttHit && !isPv && ttData.depth >= depth &&
        ( ( ttData.bound == Bound::BOUND_EXACT ) ||
        ( ttData.bound == Bound::BOUND_ALPHA && ttData.evaluation <= alpha ) ||
        ( ttData.bound == Bound::BOUND_BETA && ttData.evaluation >= beta )  ) )
    {
        // The evaluation that was stored inside the TT entry
        int eval = ttData.evaluation;

        // Normalize checkmate scores - endorces shorter checkmates
        if (eval < -CHECKMATE + 500) eval += ply;
        if (eval > CHECKMATE - 500) eval -= ply;

        // If we are a root node set the best move to the tt best move, provided it is valid
        if (ply == 0 && ttData.bestMove != NO_MOVE) bestMove = ttData.bestMove;

        // Return the evaluation found saving us this whole search tree
        return eval;
    }

    // We calculate the static evaluation of the position for reverse futility pruning etc.
    int staticValue;
    bool ttMoveIsCapture = false;
    bool ttMoveIsPromo = false;

    // If the probe was a hit, it means we already have the static value from the entry,
    // so we do not need to calculate it again
    if (ttHit)
    {
        // Store the static evaluation from the entry
        staticValue = ttData.staticEval;

        // Store the best move from the entry
        ttBestMove = ttData.bestMove;

        // Check if the hash move is a capture - useful for reverse futility and LMR
        int ttMoveFlag = getMoveFlag(ttBestMove);
        ttMoveIsCapture = isCapture(ttMoveFlag);
        ttMoveIsPromo = isPromotion(ttMoveFlag);

    }
    // If the probe was not succesful we need to calculate the static evaluation of the position
    else
    {
        // Call the NNUE evaluation function
        staticValue = scoreBoardNNUE(pos, ss->accumulator);
    }


    // Futility , Razoring & NMP
    if (!isPv && !isInCheck)
    {
        // |=================================================================================================|
        // |  Reverse Futility Pruning : If the static evaluation of the position is so high that it greatly |
        // |   exceeds the value of Beta, we can safely assume that the branch would fail high if searched.  |
        // |   In order to be certain, we only prune under specific conditions: the node must not be a PV    |
        // |     node, the king must not be in check, the hash move must not be a capture,                   |
        // |                    and the remaining depth must be relatively low.                              |
        // |=================================================================================================|
        if (depth <= 7 && !ttMoveIsCapture)
        {
            // Calculate the margin for reverse futility pruning.
            // The greater the depth the greater the margin, keeping the search stable
            int reverseFutilityMargin = 150 * depth;

            // If the RFP condition is met simply return the static evaluation
            if (staticValue >= beta + reverseFutilityMargin) return staticValue;
        }

        // |=================================================================================================|
        // |  Razoring : If the static evaluation of the position is so low that even after adding a         |
        // |   safety margin it fails to beat alpha, it is probably safe to assume that this branch would    |
        // |     fail low, were it to be searched. However, to ensure we do not accidentally prune hidden    |
        // |   tactics, we instead drop directly into quiescence search. If the value it returns is still    |
        // |             below alpha, we can safely prune the branch and return that value.                  |
        // |=================================================================================================|
        if (depth <= 3 && pos.getGamePhase() && (alpha == beta - 1))
        {
            int margin = 100 + depth * 200;
            if (staticValue + margin < beta)
            {
                int qValue =  quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
                if (qValue < beta) return qValue;
            }
        }

        // |====================================================================================================|
        // |  Null Move Pruning : If our position is so strong that it still beats Beta even after              |
        // |     doing nothing, it is safe to assume that making a real move would also beat Beta.              |
        // |   We search this "null move" at a reduced depth. If the score returned is still above beta, we     |
        // |   can safely prune the branch and save a lot of time. We just check the game phase first to make   |
        // |   sure we aren't in an endgame where skipping a could be good in certain cases (zugzwang).         |
        // |====================================================================================================|
        if (allowNullMove && depth > 3 && pos.getGamePhase()) // Do not play a null move twice in a row
        {
            int reduction = std::min(depth, 3 + depth / 3); // NMP Reduction
            int epSq = pos.getEnpassantSquare();

            (ss+1)->accumulator = ss->accumulator;
            pos.makeNullMove(epSq);
            int value = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -beta, -(beta - 1), std::max(1, depth - reduction), bestMove, ply+1, rootDepth, false, ss+1);
            pos.unmakeNullMove(epSq);
            // If the value returned still exceeds beta after not playing a move then immediately return that value
            if (value >= beta)
            {
                return value;
            }
        }
    }


    // |===================================================================================================|
    // |    Futility Pruning: If we are at depth 1 and the static evaluation is far below alpha, then      |
    // |  searching quiet moves is likely futile since they are unlikely to raise the score above alpha.   |
    // |     In this case, we flag the node to skip quiet moves later, saving significant search time.     |
    // |===================================================================================================|
    // |  Extended Futility Pruning: Same concept as normal futility pruning but applied at higher depths  |
    // |    depths. Aspen uses a larger margin here to safely account for the additional search depth.     |
    // |===================================================================================================|
    bool canFutilityPrune = false;
    bool canExtendedFutilityPrune = false;

    // Only allow futility pruning at non-PV nodes
    if constexpr (!isPv)
    {
        // We must also not be in check
        canFutilityPrune = (depth == 1 && !isInCheck && staticValue + 200 <= alpha);
        canExtendedFutilityPrune = (depth == 2 && !isInCheck && staticValue + 500 <= alpha);
    }

    // generate the moves by first extracting the legality info of the position
    Move moves[256];
    int numOfMoves = 0;

    // |=================================================================================================|
    // |  Legality Information : The engine generates every legal move at once using a legality struct   |
    // |   that contains information about the position. This info is then used during move generation   |
    // |     to exclude illegal moves. For this purpose we call a function to find every pinned piece,   |
    // |   pinner, checker, legal square for the king etc., allowing for very fast legal move generation |
    // |=================================================================================================|
    Color allyColor = pos.isWhiteToMove() ? White : Black;
    int kingSquare = pos.isWhiteToMove() ? lsbIndex(pos.getPieceBitboard(wK)) : lsbIndex(pos.getPieceBitboard(bK));

    legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);

    // Generate all legal moves using previously calculated legality info
    generateLegalMoves(info, pos, moves, numOfMoves);

    // If no legal moves were generated then check for CHECKMATE or STALEMATE
    if (numOfMoves == 0)
    {
        if (isInCheck) { return ply - CHECKMATE; }
        return 0;
    }

    // Keep track of quiet moves searched for late move pruning (LMP)
    int quietMovesCount = 0;

    int moveScores[numOfMoves];

    for (int i = 0; i < numOfMoves; i++)
    {
        // Get the score of every move
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

    int firstMoveFromSquare = getFromSquare(firstMove);
    int firstMoveToSquare = getToSquare(firstMove);
    int firstMoveFlag = getMoveFlag(firstMove);

    // Check if the move is a capture or/and promotion
    bool moveIsCapture = isCapture(firstMove);
    bool moveIsPromotion = isPromotion(firstMove);

    // If it is neither, the move is quiet
    bool moveIsQuiet = !(moveIsCapture || moveIsPromotion);

    if (score > alpha)
    {
        // TT:
        hashFlag = Bound::BOUND_EXACT;
        alpha = score;

        ttBestMove = firstMove;

        // Store PV move
        pvTable[ply][ply] = firstMove;

        // Copy move from deeper plies into current ply's line
        for (int nextPly = ply + 1; nextPly < pvLength[ply + 1]; nextPly++)
        {
            pvTable[ply][nextPly] = pvTable[ply + 1][nextPly];
        }

        // Adjust PV length
        pvLength[ply] = pvLength[ply + 1];


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
            if (moveIsQuiet)
            {
                historyMoves[firstMoveFromSquare][firstMoveToSquare] = std::min(historyMoves[firstMoveFromSquare][firstMoveToSquare] + depth * depth, 800);

                killerMoves[ply][1] = killerMoves[ply][0];
                killerMoves[ply][0] = firstMove;
            }

            return beta; // hard beta cutoff
        }
    }
    else
    {
        if (moveIsQuiet)
        {
            // History penalty - to be changed very soon
            historyMoves[firstMoveFromSquare][firstMoveToSquare] -= depth * depth / 2.0;
        }
    }
    // End of PVS




    // Start from the second move since we already searched the first
    for (int i = 1; i < numOfMoves; i++)
    {
        int nextMoveIndex = i;
        // Select the move with the highest score
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

        int fromSquare = getFromSquare(move);
        int toSquare = getToSquare(move);
        int moveFlag = getMoveFlag(move);

        // Check if the move is a capture or/and promotion
        moveIsCapture = isCapture(move);
        moveIsPromotion = isPromotion(move);

        // If it is neither, the move is quiet
        moveIsQuiet = !(moveIsCapture || moveIsPromotion);


        // |===================================================================================================|
        // |    Futility Pruning: If we are at depth 1 and the static evaluation is far below alpha, then      |
        // |  searching quiet moves is likely futile since they are unlikely to raise the score above alpha.   |
        // |     In this case, we flag the node to skip quiet moves later, saving significant search time.     |
        // |===================================================================================================|
        // |  Extended Futility Pruning: Same concept as normal futility pruning but applied at higher depths  |
        // |    depths. Aspen uses a larger margin here to safely account for the additional search depth.     |
        // |===================================================================================================|
        if (moveIsQuiet && (move != ttBestMove) && (canFutilityPrune || canExtendedFutilityPrune)) // Do not prune the move it is the hash move
        {
            continue;
        }

        // |===========================================================================|
        // | Late Move Pruning: At shallow depths, skip quiet moves if a threshold     |
        // |            number of quiet moves have already been searched.              |
        // |===========================================================================|
        if (!isPv && !isInCheck && moveIsQuiet && depth <= 4 && quietMovesCount >= lateMovePruningThreshold[depth])
        {
            continue;
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

        if (score > alpha)
        {
            score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
        }

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

            // Store PV move
            pvTable[ply][ply] = move;

            // Copy move from deeper plies into current ply's line
            for (int nextPly = ply + 1; nextPly < pvLength[ply + 1]; nextPly++)
            {
                pvTable[ply][nextPly] = pvTable[ply + 1][nextPly];
            }

            // Adjust PV length
            pvLength[ply] = pvLength[ply + 1];

            if (ply == 0)
            {
                bestMove = move;
            }


            // beta cutoff
            if (score >= beta)
            {
                save(posZobrist, ttBestMove, score, staticValue, depth, Bound::BOUND_BETA, generation, ply);
                // store killer move
                if (moveIsQuiet)
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
            if (moveIsQuiet)
            {
                // store history move
                historyMoves[fromSquare][toSquare] -= depth * depth / 2.0;
            }
        }

        // Increment the quiet move counter now for stable late move pruning
        if (moveIsQuiet) quietMovesCount++;


    }
    save(posZobrist, ttBestMove, alpha, staticValue, depth, hashFlag, generation, ply);
    return alpha;
}



// This function initiates the search using iterative deepening and aspiration windows
Move MoveSearcher::findBestMove(Position pos, int& posEval)
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

    // Set nodes counter to zero before the loop
    nodes = 0;
    quieNodes = 0;

    int score;

    // Iterative deepening loop
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        Move bmDummy = bestMove;

        if (depth < 5)
        {
            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);
        } else
        {
            // Aspiration window implementation from chess engine chal (https://github.com/namanthanki/chal)
            int delta = 15 + previousScore * previousScore / 16384;
            alpha = std::max(previousScore - delta, -CHECKMATE);
            beta = std::min(previousScore + delta, CHECKMATE);

            while (true)
            {
                score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);

                // If the time management flag is set to true, it means that the search was interrupted.
                // The results of an interrupted search are discarded since they are incomplete
                if (tm.getShouldStopFlag()) break;

                if (score <= alpha)
                {
                    beta = (alpha + beta)/2;
                    alpha = std::max(alpha - delta, -CHECKMATE);
                }
                else if (score >= beta)
                {
                    beta = std::min(beta + delta, CHECKMATE);
                }
                else
                {
                    break;
                }
                delta += delta/2;
            }
        }

        if (tm.getShouldStopFlag()) break;

        bool scoreDroppedSuddenly = (previousScore != VALUE_NONE && score + 25 < previousScore);
        bool bestMoveChanged = (previousMove != NO_MOVE && bmDummy != previousMove);

        bestMove = bmDummy;
        previousScore = score;
        previousMove = bestMove;

        // Print search statistics (Negamax nodes, QSearch nodes and current search depth)
        if (!DataGenFlag) printInfo(depth, score); // only print info if we are not generating self play data


        // If the optimum time limit that the time manager set has expired
        if (tm.isTimeEnabled() && tm.optimumExpired())
        {
            // If we found a different best move at this depth or the score suddenly dropped then we need to give the engine more time to keep searching to resolve the instability
            if ((bestMoveChanged || scoreDroppedSuddenly) && !tm.maximumExpired())
            {
                continue;
            }
            break;
        }

        // Sometimes the engine goes into a deeper search only to be interrupted after a few seconds.
        // That means the engine will spend time to find a move that will be discarded anyways.
        // To prevent that, we only continue to a deeper search if the time that has elapsed is less than 3 times the optimum time limit
        // For example if we are at depth 16 then a depth 17 search will likely need 2-3x more time to be fully searched.
        // Therefore we attempt to predict if we have enough time left to finish that search, if not then we stop the search here
        if (tm.isTimeEnabled() && (tm.elapsedTime() * 2 > tm.optimum())) break;
    }

    return bestMove;

}


// Function used for move ordering, specifically for quiescent search
int MoveSearcher::scoreQuiescenceMove(const Move& move, Position& pos, const Move& hashMove)
{
    int score = 0;
    if (move == hashMove)
    {
        return 64000;
    }

    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0xF;

    // MVV-LVA for quiescence

    Piece victim = wp;
    if (flag == 5)
    {
        victim = wp;
    }
    else
    {
        victim = pos.getPieceFromBoard(toSquare);
    }
    Piece attacker = pos.getPieceFromBoard(fromSquare);

    score = 10 * averagePieceScore[victim] - averagePieceScore[attacker];

    return score;
}


// Function used to order moves from best to worst
int MoveSearcher::scoreMove(const Move& move, const Position& pos, const Move& hashMove, const int& ply)
{
    if (move == hashMove)
    {
        return 32000;
    }

    int score = 0;

    int flag = getMoveFlag(move);

    if (flag == 11) return 15000; // Reward queen promotion
    if (flag == 15) score += 17000; // Reward capture that leads to queen promotion

    // Punish under-promotions
    if (flag & 8) return -10000;

    // Source and destination squares for MVV-LVA for captures, or history for quiet moves
    int fromSquare = getFromSquare(move);
    int toSquare = getToSquare(move);

    // MVV-LVA for capture moves
    if (flag & 4)
    {
        Piece victim = wp;
        if (flag == 5)
        {
            victim = wp;
        } else
        {
            victim = pos.getPieceFromBoard(toSquare);
        }
        Piece attacker = pos.getPieceFromBoard(fromSquare);

        return score + 1500 + 10 * averagePieceScore[(int)(victim) % 6] - averagePieceScore[(int)(attacker) % 6];
    }

    // killer moves (only non captures here)
    if (move == killerMoves[ply][0])
        return 900;
    if (move == killerMoves[ply][1])
        return 850;

    return historyMoves[fromSquare][toSquare];
}