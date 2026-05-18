//
// Created by theoa on 14/03/2026.
//

#include "Search.h"

#include <cmath>
#include <cstring>

#include "LegalMoveGen.h"
#include "MovePicker.h"
#include "Score.h"
#include "TranspositionTable.h"
#include "Time.h"


int precomputedLMR[128][256];


// prints current depth and number of nodes searched for negamax and qsearch for the position
void MoveSearcher::printInfo(int depth, int score)
{
    std::cout << "info depth " << depth << " score cp " << score << " nodes " << nodes << " pv ";

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
    if (tm.isTimeEnabled() && (((nodes & 2047) == 0 && tm.maximumExpired()) || (nodes >= MAX_NODES)))
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


    // Stand-pat
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

    nodes++; // Increment nodes counter after passing delta pruning and TT cutoffs

    if (bestValue > alpha)
    {
        alpha = bestValue;
    }


    // |=================================================================================================|
    // |  Legality Information : Because we generate pseudo-legal moves, many of them might be illegal   |
    // |   since they leave our king in check. To quickly filter out these bad moves, we need to know    |
    // |     exactly what is happening around our king. Instead of checking this for every single move,  |
    // |   we pre-calculate a struct containing all the current checkers, pinners, and pinned pieces.    |
    // |    By collecting this info right before we start, we make move validation fast and simple.      |
    // |=================================================================================================|
    Color allyColor = pos.isWhiteToMove() ? White : Black;
    int kingSquare = pos.isWhiteToMove() ? lsbIndex(pos.getPieceBitboard(wK)) : lsbIndex(pos.getPieceBitboard(bK));

    legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);


    history hDummy{}; // Dummy history node because the Move Picker uses move history only for quiet moves
    MovePicker movePicker(pos, info, &hDummy, ttBestMove, true);

    Move move;

    // The main search loop. Repeat until the move picker runs out of moves
    while ((move = movePicker.nextMove()) != NO_MOVE)
    {
        // Extract info about the move, specifically the source and destination square as well as its flag
        int fromSquare = getFromSquare(move);
        int toSquare = getToSquare(move);
        int moveFlag = getMoveFlag(move);

        bool isPromo = isPromotion(moveFlag);

        int capturedPieceValue;
        Piece movingPiece = pos.getPieceFromBoard(fromSquare);
        Piece capturedPiece;

        // If the move is en-passant then the captured piece is a pawn
        if (isEnpassant(moveFlag))
        {
            capturedPiece = movingPiece < 6 ? bp : wp;
            capturedPieceValue = averagePieceScore[capturedPiece];
        }
        else
        {
            capturedPiece = pos.getPieceFromBoard(toSquare);
            capturedPieceValue = averagePieceScore[capturedPiece];
        }

        // |=================================================================================|
        // |  Delta Pruning (Inside the search loop) : If our best score plus the captured   |
        // |  piece's value (and a small safety margin) is still below alpha, this move is   |
        // |       not promising enough to raise our score. We skip it and move on.          |
        // |=================================================================================|
        if (!isPromo && (bestValue + capturedPieceValue + 200 < alpha))
        {
            continue;
        }


        // |=========================================================================================|
        // |      SEE Pruning : If the full exchange value of a capture is found to be negative      |
        // |   then the sequence is losing we skip this move as it is likely to result in a losing   |
        // |     position. Only check the SEE score if the attacker's value is greater than the      |
        // |    victim's to limit SEE to important captures since it is computationally expensive.   |
        // |=========================================================================================|

        if (!isPromo && averagePieceScore[movingPiece] > capturedPieceValue)
        {
            if (pos.SEE(toSquare, capturedPiece, fromSquare, movingPiece) < 0) continue;
        }

        // Update the network's accumulator
        (ss+1)->accumulator = ss->accumulator;
        (ss+1)->accumulator.makeMove(move, pos);

        // make move, search it then unmake to get its score from qsearch
        pos.makeMove(move);
        int score = -quiescence<nodeType>(pos, -beta, -alpha, NO_MOVE, ply + 1, ss+1);
        pos.unmakeMove();

        // If we found a score higher than the best score we have found so far
        if (score > bestValue)
        {
            bestValue = score;

            if (score > alpha)
            {
                hashFlag = Bound::BOUND_EXACT;
                ttBestMove = move;
                if (score < beta)
                {
                    alpha = score;
                }
                else
                {
                    save(zobrist, ttBestMove, score, staticValue, Q_DEPTH, Bound::BOUND_BETA, generation, ply);
                    return score;
                }

            }
        }
    }

    bool searchInterrupted = tm.getShouldStopFlag();
    if (!searchInterrupted) save(zobrist, ttBestMove, bestValue, staticValue, Q_DEPTH, hashFlag, generation, ply);

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

    }
    // If the probe was not succesful we need to calculate the static evaluation of the position
    else
    {
        // Call the NNUE evaluation function
        staticValue = scoreBoardNNUE(pos, ss->accumulator);
    }


    // Futility & Razoring
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



    // |=================================================================================================|
    // |  Legality Information : Because we generate pseudo-legal moves, many of them might be illegal   |
    // |   since they leave our king in check. To quickly filter out these bad moves, we need to know    |
    // |     exactly what is happening around our king. Instead of checking this for every single move,  |
    // |   we pre-calculate a struct containing all the current checkers, pinners, and pinned pieces.    |
    // |    By collecting this info right before we start, we make move validation fast and simple.      |
    // |=================================================================================================|
    Color allyColor = pos.isWhiteToMove() ? White : Black;
    int kingSquare = pos.isWhiteToMove() ? lsbIndex(pos.getPieceBitboard(wK)) : lsbIndex(pos.getPieceBitboard(bK));

    legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);



    // |=================================================================================================|
    // |  Move Picker : Generating every possible move at once wastes time, especially if a branch is    |
    // |   going to be pruned early. Instead, Aspen's Move Picker handles generation in smart stages.    |
    // |     It has three main jobs: generating moves in an optimal order, scoring them, and verifying   |
    // |   their legality. It picks the most promising options first: 1. Hash Move, 2. Good Captures,    |
    // |    3. Killer Moves, 4. Quiet Moves, and 5. Bad Captures. When we ask for the next move, it      |
    // |    automatically generates it, validates it, and scores it (using MVV-LVA for captures and      |
    // |        history for quiet moves). This ensures that the search is simple and clean.              |
    // |=================================================================================================|
    MovePicker movePicker(pos, info, &historyMoves, ttBestMove, false, killerMoves[ply][0], killerMoves[ply][1]);


    // Keep track of the searched moves count for PVS (Principaled Variation Search)
    int searchedMovesCount = 0;
    // Keep track of quiet moves searched for late move pruning (LMP)
    int quietMovesCount = 0;
    Move quietMoves[256];

    int legalMovesCount = 0;

    Move move;

    // The main search loop. Repeat until the move picker runs out of moves
    while ((move = movePicker.nextMove()) != NO_MOVE)
    {
        legalMovesCount++;
        // Extract the move's flag for later use
        int moveFlag = getMoveFlag(move);

        // Extract the source and destination squares of the move
        int fromSquare = getFromSquare(move);
        int toSquare = getToSquare(move);

        // Get the piece that is moving
        Piece movingPiece = pos.getPieceFromBoard(fromSquare);

        // Check if the move is a capture or/and promotion
        bool moveIsCapture = isCapture(move);
        bool moveIsPromotion = isPromotion(move);

        // If it is neither, the move is quiet
        bool moveIsQuiet = !(moveIsCapture || moveIsPromotion);

        // Add it to the quiet moves list and increment the quiet moves index
        if (moveIsQuiet) quietMoves[quietMovesCount++] = move;

        // |================================================================================================|
        // |  SEE Pruning : Skip captures whose full exchange value is below a depth-scaled threshold.      |
        // |   Only applied after atleast one legal move has been played so we never prune the first move.  |
        // |             Code from https://github.com/namanthanki/chal/blob/main/src/chal.c                 |
        // |================================================================================================|
        //if (!isPv && !isInCheck && moveIsCapture && !moveIsPromotion && searchedMovesCount > 1)
        //{
        //    // First get the value of the piece captured piece
        //    int capturedPieceValue;
        //    Piece capturedPiece;

        //    // If the move is en-passant then the captured piece is a pawn
        //    if (isEnpassant(moveFlag))
        //    {
        //        capturedPiece = movingPiece < 6 ? bp : wp;
        //        capturedPieceValue = averagePieceScore[capturedPiece];
        //    }
        //    else
        //    {
        //        capturedPiece = pos.getPieceFromBoard(toSquare);
        //        capturedPieceValue = averagePieceScore[capturedPiece];
        //    }

        //    // Only check the SEE (Static Exchange Evaluation) score if the attacker's value is greater than the victim's
        //    if (averagePieceScore[movingPiece] > capturedPieceValue)
        //    {
        //        if (pos.SEE(toSquare, capturedPiece, fromSquare, movingPiece) < (-averagePieceScore[wp] * depth)) continue;
        //    }
        //}



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

        // Increment searched moves only after futility pruning and SEE pruning
        searchedMovesCount++;

        // |================================================================================================|
        // | Late Move Pruning: At shallow depths, skip quiet moves if a threshold number of quiet moves    |
        // | have already been searched.                                                                    |
        // |================================================================================================|
        if (!isPv && !isInCheck && moveIsQuiet && depth <= 4 && quietMovesCount >= lateMovePruningThreshold[depth])
        {
            continue;
        }

        // Update the accumulator for the next ply
        (ss+1)->accumulator = ss->accumulator;
        (ss+1)->accumulator.makeMove(move, pos);

        // Make the move
        pos.makeMove(move);

        // Extend the search depth by one every time a move gives check
        //if (givesCheck && ply < 20) depth++;

        int score;


        // |=================================================================================================|
        // |  Principal Variation Search : The first move is the most promising. For this reason, we search  |
        // |   it with a full window, while the remaining moves are searched with a limited 'null window'.   |
        // |        If the move stayed within the null window, it is re-searched with a normal one           |
        // |=================================================================================================|
        if (searchedMovesCount == 1)
        {
            // PVS: Full window
            score = -negaMaxAlphaBeta<nodeType>(pos, -beta, -alpha, depth - 1, bestMove, ply+1, rootDepth, true, ss+1);
        }
        // If this move is not the first to be searched
        else
        {
            // |===============================================================================================|
            // |   Late Move Reductions: Good move ordering ensures that the best moves are searched first.    |
            // |  This allows us to reduce the search depth of later, less promising moves on the assumption   |
            // |   that they are unlikely to surpass the scores of the top moves provided by the move picker   |
            // |===============================================================================================|

            // Base reduction is 1
            int depthReduction = 1;

            // We only use LMR if certain conditions are met:
            // 1. Current depth is greater than 3
            // 2. The first 4 moves -that are likely to be good- have been searched without LMR
            // 3. We are not in check
            // 4. The move is quiet, otherwise reducing depth would be unstable
            if (depth > 3 && searchedMovesCount > 3 && !isInCheck && moveIsQuiet)
            {
                depthReduction = precomputedLMR[depth][searchedMovesCount];
            }

            // Keep the final depth (after LMR) non-negative
            int reducedDepth = std::max(0, depth - depthReduction);

            // First search the move at null window length and with LMR
            score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, reducedDepth, bestMove, ply+1, rootDepth, true, ss+1);

            // If the score beat alpha we re-search without the LMR reduction
            if (score > alpha && depthReduction > 1)
            {
                score = -negaMaxAlphaBeta<NodeType::NonPV>(pos, -alpha-1, -alpha, (depth - 1), bestMove, ply+1, rootDepth, true, ss+1);
            }

            // If the score beat alpha again then re-search with a full window and no LMR - Only if the node is PV
            if constexpr (isPv)
            {
                if ((score > alpha) && (rootNode || score < beta))
                {
                    // The node becomes PV
                    score = -negaMaxAlphaBeta<NodeType::PV>(pos, -beta, -alpha, (depth - 1), bestMove, ply+1, rootDepth, true, ss+1);
                }
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
                // Store killer move and update history move scores
                if (moveIsQuiet)
                {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;

                    int bonus = depth * depth;
                    int history = historyMoves[fromSquare][toSquare];
                    history += bonus - history * bonus / 16000;
                    historyMoves[fromSquare][toSquare] = history > 16000 ? 16000 : history;

                    for (int j = 0; j < quietMovesCount; j++) {
                        Move quietMove = quietMoves[j];

                        // Do not penalize the move that caused the beta cutoff
                        if (quietMove == move) continue;

                        // Extract the quiet move's source and destination squares
                        int quietMoveFrom = getFromSquare(quietMove);
                        int quietMoveTo = getToSquare(quietMove);

                        int hm = historyMoves[quietMoveFrom][quietMoveTo];
                        hm -= bonus + hm * bonus / 16000;
                        historyMoves[quietMoveFrom][quietMoveTo] = hm < -16000 ? -16000 : hm;
                    }
                }

                return beta; // hard beta cutoff
            }
        }
    }


    // If no moves were generated then the side to move got checkmated or the game ended in stalemate
    if (legalMovesCount == 0)
    {
        if (isInCheck)
        {
            return -CHECKMATE + ply; // Add ply to the CHECKMATE score to encourage avoiding checkmate for as long as possible
        }
        return 0;
    }

    bool searchInterrupted = tm.getShouldStopFlag();
    if (!searchInterrupted) save(posZobrist, ttBestMove, alpha, staticValue, depth, hashFlag, generation, ply);

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

    // Iterative deepening loop
    for (int depth = 1; depth <= MAX_DEPTH; depth++)
    {
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
            if (((previousMove != NO_MOVE && previousMove != bestMove) || (previousScore != VALUE_NONE && ((previousScore - 50) > score))) && !tm.maximumExpired())
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