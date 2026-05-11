//
// Created by theoa on 07/05/2026.
//

#include "Nnue.h"
#include <fstream>
#include <iostream>

NetworkStruct NNUE;

void loadQuantised(const std::string& filePath)
{
    std::ifstream file{filePath, std::ios::binary};

    if (!file.is_open()) {
        std::cerr << "ERROR! Provided file path is wrong: " << filePath << std::endl;
        return;
    }

    file.read(reinterpret_cast<char *>(&NNUE), sizeof(NetworkStruct));
}


int evaluateNNUE(const Accumulator& acc, Color sideToMove)
{
    int score = NNUE.outputBias;

    const int16_t* stmAcc = (sideToMove == White) ? acc.white : acc.black;
    const int16_t* nstmAcc = (sideToMove == White) ? acc.black : acc.white;

    for (int i = 0; i < HiddenSize; i++)
    {
        int stmActivation = SCReLU(stmAcc[i]);
        int nstmActivation = SCReLU(nstmAcc[i]);

        score += stmActivation * NNUE.outputWeights[i];
        score += nstmActivation * NNUE.outputWeights[HiddenSize + i];
    }

    return (static_cast<int64_t>(score) * Scale) / (TotalQ);
}



