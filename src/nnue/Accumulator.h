//
// Created by theoa on 08/05/2026.
//

#pragma once

#include "Nnue.h"
#include "../Position.h"
#include "Architecture.h"
#include <immintrin.h>

class Accumulator
{
public:
    // Align the starting memory to a multiple of 32 bytes, which is required for fast aligned AVX2 SIMD instructions
    alignas(32) int16_t white[HiddenSize]{};
    alignas(32) int16_t black[HiddenSize]{};

    Accumulator() = default;

    Accumulator(const Position& pos)
    {
        initializeAccumulator(pos);
    }

    // Initialize or reset the accumulator
    void initializeAccumulator(const Position& pos);

    // Used when making or unmaking moves to update the accumulator
    void movePiece(int fromSq, int toSq, int pieceIndex);

    template<bool isPromotionCaptureMove>
    void makeCapture(int fromSq, int toSq, int movingPieceIndex, int capSq, int capPieceIndex, int promotionPieceIndex = 12);

    void makePromotion(int fromSq, int toSq, int movingPieceIndex, int promotionPieceIndex);

    void makeCastle(int kingFromSq, int kingToSq, int kingPieceIndex, int rookFromSq, int rookToSq, int rookPieceIndex);

    void makeMove(Move move, const Position& pos);
};
