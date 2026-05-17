//
// Created by theoa on 16/05/2026.
//

#include "MovePicker.h"

#include "PseudoMoveGen.h"

Move MovePicker::nextMove()
{
    while (phase != MovePickerPhase::End)
    {
        switch(phase)
        {
            //    |==========================================================|
            //    |  1. Hash Move : If there is a hash move and it is legal, |
            //    |      then pick it as the first move to be searched       |
            //    |==========================================================|
            case MovePickerPhase::HashMove:
            {
                // Move on to the next phase
                phase = MovePickerPhase::GenAndScoreCapsAndPromos;

                if (hashMove != NO_MOVE && position.moveIsLegal(legalityInfo, hashMove))
                {
                    return hashMove;
                }

                // If no valid hash move was found move on to the next stage - The generation of every capture and promotion move
                continue;
            }
            //    |=================================================================================|
            //    |  2. Cap And Promo Moves Generation Phase : Here we generate every capture and   |
            //    |    promotion move. After that we also score all the moves we just generated.    |
            //    |   This prepares us for phase 3 where we pick the best promotion/capture move    |
            //    |=================================================================================|
            case MovePickerPhase::GenAndScoreCapsAndPromos:
            {
                // Move on to the next phase
                phase = MovePickerPhase::WinningCapturesAndPromos;

                // Generate captures and promotions
                generatePseudoLegalCapAndPromoMoves(position, moves, numOfMoves);

                // Score the moves we just generated
                scoreCapAndPromoMoves();

                // Store the ending index of the losing captures/promotions for the losing captures/promotions phase
                losingCapAndPromoEndIndex = numOfMoves;

                break;
            }
            //    |=================================================================================|
            //    |  3. Winning Captures And Promotions :  After the generation phase is over,      |
            //    |   the move picker will loop through all the generated moves and pick the one    |
            //    |             with the highest score, which was calculated at phase 2             |
            //    |=================================================================================|
            case MovePickerPhase::WinningCapturesAndPromos:
            {
                // Loop through all the captures/promotions and pick the highest rated move
                while (currentMoveIndex < numOfMoves)
                {
                    int bestScore = -9999999;
                    int bestMoveIndex = currentMoveIndex;

                    // Loop through all the moves and their scores to find the best move
                    for (int i = currentMoveIndex; i < numOfMoves; i++)
                    {
                        if (moveScores[i] > bestScore)
                        {
                            bestScore = moveScores[i];
                            bestMoveIndex = i;
                        }
                    }

                    // If the best score found is below 0 it means we have run out of winning captures and promotions
                    if (bestScore < 0)
                    {
                        losingCapAndPromoStartIndex = currentMoveIndex;
                        // Move on to the next phase
                        phase = MovePickerPhase::FirstKillerMove;
                        break;
                    }

                    Move bestMove = moves[bestMoveIndex];

                    // Swap the best move to the front of the moves list so that we dont loop over it on the next iteration
                    // Same with the move score and the move scores list
                    std::swap(moveScores[bestMoveIndex], moveScores[currentMoveIndex]);
                    std::swap(moves[bestMoveIndex], moves[currentMoveIndex++]);


                    if (bestMove == hashMove) continue;

                    if (position.moveIsLegal(legalityInfo, bestMove))
                    {
                        return bestMove;
                    }
                }
                // Move on to the next phase
                phase = MovePickerPhase::FirstKillerMove;
                break;
            }
            //    |=====================================================================================|
            //    |  4 & 5. Killer Moves :  After having searched every winning capture and promotion,  |
            //    |       the next phase is to pick the two killer moves provided they are valid        |
            //    |=====================================================================================|
            case MovePickerPhase::FirstKillerMove:
            {
                phase = MovePickerPhase::SecondKillerMove;
                if (firstKillerMove != NO_MOVE && firstKillerMove != hashMove && position.moveIsLegal(legalityInfo, firstKillerMove))
                {
                    return firstKillerMove;
                }
                break;
            }
            // Second killer move
            case MovePickerPhase::SecondKillerMove:
            {
                phase = MovePickerPhase::GenAndScoreQuietMoves;
                if (secondKillerMove != NO_MOVE && secondKillerMove != hashMove && position.moveIsLegal(legalityInfo, secondKillerMove))
                {
                        return secondKillerMove;
                }
                break;
            }
            //    |======================================================================================|
            //    |  6. Generate And Score Quiet Moves :  After searching the killer moves we move on    |
            //    |  to normal quiet moves. We generate and score them in preparation for the next step  |
            //    |======================================================================================|
            case MovePickerPhase::GenAndScoreQuietMoves:
            {
                // Set the current move index past the losing captures and promotions from phase 3
                currentMoveIndex = numOfMoves;

                // Move on to the next phase
                phase = MovePickerPhase::QuietMoves;

                // Generate quiet moves
                generatePseudoLegalQuietMoves(position, moves, numOfMoves);

                // Score every quiet move
                scoreQuietMoves();

                break;
            }
            //    |=====================================================================|
            //    |  7. Quiet Moves :  After the quiet moves generation phase is over,  |
            //    |   the move picker will loop through all the generated quiet moves   |
            //    |             and pick the one with the highest score                 |
            //    |=====================================================================|
            case MovePickerPhase::QuietMoves:
            {
                // Loop through all the quiet moves and pick the one that has the highest score
                while (currentMoveIndex < numOfMoves)
                {
                    int bestScore = -9999999;
                    int bestMoveIndex = currentMoveIndex;

                    // Loop through all the moves and their scores to find the best move
                    for (int i = currentMoveIndex; i < numOfMoves; i++)
                    {
                        if (moveScores[i] > bestScore)
                        {
                            bestScore = moveScores[i];
                            bestMoveIndex = i;
                        }
                    }

                    Move bestMove = moves[bestMoveIndex];

                    // Swap the best move to the front of the moves list so that we dont loop over it on the next iteration
                    // Same with the move score and the move scores list
                    std::swap(moveScores[bestMoveIndex], moveScores[currentMoveIndex]);
                    std::swap(moves[bestMoveIndex], moves[currentMoveIndex++]);


                    if (bestMove == hashMove || bestMove == firstKillerMove || bestMove == secondKillerMove) continue;

                    // Check for legality and only then return the move
                    if (position.moveIsLegal(legalityInfo, bestMove))
                    {
                        return bestMove;
                    }
                }
                // Move on to the next phase
                phase = MovePickerPhase::LosingCapturesAndPromos;
                break;
            }
            //    |========================================================================================================|
            //    |  8. Losing Captures And Under Promotions : At phase 3 we only picked winning captures and promotions.  |
            //    |         When we found the first losing capture/promotion we stored the index it was found at.          |
            //    |         Therefore, we can resume their search at phase 8 which ensures that the best captures          |
            //    |             and promotions have already been searched leading to cutoffs most of the time              |
            //    |========================================================================================================|
            case MovePickerPhase::LosingCapturesAndPromos:
            {
                // Set the current index to the one we found at phase 3 to begin searching losing captures/promotions
                currentMoveIndex = losingCapAndPromoStartIndex;

                // Loop through all the losing captures/promotions that we generated at phase 2.
                while (currentMoveIndex < losingCapAndPromoEndIndex)
                {
                    int bestScore = -9999999;
                    int bestMoveIndex = currentMoveIndex;

                    // Loop through all the moves and their scores to find the best move
                    for (int i = currentMoveIndex; i < losingCapAndPromoEndIndex; i++)
                    {
                        if (moveScores[i] > bestScore)
                        {
                            bestScore = moveScores[i];
                            bestMoveIndex = i;
                        }
                    }

                    Move bestMove = moves[bestMoveIndex];

                    // Swap the best move to the front of the moves list so that we dont loop over it on the next iteration
                    // Same with the move score and the move scores list
                    std::swap(moveScores[bestMoveIndex], moveScores[currentMoveIndex]);
                    std::swap(moves[bestMoveIndex], moves[currentMoveIndex++]);

                    // Discard the move if it is the same as the hash move - it has already been searched
                    if (bestMove == hashMove) continue;

                    // Check for legality and only then return the move
                    if (position.moveIsLegal(legalityInfo, bestMove))
                    {
                        return bestMove;
                    }
                }
                // Move on to the next phase
                phase = MovePickerPhase::End;
                break;
            }
            //    |===================================================|
            //    |  9. The end : Every phase has been completed and  |
            //    |   there are no more moves left to be searched     |
            //    |===================================================|
            default:
            {
                return NO_MOVE;
            }
        }
    }
    return NO_MOVE;
}


int MovePicker::scoreQuietMove(Move move) {
    int fromSquare = getFromSquare(move);
    int toSquare = getToSquare(move);

    return historyMoves[fromSquare][toSquare];
}


int MovePicker::scoreCaptureAndPromoMove(Move move) {
    int flag = getMoveFlag(move);
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int score = 0;

    // MVV-LVA for capture moves
    /*
    if (flag & 4)
    {
        Piece victim = wp;
        if (flag == 5)
        {
            victim = wp;
        } else
        {
            victim = position.getPieceFromBoard(toSquare);
        }
        Piece attacker = position.getPieceFromBoard(fromSquare);

        score += 10 * averagePieceScore[victim] - averagePieceScore[attacker];
    }
    */

    // Sorting with SEE - remains to be tested

    if (flag & 4)
    {
        int attacker = position.getPieceFromBoard(fromSquare);

        // Get the captured piece
        // If the move is enpassant then the captured piece is a black pawn or a white pawn
        // Otherwise it is just the piece located at 'toSquare'
        Piece capturedPiece = flag == 5 ? attacker < 6 ? bp : wp : position.getPieceFromBoard(toSquare);

        // Get the SEE score of the move
        score += 100 * position.SEE(toSquare, capturedPiece, fromSquare, attacker);
        // Weak MVV-LVA to reinforce the score and prevent two moves from having the same score too often
        score += 10 * averagePieceScore[capturedPiece] - averagePieceScore[attacker];
    }



    if (flag & 8)
    {
        // Prioritize queen promotions
        if (flag == 11 || flag == 15)
        {
            score += 15000;
        } else
        {
            score -= 15000;
        }
    }

    return score;
}


void MovePicker::scoreQuietMoves()
{
    for (int i = currentMoveIndex; i < numOfMoves; i++)
    {
        moveScores[i] = scoreQuietMove(moves[i]);
    }
}

void MovePicker::scoreCapAndPromoMoves()
{
    for (int i = currentMoveIndex; i < numOfMoves; i++)
    {
        moveScores[i] = scoreCaptureAndPromoMove(moves[i]);
    }
}