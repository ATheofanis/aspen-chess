//
// Created by theoa on 07/05/2026.
//

#include "Nnue.h"
#include <fstream>
#include <iostream>
#include "../../3rdparty/incbin.h"

INCBIN(NetworkWeights, "NetworkFiles/aspen-net562-8.bin");

NetworkStruct NNUE;

void loadQuantised()
{
    std::memcpy(&NNUE, gNetworkWeightsData, sizeof(NetworkStruct));
}

// NNUE evaluation using Lizard SCReLU for faster calculations
int evaluateNNUE(const Accumulator& acc, Color sideToMove)
{
    const __m256i vec_zero = _mm256_setzero_si256();
    const __m256i vec_qa = _mm256_set1_epi16(QuantizationA);
    __m256i sum = vec_zero;

    const int16_t* stmAcc = (sideToMove == White) ? acc.white : acc.black;
    const int16_t* nstmAcc = (sideToMove == White) ? acc.black : acc.white;

    // Lizard SCReLU by Liam McGuire (https://www.chessprogramming.org/NNUE#Lizard_SCReLU)
    for (int i = 0; i < HiddenSize; i += 16)
    {
        const __m256i us = _mm256_load_si256((__m256i*)&stmAcc[i]); // Load from accumulator
        const __m256i them = _mm256_load_si256((__m256i*)&nstmAcc[i]);

        const __m256i us_weights = _mm256_load_si256((__m256i*)&NNUE.outputWeights[i]); // Load from net
        const __m256i them_weights = _mm256_load_si256((__m256i*)&NNUE.outputWeights[HiddenSize + i]);

        const __m256i us_clamped   = _mm256_min_epi16( _mm256_max_epi16(   us, vec_zero ), vec_qa );
        const __m256i them_clamped = _mm256_min_epi16( _mm256_max_epi16( them, vec_zero ), vec_qa );

        const __m256i us_results   = _mm256_madd_epi16( _mm256_mullo_epi16(   us_weights,   us_clamped ),us_clamped );
        const __m256i them_results = _mm256_madd_epi16( _mm256_mullo_epi16( them_weights, them_clamped ),them_clamped );

        sum = _mm256_add_epi32(sum,us_results);
        sum = _mm256_add_epi32(sum,them_results);
    }


    // Fold top and bottom halves of the 256 bit sum together
    __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum),_mm256_extracti128_si256(sum, 1));

    // Shuffle the sum and add it to itself to get the vector with values: v3+v0, v2+v1, v1+v2, v0+v3
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(0, 1, 2, 3)));

    // We do a final shuffle and addition like earlier to get the result: v0+v1+v2+v3 in all 4 values of the vector
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2)));

    // Extract only the first value of the vector which contains the total sum
    int totalSum = _mm_cvtsi128_si32(sum128);

    // Dequantize first before adding the output bias so we dont lose precision
    int score = totalSum / QuantizationA + NNUE.outputBias;

    // For the final step cast score to 64 bits so it does not overflow from scaling
    // Also de-quantize the total quantization factor
    return (static_cast<int64_t>(score) * Scale) / (TotalQ);
}



