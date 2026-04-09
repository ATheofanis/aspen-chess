//
// Created by theoa on 07/04/2026.
//

#pragma once
#include "Position.h"
#include <vector>

inline int totalDoubledPawns = 0;
inline int totalIsolatedPawns = 0;

struct TuningEntry
{
    //int doubledPawnsCount;
    //int isolatedPawnsCount;
    //int passedPawnsCount;
    //int semiOpenRooksCount;
    int bishopPairsCount;
    double result;
    int staticEval;

    TuningEntry() = default;
};

inline std::vector<TuningEntry> tuningData;


void parseLineForTuning(const std::string& line, Position& pos);

int numOfDoubledPawns(Color ofSide, const Position& pos);
int numOfIsolatedPawns(Color ofSide, const Position& pos);
int getPassedPawnDiff(const Position& pos, int rank);
int getBishopPairDiff(const Position& pos);

int getSemiOpenFileRooksDiff(const Position& pos);


void tuner();