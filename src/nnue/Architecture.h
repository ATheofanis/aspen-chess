//
// Created by theoa on 08/05/2026.
//
#pragma once

#include "../Types.h"

constexpr int InputSize = 768;
constexpr int HiddenSize = 256;

// Scale of quantization between input and the hidden layer
constexpr int QuantizationA = 255;

// Scale of quantization between the hidden layer and the output node
constexpr int QuantizationB = 64;

// The combined scale of quantization
// Used for de-quantization inside the NNUE evaluation function
constexpr int TotalQ = QuantizationA * QuantizationB;

constexpr int Scale = 400;