//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cmath>
#include <cstring>

#include "LegalMoveGen.h"
#include "Score.h"
#include "TranspositionTable.h"
#include "TimeManager.h"


int precomputedLMR[128][256];


// prints current depth and number of nodes searched for negamax and qsearch for the position
void MoveSearcher::printInfo(int depth, int score)
{
    std::cout << "info depth " << depth
              << " score cp " << score
              << " nodes "    << nodes
              << " nps "      << (timeManager.elapsedTime() > 0 ? nodes * 1000LL / timeManager.elapsedTime() : 0)
              << " time "     << timeManager.elapsedTime()
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
    if (uciStop || ((timeManager.isTimeEnabled() && (((nodes & 2047) == 0 && timeManager.maximumExpired()))) || (nodes >= MAX_NODES)))
    {
        // Update the 'STOP' flag of the timer manager
        timeManager.stopSearch();
        return 0;
    }

    if (uciStop || timeManager.getShouldStopFlag())
    {
        return 0;
    }

    constexpr bool isPv = nodeType == NodeType::PV;

    // Get the zobrist hash of the position in order to probe the Transposition Table for this position
    ZobristHash zobrist = pos.getZobristHash();

    int bestScore;

    // We also calculate the static evaluation of the position
    // If we are lucky it is already stored inside the transposition table, saving us a lot of time
    int staticEvaluation;


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
    if (!isPv && ttHit && ttData.depth >= Q_DEPTH && ( ( ttData.bound == Bound::BOUND_EXACT ) ||
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
        staticEvaluation = ttData.staticEval; // Static Evaluation

        ttBestMove = ttData.bestMove; // Best Move
    }
    // Otherwise we need to manually calculate the static evaluation
    else
    {
        staticEvaluation = scoreBoardNNUE(pos, ss->accumulator);
    }

    ss->staticEval = staticEvaluation;

    Bound hashFlag = Bound::BOUND_ALPHA;


    // Stand-pat
    bestScore = staticEvaluation;
    bool inCheck = pos.sideToMoveIsInCheck();


    // If we are in check then we can't trust the static evaluation to determine the score of the position
    if (inCheck)
    {
        bestScore = -CHECKMATE;
    }
    else
    {
        // |==========================================================================================|
        // | Delta Pruning (Before the search loop) : If our best score (stand pat) is so far below   |
        // |    alpha that even capturing a Queen wouldn't raise our score above alpha,               |
        // |  this node is doomed to fail-low. We can safely prune the entire branch right now        |
        // |           instead of generating and testing captures in the loop.                        |
        // |==========================================================================================|
        if (bestScore < alpha - 980)
        {
            return bestScore;
        }

        // |================================================================================================|
        // | Stand-Pat Cutoff : The static evaluation serves as a lower bound. If the stand-pat score is    |
        // |   already >= beta, the position is "too good" and the opponent would have deviated earlier.    |
        // | We return the evaluation immediately (fail-high) without generating or searching any captures. |
        // |================================================================================================|
        if (bestScore >= beta)
        {
            // Store the data we have found in the transposition table, only if a valid best move has been found
            if (ttBestMove != NO_MOVE)
            {
                save(zobrist, ttBestMove, bestScore, staticEvaluation, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
            }
            return bestScore;
        }
    }

    nodes++;

    if (bestScore > alpha)
    {
        alpha = bestScore;
    }

    Move moves[128];
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

    if (inCheck)
    {
        generateLegalMoves(info, pos, moves, numOfMoves);
    }
    else
    {
        // Generate capture moves using previously calculated legality info
        generateCaptures(info, pos, moves, numOfMoves);
    }

    int moveScores[numOfMoves];

    // Get the score of every move for move ordering
    for (int i = 0; i < numOfMoves; i++)
    {
        moveScores[i] = scoreMove(moves[i], pos, ttBestMove);
    }


    for (int i = 0; i < numOfMoves; i++)
    {
        int nextMoveIndex = i;

        // Pick the next best capture
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

        if (!inCheck)
        {
            Piece capturedPiece = pos.getPieceFromBoard(toSquare);
            int capturedPieceScore = averagePieceScore[0];

            if (capturedPiece != NO_PIECE)
            {
                capturedPieceScore = averagePieceScore[capturedPiece];
            }

            // |==========================================================================================|
            // | Delta Pruning (Inside the search loop) : If the best score found so far below alpha that |
            // |    below alpha that even after the capture is played, then this node is most likely      |
            // |   going to fail-low. We can safely prune the entire branch right now instead of          |
            // |           generating and testing  captures in the loop.                                  |
            // |==========================================================================================|
            if (bestScore + capturedPieceScore + 200 < alpha)
            {
                continue;
            }


            // SEE pruning
            int attacker = pos.getPieceFromBoard(fromSquare);
            // SEE is computationally expensive so we can limit it strictly to captures where the attacker is more valuable than the victim
            if (averagePieceScore[attacker] > averagePieceScore[capturedPiece])
            {
                // Skip move if SEE returns less than 0 meaning losing series of captures
                if (pos.SEE(toSquare, capturedPiece, fromSquare, attacker) < 0) continue;
            }
        }

        // Update the next ply's accumulator by registering the move
        (ss+1)->accumulator = ss->accumulator;
        (ss+1)->accumulator.makeMove(move, pos);

        // Make the move to get its score
        pos.makeMove(move);
        int score = -quiescence<nodeType>(pos, -beta, -alpha, NO_MOVE, ply + 1, ss+1);
        pos.unmakeMove();


        if (score > bestScore) bestScore = score;

        if (score > alpha)
        {
            alpha = score;

            hashFlag = Bound::BOUND_EXACT;
            ttBestMove = move;
        }

        if (alpha >= beta)
        {
            save(zobrist, ttBestMove, score, staticEvaluation, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
            break;
        }

    }
    save(zobrist, ttBestMove, bestScore, staticEvaluation, Q_DEPTH, hashFlag, generation, ply);
    return bestScore;
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

    if (uciStop || ((timeManager.isTimeEnabled() && ((nodes & 2047) == 0) && timeManager.maximumExpired()) || nodes >= MAX_NODES))
    {
        // Update the 'STOP' flag of the timer manager
        timeManager.stopSearch();
        return 0;
    }

    if (uciStop || timeManager.getShouldStopFlag())
    {
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

    // Extend search depth if side to move is in check
    if (isInCheck && ss->previousExtensions < 4)
    {
        ss->previousExtensions++;
        depth++;
    }

    ss->inCheck = isInCheck;
    ss->staticEval = VALUE_NONE;

    //    |===============================================================================================|
    //    |  Quiescent Search : Once the search reaches a depth of 0, it hits the standard search limit.  |
    //    |   However if we were to return the score as-is, it would be prone to the most common horizon  |
    //    |     effect - specifically missing captures and promotions. Quiescent search continues         |
    //    |         the search, limiting itself to captures and promotions. This ensures that             |
    //    |          the final leaf nodes are stable, quiet and most importantly trustworthy              |
    //    |===============================================================================================|

    if (depth <= 0)
    {
        // Drop into quiescent search - we are not in check
        return quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
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

    ss->staticEval = staticValue;

    { // Unused code (for now)
        int improving;
        int worsening;

        if (isInCheck)
        {
            improving = false;
            worsening = true;
        }
        else if (ply >= 2 && (ss - 2)->staticEval != VALUE_NONE)
        {
            improving = ss->staticEval > (ss - 2)->staticEval;
            worsening = ss->staticEval < (ss - 2)->staticEval;
        }
        else if (ply >= 4 && (ss - 4)->staticEval != VALUE_NONE)
        {
            improving = ss->staticEval > (ss - 4)->staticEval;
            worsening = ss->staticEval < (ss - 4)->staticEval;
        }
        else
        {
            improving = true;
            worsening = false;
        }
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
            int reverseFutilityMargin = 70 * depth;

            // If the RFP condition is met simply return the static evaluation
            if (staticValue - reverseFutilityMargin >= beta) return staticValue - reverseFutilityMargin;
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
            int margin = 300 + depth * 60;
            if (staticValue + margin < alpha)
            {
                return quiescence<nodeType>(pos, alpha, beta, NO_MOVE, ply, ss);
            }
        }

        // |====================================================================================================|
        // |  Null Move Pruning : If our position is so strong that it still beats Beta even after              |
        // |     doing nothing, it is safe to assume that making a real move would also beat Beta.              |
        // |   We search this "null move" at a reduced depth. If the score returned is still above beta, we     |
        // |   can safely prune the branch and save a lot of time. We just check the game phase first to make   |
        // |   sure we aren't in an endgame where skipping a could be good in certain cases (zugzwang).         |
        // |====================================================================================================|
        if (ply > 0 && (alpha == beta - 1) && allowNullMove && depth > 3 && pos.getGamePhase() && beta < CHECKMATE - MAX_PLY) // Do not play a null move twice in a row
        {
            int reduction = std::min(depth, 4 + depth / 3); // NMP Reduction
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

    int quietMoves[256];

    int moveScores[numOfMoves];

    for (int i = 0; i < numOfMoves; i++)
    {
        // Get the score of every move
        moveScores[i] = scoreMove(moves[i], pos, ttBestMove, ply);
    }

    int score;

    // Begin searching all the moves until we either run out of moves or a cutoff occurs
    for (int i = 0; i < numOfMoves; i++)
    {
        int maxScore = moveScores[i];
        int nextMoveIndex = i;

        // Pick the move with the highest score
        for (int j = i + 1; j < numOfMoves; j++)
        {
            if (moveScores[j] > maxScore)
            {
                maxScore = moveScores[j];
                nextMoveIndex = j;
            }
        }

        // Swap the best move found to the beginning of the list so that we don't loop through it at the next iteration
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
        bool moveIsCapture = isCapture(moveFlag);
        bool moveIsPromotion = isPromotion(moveFlag);

        // If it is neither a capture nor a promotion, then the move is quiet
        bool moveIsQuiet = !(moveIsCapture || moveIsPromotion);

        // |=================================================================================================|
        // |  Principal Variation Search : The first move is the most promising. For this reason, we search  |
        // |   it with a full window, while the remaining moves are searched with a limited 'null window'.   |
        // |        If the move stayed within the null window, it is re-searched with a normal one           |
        // |=================================================================================================|
        if (i == 0)
        {
            // Update the accumulator for the next ply
            (ss+1)->accumulator = ss->accumulator;
            (ss+1)->accumulator.makeMove(move, pos);


            pos.makeMove(move);

            // Search the first move
            score = -negaMaxAlphaBeta<nextNodeType>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);

            pos.unmakeMove();
        }
        else
        {
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
            if (!isPv && !isInCheck && moveIsQuiet && depth <= 4)
            {
                int LMP_Threshold = lateMovePruningThreshold[depth];

                if (quietMovesCount >= LMP_Threshold) continue;
            }

            // Update the accumulator for the next ply
            (ss+1)->accumulator = ss->accumulator;
            (ss+1)->accumulator.makeMove(moves[i], pos);

            pos.makeMove(moves[i]);

            // Base depth reduction is 1
            int depthReduction = 1;

            // |===========================================================================|
            // |   Late Move Reductions (LMR): Since the moves are ordered from most       |
            // |  promising to least, the deeper we move into the list the worse a move    |
            // |  will statistically be. We take advantage of this by reducing the depth   |
            // |  at which later moves are searched. To keep the search stable, we limit   |
            // |      the reductions strictly to quiet moves that do not give check.       |
            // |===========================================================================|
            if (depth > 3 && i > 3 && !isInCheck && moveIsQuiet)
            {
                // Reduce the depth of the current move is the hash move is a capture or a promotion
                // because it probably means the hash move line is much stronger than the current
                // Also reduce depth if there is no hash move. A branch without a hash move is usually less important,
                // meaning we can reduce the depth at which we will search it to save time for more important nodes
                if (ttBestMove == NO_MOVE || ttMoveIsCapture || ttMoveIsPromo) depthReduction++;

                depthReduction += precomputedLMR[depth][i];
            }

            // Make sure the depth reduction does not lead to a depth less than zero
            int reducedDepth = std::max(0, depth - depthReduction);


            // LMR + PVS : Depth reduction combined with a null window for moves after the first.
            // This way we spend less time searching moves that are not that promising
            score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, reducedDepth, bestMove, ply+1, rootDepth, true, ss+1);

            // If the score found from the LMR + PVS search exceeds alpha, then the move proved to be worthy of further analysis.
            // However first we need to confirm that LMR was applied for this move. If it was then we re-search it with LMR deactivated.
            if (score > alpha && depthReduction > 1)
            {
                score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
            }

            // If we are at a PV node
            if constexpr (isPv)
            {
                // If the score returned by this move still exceeds alpha with LMR de-activated:
                // do a full-window re-search if the score is inside the exact window (score < beta),
                // or if we are at the root, because at the root we must play a move and for that we need to know its exact score
                if ((score > alpha) && (rootNode || score < beta))
                {
                    // The moves that reach this point are proved worthy and the node type passed down the search is PV instead of NonPV like the rest
                    score = -negaMaxAlphaBeta<NodeType::PV>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
                }
            }
            pos.unmakeMove();
        }

        // Increment the quiet move counter now for stable late move pruning
        if (moveIsQuiet) quietMoves[quietMovesCount++] = move;

        if (score > alpha)
        {
            // Set the node's bound to exact. Later on if we find out that it exceeds beta it is set to beta bound instead
            hashFlag = Bound::BOUND_EXACT;

            // Update alpha
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

            // Only update the best move found if we are at the root
            if (ply == 0)
            {
                bestMove = move;
            }

            // The score exceeds beta, the opponent will deviate from this line so we can prune this branch
            if (score >= beta)
            {
                // Save the node with beta bound since it exceeds beta
                save(posZobrist, ttBestMove, score, staticValue, depth, Bound::BOUND_BETA, generation, ply);

                Color sideToMove = pos.isWhiteToMove() ? White : Black;

                // The move that caused the cutoff receives a bonus based on current depth
                const int bonus = 300 * depth - 250;

                // If the move is quiet update its history score
                if (moveIsQuiet)
                {
                    updateHistory(sideToMove, fromSquare, toSquare, bonus);

                    // The move caused a beta cutoff so we also update the killers for this ply
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                }

                // We also penalize the quiet moves prior to the one we just searched,
                // since they failed to cause a cutoff and are, therefore, weaker moves
                for (int j = 0; j < quietMovesCount; j++)
                {
                    Move quietMove = quietMoves[j];

                    if (quietMove == move) continue; // Don't penalize the move that caused the beta cutoff

                    int qFromSquare = getFromSquare(quietMove);
                    int qToSquare = getToSquare(quietMove);

                    // Call the update history function but with a negative bonus
                    updateHistory(sideToMove, qFromSquare, qToSquare, -bonus);
                }
                // Beta cutoff
                return beta;
            }
        }
    }

    save(posZobrist, ttBestMove, alpha, staticValue, depth, hashFlag, generation, ply);
    return alpha;
}



// This function initiates the search using iterative deepening and aspiration windows
Move MoveSearcher::findBestMove(Position pos, int& posEval)
{
    // Increment generation for transposition table aging replacement scheme
    generation++;

    // Reset killer moves
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

    int score;

    // Iterative deepening loop
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
        if (uciStop) break;

        Move bmDummy = bestMove;

        // Stop if the elapsed time is larger than 90% of the maximum time limit
        if (timeManager.isTimeEnabled() && timeManager.elapsedTime() > (timeManager.maximum() * 0.90))
        {
            timeManager.stopSearch();
            break;
        }

        if (depth < 4)
        {
            score = negaMaxAlphaBeta<NodeType::Root>(pos, alpha, beta, depth, bmDummy, 0, depth, false, ss);
        }
        else
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
                if (timeManager.getShouldStopFlag()) break;

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

        if (uciStop || timeManager.getShouldStopFlag()) break;

        bool scoreDroppedSuddenly = (previousScore != VALUE_NONE && score + 25 < previousScore);
        bool bestMoveChanged = (previousMove != NO_MOVE && bmDummy != previousMove);

        bestMove = bmDummy;
        previousScore = score;
        previousMove = bestMove;

        // Print search statistics
        if (!DataGenFlag) printInfo(depth, score); // Only print info if we are not generating self-play data

        // If the optimum time limit that the time manager set has expired
        if (uciStop || timeManager.isTimeEnabled() && timeManager.optimumExpired())
        {
            // If we found a different best move at this depth or the score suddenly dropped then we need to give the engine more time to keep searching to resolve the instability
            if ((bestMoveChanged || scoreDroppedSuddenly) && !timeManager.maximumExpired())
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
        if (uciStop || timeManager.isTimeEnabled() && (timeManager.elapsedTime() * 3 > timeManager.optimum())) break;
    }

    return bestMove;

}



// Function used to order moves from best to worst
int MoveSearcher::scoreMove(Move move, const Position& pos, Move hashMove, int ply)
{
    if (move == hashMove)
    {
        return HashMoveScore;
    }

    int score = 0;

    int flag = getMoveFlag(move);

    if (flag == 11) return 150000; // Reward queen promotion
    if (flag == 15) score += 170000; // Reward capture that leads to queen promotion

    // Punish under-promotions
    if (flag & 8) return -100000;

    // Source and destination squares for MVV-LVA for captures, or history for quiet moves
    int fromSquare = getFromSquare(move);
    int toSquare = getToSquare(move);

    // MVV-LVA for capture moves
    if (isCapture(getMoveFlag(move)))
    {
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

        return score + 25000 + 10 * averagePieceScore[victim] - averagePieceScore[attacker];
    }

    if (ply >= 0)
    {
        // Killer moves
        if (move == killerMoves[ply][0]) return KillerMoveScore0;
        if (move == killerMoves[ply][1]) return KillerMoveScore1;
    }

    return historyScores[pos.isWhiteToMove() ? White : Black][fromSquare][toSquare];
}

// Function to update the history score of a move
// The value argument is the bonus/penalty applied to the move
void MoveSearcher::updateHistory(Color sideToMove, int fromSquare, int toSquare, int value)
{
    int clampedBonus = std::clamp(value, -MaxHistoryScore, MaxHistoryScore);
    historyScores[sideToMove][fromSquare][toSquare] += clampedBonus - historyScores[sideToMove][fromSquare][toSquare] * std::abs(clampedBonus) / MaxHistoryScore;
}