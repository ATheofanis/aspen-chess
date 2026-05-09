//
// Created by theoa on 08/05/2026.
//

#include "Accumulator.h"

// Used to initialize or reset the accumulator
void Accumulator::initializeAccumulator(const Position& pos)
{
    // Set the value of the hidden neurons to their respective bias from the neural network
    for (int h = 0; h < HiddenSize; h++)
    {
        white[h] = NNUE.featureBiases[h];
        black[h] = NNUE.featureBiases[h];
    }

    for (int sq = 0; sq < 64; sq++)
    {
        Piece p = pos.getPieceFromBoard(sq);
        if (p != NO_PIECE)
        {
            int pieceIndex = (int)p;

            // XOR with 56 to flip the square for black's side
            int oppositeSq = sq ^ 56;

            // Swap the piece color
            int oppositePieceIndex = pieceIndex > 5 ? pieceIndex - 6 : pieceIndex + 6;

            // Get the index of the piece based on its index and square
            int featureIndex = pieceIndex * 64 + sq;

            int oppositeFeatureIndex = oppositePieceIndex * 64 + oppositeSq;

            // Add the weights of this piece on this specific square - opposite for black
            for (int i = 0; i < HiddenSize; i++)
            {
                white[i] += NNUE.featureWeights[featureIndex][i];
                black[i] += NNUE.featureWeights[oppositeFeatureIndex][i];
            }
        }
    }
}


void Accumulator::movePiece(int fromSq, int toSq, int pieceIndex)
{
    if (pieceIndex != 12)
    {
        int oppositeFromSq = fromSq ^ 56;
        int oppositeToSq = toSq ^ 56;

        // Swap the piece color
        int oppositePieceIndex = pieceIndex > 5 ? pieceIndex - 6 : pieceIndex + 6;

        // Get the index of the piece based on its index and square
        int featureIndexFrom = pieceIndex * 64 + fromSq;
        int featureIndexTo = pieceIndex * 64 + toSq;

        int oppositeFeatureIndexFrom = oppositePieceIndex * 64 + oppositeFromSq;
        int oppositeFeatureIndexTo = oppositePieceIndex * 64 + oppositeToSq;


        // Add the to square weights and remove the from square weights
        for (int i = 0; i < HiddenSize; i++)
        {
            white[i] -= NNUE.featureWeights[featureIndexFrom][i];
            white[i] += NNUE.featureWeights[featureIndexTo][i];

            black[i] -= NNUE.featureWeights[oppositeFeatureIndexFrom][i];
            black[i] += NNUE.featureWeights[oppositeFeatureIndexTo][i];
        }
    }
}


void Accumulator::removePiece(int fromSq, int pieceIndex)
{
    if (pieceIndex != 12)
    {
        int oppositeFromSq = fromSq ^ 56;

        // Swap the piece color
        int oppositePieceIndex = pieceIndex > 5 ? pieceIndex - 6 : pieceIndex + 6;

        // Get the index of the piece based on its index and square
        int featureIndexFrom = pieceIndex * 64 + fromSq;

        int oppositeFeatureIndexFrom = oppositePieceIndex * 64 + oppositeFromSq;

        // Subtract the feature weights of the piece on that specific square to remove the piece from the accumulator
        for (int i = 0; i < HiddenSize; i++)
        {
            white[i] -= NNUE.featureWeights[featureIndexFrom][i];
            black[i] -= NNUE.featureWeights[oppositeFeatureIndexFrom][i];
        }
    }
}


void Accumulator::addPiece(int toSq, int pieceIndex)
{
    if (pieceIndex != 12)
    {
        int oppositeToSq = toSq ^ 56;

        // Swap the piece color
        int oppositePieceIndex = pieceIndex > 5 ? pieceIndex - 6 : pieceIndex + 6;

        // Get the index of the piece based on its index and square
        int featureIndexTo = pieceIndex * 64 + toSq;

        int oppositeFeatureIndexTo = oppositePieceIndex * 64 + oppositeToSq;

        // Add the feature weights of the piece that is added to the board
        for (int i = 0; i < HiddenSize; i++)
        {
            white[i] += NNUE.featureWeights[featureIndexTo][i];
            black[i] += NNUE.featureWeights[oppositeFeatureIndexTo][i];
        }
    }
}

// Function to update the accumulator when a move is made
void Accumulator::makeMove(Move move, int movingPieceIndex, int capturedPieceSquare, int capturedPieceIndex)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;

    // Remove the piece if the move was a capture
    if (capturedPieceIndex != 12)
    {
        removePiece(capturedPieceSquare, capturedPieceIndex);
    }

    // Handle promotions
    if (flag & 8)
    {
        int colorDelta = (movingPieceIndex < 6) ? 0 : 6;

        int promotedPieceIndex = colorDelta + flag % 4 + 1;

        removePiece(fromSquare, movingPieceIndex);
        addPiece(toSquare, promotedPieceIndex);
    } else
    {
        movePiece(fromSquare, toSquare, movingPieceIndex);
    }

    // Also move the rook if the move was queenside or kingside castles
    if (flag == 2)
    {

    }
}


Accumulator accumulator;