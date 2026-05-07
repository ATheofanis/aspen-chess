//
// Created by theoa on 03/05/2026.
//

#pragma once
#include <string>

constexpr int dataGenMaxNodes = 5000;
inline int NumberOfGames = 20; // 1500000
constexpr int maxNumberOfMoves = 256;
constexpr int NumOfRandomOpeningMoves = 8;
constexpr int evalThreshold = 2000;


struct DataGenEntry
{
    int evaluation{};
    float wdl{};
    bool valid = false;
    std::string fenStr{};

    void setEvalAndFen(int eval, std::string fen)
    {
        evaluation = eval;
        fenStr = fen;
    }

    void setWdl(float Wdl)
    {
        wdl = Wdl;
    }
};

void generateData();
