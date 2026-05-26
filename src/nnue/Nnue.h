//
// Created by theoa on 07/05/2026.
//

#pragma once

#include <cstdint>
#include <string>
#include "Accumulator.h"
#include "Architecture.h"


constexpr int OutputBuckets = 8;
constexpr int Divisor = (32 + OutputBuckets - 1) / OutputBuckets;

class Accumulator;

struct NetworkStruct
{
    int16_t featureWeights[InputSize][HiddenSize];
    int16_t featureBiases[HiddenSize];
    int16_t outputWeights[OutputBuckets][HiddenSize * 2];
    int16_t outputBias[OutputBuckets];
};

extern NetworkStruct NNUE;


inline int chooseOutputBuckets(Bitboard occupancy)
{
    return (__builtin_popcountll(occupancy) - 2) / Divisor;
}


// Function to load the quantisised binary into the NNUE struct
void loadQuantised();

// NNUE evaluation function (Lizard SCReLU)
int evaluateNNUE(const Accumulator& acc, Color sideToMove, int bucket);
