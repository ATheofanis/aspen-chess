//
// Created by theoa on 16/05/2026.
//

#include "MovePicker.h"

#include "LegalMoveGen.h"
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
                phase = MovePickerPhase::GenLegalMoves;

                if ((skipQuietMoves) && isQuiet(getMoveFlag(hashMove))) break;

                if (hashMove != NO_MOVE && position.moveIsValid(hashMove) && position.moveIsLegal(legalityInfo, hashMove))
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
            case MovePickerPhase::GenLegalMoves:
            {
                // Move on to the next phase
                phase = MovePickerPhase::PlayLegalMoves;

                // Generate captures and promotions
                generateLegalMoves(legalityInfo, position, moves, numOfMoves);

                // Score the moves we just generated
                scoreMoves();

                break;
            }
            //    |=================================================================================|
            //    |  3. Winning Captures And Promotions :  After the generation phase is over,      |
            //    |   the move picker will loop through all the generated moves and pick the one    |
            //    |             with the highest score, which was calculated at phase 2             |
            //    |=================================================================================|
            case MovePickerPhase::PlayLegalMoves:
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

                    Move bestMove = moves[bestMoveIndex];

                    // Swap the best move to the front of the moves list so that we dont loop over it on the next iteration
                    // Same with the move score and the move scores list
                    std::swap(moveScores[bestMoveIndex], moveScores[currentMoveIndex]);
                    std::swap(moves[bestMoveIndex], moves[currentMoveIndex++]);


                    if (bestMove == hashMove) continue;

                    return bestMove;
                }
                phase = MovePickerPhase::End;

                break;
            }
            default:
            {
                return NO_MOVE;
            }
        }
    }
    return NO_MOVE;
}


// Function used to order moves from best to worst
int MovePicker::scoreMove(Move move)
{
    int flag = getMoveFlag(move);

    if (flag == 11) return QueenPromotion; // Reward queen promotion
    if (flag == 15) return QueenPromoCapture; // Reward capture that leads to queen promotion

    // Punish under-promotions
    if (flag & 8) return UnderPromotions;

    // Source and destination squares for MVV-LVA for captures, or history for quiet moves
    int fromSquare = getFromSquare(move);
    int toSquare = getToSquare(move);

    Piece movingPiece = position.getPieceFromBoard(fromSquare);

    // MVV-LVA for capture moves
    if (isCapture(getMoveFlag(move)))
    {
        Piece victim;
        if (flag == 5)
        {
            victim = position.isWhiteToMove() ? bp : wp;
        }
        else
        {
            victim = position.getPieceFromBoard(toSquare);
        }

        return GoodCapturesBase + 10 * averagePieceScore[victim] - averagePieceScore[movingPiece];
    }

    // Killer moves
    if (move == firstKillerMove) return KillerMoveScore0;
    if (move == secondKillerMove) return KillerMoveScore1;

    return (*historyScores)[position.isWhiteToMove() ? White : Black][fromSquare][toSquare];
}




void MovePicker::scoreMoves()
{
    for (int i = currentMoveIndex; i < numOfMoves; i++)
    {
        moveScores[i] = scoreMove(moves[i]);
    }
}

void MovePicker::scoreCapAndPromoMoves()
{
    for (int i = currentMoveIndex; i < numOfMoves; i++)
    {
        moveScores[i] = scoreMove(moves[i]);
    }
}
