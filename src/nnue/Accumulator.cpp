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

// This function handles all capture moves, updating the accumulator accordingly
template<bool promoCaptureFlag>
void Accumulator::makeCapture(int fromSq, int toSq, int movingPieceIndex, int capSq, int capPieceIndex, int promotionPieceIndex)
{
    constexpr bool isPromotion = (promoCaptureFlag == true);

    int oppositeFromSq = fromSq ^ 56;
    int oppositeToSq = toSq ^ 56;
    int oppositeCapSq = capSq ^ 56;

    int oppositePieceIndex = movingPieceIndex > 5 ? movingPieceIndex - 6 : movingPieceIndex + 6;
    int oppositeCapPieceIndex = capPieceIndex > 5 ? capPieceIndex - 6 : capPieceIndex + 6;

    // Moving piece feature index
    int featureIndexFrom = movingPieceIndex * 64 + fromSq;

    // Captured piece feature index
    int featureIndexCap = capPieceIndex * 64 + capSq;

    int oppositeFeatureIndexFrom = oppositePieceIndex * 64 + oppositeFromSq;
    int oppositeFeatureIndexCap = oppositeCapPieceIndex * 64 + oppositeCapSq;


    if constexpr (isPromotion)
    {
        int oppositePromotionPieceIndex = promotionPieceIndex > 5 ? promotionPieceIndex - 6 : promotionPieceIndex + 6;


        // Promoted piece feature index
        int featureIndexPromo = promotionPieceIndex * 64 + toSq;

        // Opposite feature index of promoted piece feature index
        int oppositeFeatureIndexPromo = oppositePromotionPieceIndex * 64 + oppositeToSq;


        for (int i = 0; i < HiddenSize; i++)
        {
            // Remove moving piece from 'fromSq'
            white[i] -= NNUE.featureWeights[featureIndexFrom][i];
            black[i] -= NNUE.featureWeights[oppositeFeatureIndexFrom][i];

            // Add promoted piece to 'toSq'
            white[i] += NNUE.featureWeights[featureIndexPromo][i];
            black[i] += NNUE.featureWeights[oppositeFeatureIndexPromo][i];

            // Remove captured piece from 'capSq'
            white[i] -= NNUE.featureWeights[featureIndexCap][i];
            black[i] -= NNUE.featureWeights[oppositeFeatureIndexCap][i];
        }
    }
    else
    {
        // Feature index of moving piece to square 'toSq'
        int featureIndexTo = movingPieceIndex * 64 + toSq;

        // Opposite feature index of promoted piece feature index
        int oppositeFeatureIndexTo = oppositePieceIndex * 64 + oppositeToSq;


        for (int i = 0; i < HiddenSize; i++)
        {
            // Remove moving piece from 'fromSq'
            white[i] -= NNUE.featureWeights[featureIndexFrom][i];
            black[i] -= NNUE.featureWeights[oppositeFeatureIndexFrom][i];

            // Add moving piece to 'toSq'
            white[i] += NNUE.featureWeights[featureIndexTo][i];
            black[i] += NNUE.featureWeights[oppositeFeatureIndexTo][i];

            // Remove captured piece from 'capSq'
            white[i] -= NNUE.featureWeights[featureIndexCap][i];
            black[i] -= NNUE.featureWeights[oppositeFeatureIndexCap][i];
        }
    }
}


// This function handles normal promotions for the accumulator
void Accumulator::makePromotion(int fromSq, int toSq, int movingPieceIndex, int promotionPieceIndex)
{
    int oppositeFromSq = fromSq ^ 56;
    int oppositeToSq = toSq ^ 56;

    int oppositePieceIndex = movingPieceIndex > 5 ? movingPieceIndex - 6 : movingPieceIndex + 6;
    int oppositePromotionPieceIndex = promotionPieceIndex > 5 ? promotionPieceIndex - 6 : promotionPieceIndex + 6;

    // Moving piece feature index
    int featureIndexFrom = movingPieceIndex * 64 + fromSq;
    // Promoting piece feature index
    int featureIndexPromo = promotionPieceIndex * 64 + toSq;

    // Opposite feature index
    int oppositeFeatureIndexFrom = oppositePieceIndex * 64 + oppositeFromSq;
    int oppositeFeatureIndexPromo = oppositePromotionPieceIndex * 64 + oppositeToSq;


    for (int i = 0; i < HiddenSize; i++)
    {
        // Remove moving piece from 'fromSq'
        white[i] -= NNUE.featureWeights[featureIndexFrom][i];
        black[i] -= NNUE.featureWeights[oppositeFeatureIndexFrom][i];

        // Add promoted piece to 'toSq'
        white[i] += NNUE.featureWeights[featureIndexPromo][i];
        black[i] += NNUE.featureWeights[oppositeFeatureIndexPromo][i];
    }
}


void Accumulator::makeCastle(int kingFromSq, int kingToSq, int kingPieceIndex, int rookFromSq, int rookToSq, int rookPieceIndex)
{
    int oppositeKingFromSq = kingFromSq ^ 56;
    int oppositeKingToSq = kingToSq ^ 56;
    int oppositeRookFromSq = rookFromSq ^ 56;
    int oppositeRookToSq = rookToSq ^ 56;

    // Swap the piece index of rook and king
    int oppositeKingPieceIndex = kingPieceIndex > 5 ? kingPieceIndex - 6 : kingPieceIndex + 6;
    int oppositeRookPieceIndex = rookPieceIndex > 5 ? rookPieceIndex - 6 : rookPieceIndex + 6;

    // Get the feature index of the piece based on its index and square
    int featureKingIndexFrom = kingPieceIndex * 64 + kingFromSq;
    int featureKingIndexTo = kingPieceIndex * 64 + kingToSq;
    int featureRookIndexFrom = rookPieceIndex * 64 + rookFromSq;
    int featureRookIndexTo = rookPieceIndex * 64 + rookToSq;

    // Get the opposite feature indices
    int oppositeFeatureKingIndexFrom = oppositeKingPieceIndex * 64 + oppositeKingFromSq;
    int oppositeFeatureKingIndexTo = oppositeKingPieceIndex * 64 + oppositeKingToSq;
    int oppositeFeatureRookIndexFrom = oppositeRookPieceIndex * 64 + oppositeRookFromSq;
    int oppositeFeatureRookIndexTo = oppositeRookPieceIndex * 64 + oppositeRookToSq;


    // Update the position of the king and the rook by removing the weights of their previous squares and adding the weights of their respective destination squares
    for (int i = 0; i < HiddenSize; i++)
    {
        // King
        white[i] -= NNUE.featureWeights[featureKingIndexFrom][i];
        white[i] += NNUE.featureWeights[featureKingIndexTo][i];

        black[i] -= NNUE.featureWeights[oppositeFeatureKingIndexFrom][i];
        black[i] += NNUE.featureWeights[oppositeFeatureKingIndexTo][i];

        // Rook
        white[i] -= NNUE.featureWeights[featureRookIndexFrom][i];
        white[i] += NNUE.featureWeights[featureRookIndexTo][i];

        black[i] -= NNUE.featureWeights[oppositeFeatureRookIndexFrom][i];
        black[i] += NNUE.featureWeights[oppositeFeatureRookIndexTo][i];
    }
}


// Function to update the accumulator when a move is made
void Accumulator::makeMove(Move move, const Position& pos)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0xF;

    int movingPieceIndex = pos.getPieceFromBoard(fromSquare);

    bool whiteToMove = movingPieceIndex < 6;

    const int colorDelta = whiteToMove ? 0 : 6;

    if (flag & 4)
    {
        if (flag == 5)
        {
            if (whiteToMove) // white played en-passant
            {
                int epCapSq = toSquare - 8;
                makeCapture<false>(fromSquare, toSquare, wp, epCapSq, bp);
            }
            else // black played en-passant
            {
                int epCapSq = toSquare + 8;
                makeCapture<false>(fromSquare, toSquare, bp, epCapSq, wp);
            }
        }
        else
        {
            // Promo-capture
            if (flag & 8)
            {
                int promotedPieceIndex = colorDelta + flag % 4 + 1;

                makeCapture<true>(fromSquare, toSquare, colorDelta, toSquare, pos.getPieceFromBoard(toSquare), promotedPieceIndex);
            }
            else // Normal captures
            {
                makeCapture<false>(fromSquare, toSquare, movingPieceIndex, toSquare, pos.getPieceFromBoard(toSquare));
            }
        }
    }
    else
    {
        if (flag & 8) // Non-capture promotions
        {
            int promotedPieceIndex = colorDelta + flag % 4 + 1;

            makePromotion(fromSquare, toSquare, movingPieceIndex, promotedPieceIndex);
        }
        else // Castle moves
        {
            // Also move the rook if the move was queenside or kingside castles
            if (flag == 2)
            {
                if (whiteToMove) // h1 to f1 (7 - 5)
                {
                    makeCastle(fromSquare, toSquare, wK, 7, 5, wR);
                }
                else // h8 to f8 (63 - 61)
                {
                    makeCastle(fromSquare, toSquare, bK, 63, 61, bR);
                }
            }
            else if (flag == 3)
            {
                if (whiteToMove) // a1 to d1 (0 - 3)
                {
                    makeCastle(fromSquare, toSquare, wK, 0, 3, wR);
                }
                else // a8 to d8 (56 - 59)
                {
                    makeCastle(fromSquare, toSquare, bK, 56, 59, bR);
                }
            }
            else // Normal moves - quiet and 2 square advances
            {
                movePiece(fromSquare, toSquare, movingPieceIndex);
            }
        }
    }
}