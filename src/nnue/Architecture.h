//
// Created by theoa on 08/05/2026.
//
#pragma once

#include "../Types.h"

constexpr int InputSize = 768;
constexpr int HiddenSize = 256;

// Scale of quantization between input and the hidden layer
constexpr int Quantization0 = 255;

// Scale of quantization between the hidden layer and the output node
constexpr int Quantization1 = 64;

// The combined scale of quantization
// Used for de-quantization inside the NNUE evaluation function
constexpr int TotalQ = Quantization0 * Quantization1;

constexpr int Scale = 400;