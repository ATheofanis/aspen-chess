//
// Created by theoa on 07/05/2026.
//

#pragma once

#include <cstdint>
#include <string>
#include "Accumulator.h"
#include "Architecture.h"

class Accumulator;

struct NetworkStruct
{
    int16_t featureWeights[InputSize][HiddenSize];
    int16_t featureBiases[HiddenSize];
    int16_t outputWeights[HiddenSize * 2];
    int16_t outputBias;
};

extern NetworkStruct NNUE;


// Function to load the quantisised binary into the NNUE struct
void loadQuantised(const std::string& filePath);


int evaluateNNUE(const Accumulator& acc, Color sideToMove);

// SCReLU - (Squared Clipped ReLU) Activation Function
inline int SCReLU(int value)
{
    int CReLU = std::max(0, std::min(value, QuantizationA));
    return (CReLU * CReLU) / QuantizationA;
}
