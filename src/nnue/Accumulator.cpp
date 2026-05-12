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
            // Increment by 16 because we are adding 16 values with each iteration using SIMD instructions
            for (int i = 0; i < HiddenSize; i+=16)
            {
                // Store the weights into pointers so that they can be loaded
                int16_t* weights = &NNUE.featureWeights[featureIndex][i];
                int16_t* oppositeWeights = &NNUE.featureWeights[oppositeFeatureIndex][i];

                // Load the weights into 256 bit registers
                __m256i whiteRegister = _mm256_load_si256((__m256i*)weights);
                __m256i blackRegister = _mm256_load_si256((__m256i*)oppositeWeights);

                // Load the accumulator values into registers
                __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
                __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

                // Add the weights to the accumulator white and black values
                __m256i updatedWhiteAcc = _mm256_add_epi16(whiteAcc, whiteRegister);
                __m256i updatedBlackAcc = _mm256_add_epi16(blackAcc, blackRegister);

                // Store the new updated values into the white and black accumulator arrays
                _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
                _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
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
        for (int i = 0; i < HiddenSize; i+=16)
        {
            // Store the weights into pointers so that they can be loaded
            int16_t* weightsFrom = &NNUE.featureWeights[featureIndexFrom][i];
            int16_t* oppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureIndexFrom][i];
            int16_t* weightsTo = &NNUE.featureWeights[featureIndexTo][i];
            int16_t* oppositeWeightsTo = &NNUE.featureWeights[oppositeFeatureIndexTo][i];

            // Load the weights into 256 bit registers
            __m256i whiteRegisterFrom = _mm256_load_si256((__m256i*)weightsFrom);
            __m256i blackRegisterFrom = _mm256_load_si256((__m256i*)oppositeWeightsFrom);
            __m256i whiteRegisterTo = _mm256_load_si256((__m256i*)weightsTo);
            __m256i blackRegisterTo = _mm256_load_si256((__m256i*)oppositeWeightsTo);

            // Load the accumulator values into registers
            __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
            __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

            // Add the weights to the accumulator white and black values
            __m256i updatedWhiteAcc = _mm256_sub_epi16(whiteAcc, whiteRegisterFrom);
            updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, whiteRegisterTo);

            __m256i updatedBlackAcc = _mm256_sub_epi16(blackAcc, blackRegisterFrom);
            updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, blackRegisterTo);

            // Store the new updated values into the white and black accumulator arrays
            _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
            _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
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

    // If capture is also a promotion
    if constexpr (isPromotion)
    {
        int oppositePromotionPieceIndex = promotionPieceIndex > 5 ? promotionPieceIndex - 6 : promotionPieceIndex + 6;


        // Promoted piece feature index
        int featureIndexPromo = promotionPieceIndex * 64 + toSq;

        // Opposite feature index of promoted piece feature index
        int oppositeFeatureIndexPromo = oppositePromotionPieceIndex * 64 + oppositeToSq;


        for (int i = 0; i < HiddenSize; i+=16)
        {
            // Store the weights into pointers so that they can be loaded
            int16_t* weightsFrom = &NNUE.featureWeights[featureIndexFrom][i];
            int16_t* oppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureIndexFrom][i];
            int16_t* weightsPromo = &NNUE.featureWeights[featureIndexPromo][i];
            int16_t* oppositeWeightsPromo = &NNUE.featureWeights[oppositeFeatureIndexPromo][i];
            int16_t* weightsCap = &NNUE.featureWeights[featureIndexCap][i];
            int16_t* oppositeWeightsCap = &NNUE.featureWeights[oppositeFeatureIndexCap][i];

            // Load the weights into 256 bit registers
            __m256i whiteRegisterFrom = _mm256_load_si256((__m256i*)weightsFrom);
            __m256i blackRegisterFrom = _mm256_load_si256((__m256i*)oppositeWeightsFrom);
            __m256i whiteRegisterPromo = _mm256_load_si256((__m256i*)weightsPromo);
            __m256i blackRegisterPromo = _mm256_load_si256((__m256i*)oppositeWeightsPromo);
            __m256i whiteRegisterCap = _mm256_load_si256((__m256i*)weightsCap);
            __m256i blackRegisterCap = _mm256_load_si256((__m256i*)oppositeWeightsCap);


            // Load the accumulator values into registers
            __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
            __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

            // Add the weights to the accumulator white and black values
            __m256i updatedWhiteAcc = _mm256_sub_epi16(whiteAcc, whiteRegisterFrom);
            updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, whiteRegisterPromo);
            updatedWhiteAcc = _mm256_sub_epi16(updatedWhiteAcc, whiteRegisterCap);

            __m256i updatedBlackAcc = _mm256_sub_epi16(blackAcc, blackRegisterFrom);
            updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, blackRegisterPromo);
            updatedBlackAcc = _mm256_sub_epi16(updatedBlackAcc, blackRegisterCap);

            // Store the new updated values into the white and black accumulator arrays
            _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
            _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
        }
    }
    else // Non-promotion capture
    {
        // Feature index of moving piece to square 'toSq'
        int featureIndexTo = movingPieceIndex * 64 + toSq;

        // Opposite feature index of moving piece feature index
        int oppositeFeatureIndexTo = oppositePieceIndex * 64 + oppositeToSq;


        for (int i = 0; i < HiddenSize; i+=16)
        {

            // Store the weights into pointers so that they can be loaded
            int16_t* weightsFrom = &NNUE.featureWeights[featureIndexFrom][i];
            int16_t* oppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureIndexFrom][i];
            int16_t* weightsTo = &NNUE.featureWeights[featureIndexTo][i];
            int16_t* oppositeWeightsTo = &NNUE.featureWeights[oppositeFeatureIndexTo][i];
            int16_t* weightsCap = &NNUE.featureWeights[featureIndexCap][i];
            int16_t* oppositeWeightsCap = &NNUE.featureWeights[oppositeFeatureIndexCap][i];

            // Load the weights into 256 bit registers
            __m256i whiteRegisterFrom = _mm256_load_si256((__m256i*)weightsFrom);
            __m256i blackRegisterFrom = _mm256_load_si256((__m256i*)oppositeWeightsFrom);
            __m256i whiteRegisterTo = _mm256_load_si256((__m256i*)weightsTo);
            __m256i blackRegisterTo = _mm256_load_si256((__m256i*)oppositeWeightsTo);
            __m256i whiteRegisterCap = _mm256_load_si256((__m256i*)weightsCap);
            __m256i blackRegisterCap = _mm256_load_si256((__m256i*)oppositeWeightsCap);


            // Load the accumulator values into registers
            __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
            __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

            // Add the weights to the accumulator white and black values
            __m256i updatedWhiteAcc = _mm256_sub_epi16(whiteAcc, whiteRegisterFrom);
            updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, whiteRegisterTo);
            updatedWhiteAcc = _mm256_sub_epi16(updatedWhiteAcc, whiteRegisterCap);

            __m256i updatedBlackAcc = _mm256_sub_epi16(blackAcc, blackRegisterFrom);
            updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, blackRegisterTo);
            updatedBlackAcc = _mm256_sub_epi16(updatedBlackAcc, blackRegisterCap);

            // Store the new updated values into the white and black accumulator arrays
            _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
            _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
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


    for (int i = 0; i < HiddenSize; i+=16)
    {
        // Store the weights into pointers so that they can be loaded
        int16_t* weightsFrom = &NNUE.featureWeights[featureIndexFrom][i];
        int16_t* oppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureIndexFrom][i];
        int16_t* weightsPromo = &NNUE.featureWeights[featureIndexPromo][i];
        int16_t* oppositeWeightsPromo = &NNUE.featureWeights[oppositeFeatureIndexPromo][i];

        // Load the weights into 256 bit registers
        __m256i whiteRegisterFrom = _mm256_load_si256((__m256i*)weightsFrom);
        __m256i blackRegisterFrom = _mm256_load_si256((__m256i*)oppositeWeightsFrom);
        __m256i whiteRegisterPromo = _mm256_load_si256((__m256i*)weightsPromo);
        __m256i blackRegisterPromo = _mm256_load_si256((__m256i*)oppositeWeightsPromo);


        // Load the accumulator values into registers
        __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
        __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

        // Add the weights to the accumulator white and black values
        __m256i updatedWhiteAcc = _mm256_sub_epi16(whiteAcc, whiteRegisterFrom);
        updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, whiteRegisterPromo);

        __m256i updatedBlackAcc = _mm256_sub_epi16(blackAcc, blackRegisterFrom);
        updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, blackRegisterPromo);

        // Store the new updated values into the white and black accumulator arrays
        _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
        _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
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
    for (int i = 0; i < HiddenSize; i+=16)
    {
        // Store the weights into pointers so that they can be loaded
        // King weights
        int16_t* KingWeightsFrom = &NNUE.featureWeights[featureKingIndexFrom][i];
        int16_t* KingOppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureKingIndexFrom][i];
        int16_t* KingWeightsTo = &NNUE.featureWeights[featureKingIndexTo][i];
        int16_t* KingOppositeWeightsTo = &NNUE.featureWeights[oppositeFeatureKingIndexTo][i];

        // Rook weights
        int16_t* RookWeightsFrom = &NNUE.featureWeights[featureRookIndexFrom][i];
        int16_t* RookOppositeWeightsFrom = &NNUE.featureWeights[oppositeFeatureRookIndexFrom][i];
        int16_t* RookWeightsTo = &NNUE.featureWeights[featureRookIndexTo][i];
        int16_t* RookOppositeWeightsTo = &NNUE.featureWeights[oppositeFeatureRookIndexTo][i];

        // Load the weights into 256 bit registers
        // Load king weights into registers
        __m256i KingWhiteRegisterFrom = _mm256_load_si256((__m256i*)KingWeightsFrom);
        __m256i KingBlackRegisterFrom = _mm256_load_si256((__m256i*)KingOppositeWeightsFrom);
        __m256i KingWhiteRegisterTo = _mm256_load_si256((__m256i*)KingWeightsTo);
        __m256i KingBlackRegisterTo = _mm256_load_si256((__m256i*)KingOppositeWeightsTo);

        // Load rook weights into registers
        __m256i RookWhiteRegisterFrom = _mm256_load_si256((__m256i*)RookWeightsFrom);
        __m256i RookBlackRegisterFrom = _mm256_load_si256((__m256i*)RookOppositeWeightsFrom);
        __m256i RookWhiteRegisterTo = _mm256_load_si256((__m256i*)RookWeightsTo);
        __m256i RookBlackRegisterTo = _mm256_load_si256((__m256i*)RookOppositeWeightsTo);


        // Load the accumulator values into registers
        __m256i whiteAcc = _mm256_load_si256((__m256i*)&white[i]);
        __m256i blackAcc = _mm256_load_si256((__m256i*)&black[i]);

        // Add the weights to the accumulator white and black values
        __m256i updatedWhiteAcc = _mm256_sub_epi16(whiteAcc, KingWhiteRegisterFrom);
        updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, KingWhiteRegisterTo);
        updatedWhiteAcc = _mm256_sub_epi16(updatedWhiteAcc, RookWhiteRegisterFrom);
        updatedWhiteAcc = _mm256_add_epi16(updatedWhiteAcc, RookWhiteRegisterTo);

        __m256i updatedBlackAcc = _mm256_sub_epi16(blackAcc, KingBlackRegisterFrom);
        updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, KingBlackRegisterTo);
        updatedBlackAcc = _mm256_sub_epi16(updatedBlackAcc, RookBlackRegisterFrom);
        updatedBlackAcc = _mm256_add_epi16(updatedBlackAcc, RookBlackRegisterTo);

        // Store the new updated values into the white and black accumulator arrays
        _mm256_store_si256((__m256i*)&white[i], updatedWhiteAcc);
        _mm256_store_si256((__m256i*)&black[i], updatedBlackAcc);
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