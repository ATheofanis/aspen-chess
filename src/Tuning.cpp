//
// Created by theoa on 07/04/2026.
//

#include "Tuning.h"
#include <cmath>


double calculateError(int bonus, double K)
{
    double totalError = 0;

    // for every entry calculate the total score of the position using previously stored static eval as the base
    // then calculate the sigmoid which returns the win probability
    // the total error is then calculated by adding up all the squares of the diff's of 'result - sigmoid'
    // and for the last step divide the total error sum with the number of entries to return the total error
    for (const auto& tuningEntry : tuningData)
    {
        double score = (double)tuningEntry.staticEval + (double)tuningEntry.bishopPairsCount * (double)bonus;

        double sigmoid = 1.0 / (1.0 + exp(-K * score/ 400.0));

        double error = tuningEntry.result - sigmoid;

        totalError += error * error;
    }

    return totalError / (double)tuningData.size();
}


// to read line by line from a file containing game information, like board position and which side won
void parseLineForTuning(const std::string& line, Position& pos)
{
    TuningEntry newEntry;
    pos.loadFen(line);

    double result = 0.5;
    if (line.find("\"1-0\"") != std::string::npos)
    {
        result = 1.0;
    } else if (line.find("\"0-1\"") != std::string::npos)
    {
        result = 0.0;
    }
    newEntry.result = result;
    //newEntry.staticEval = scoreBoardNNUE(pos);

    newEntry.bishopPairsCount = getBishopPairDiff(pos);

    // to test that the tuning data is parsed correctly
    if (tuningData.size() < 6)
    {
        std::cout << "Num of bishop pairs:" << newEntry.bishopPairsCount << std::endl;
    }


    tuningData.push_back(newEntry);
}







int numOfIsolatedPawns(Color ofSide, const Position& pos)
{
    int numOfIsolatedPawns = 0;
    Bitboard friendlyPawns, enemyPawns;

    if (ofSide == White)
    {
        friendlyPawns = pos.getPieceBitboard(wp);
        enemyPawns = pos.getPieceBitboard(bp);
    } else
    {
        friendlyPawns = pos.getPieceBitboard(bp);
        enemyPawns = pos.getPieceBitboard(wp);
    }

    Bitboard friendlyPCopy = friendlyPawns;


    while (friendlyPawns)
    {
        int currSq = popLsbAndReturnIndex(friendlyPawns);

        if ((isolatedMasks[currSq] & friendlyPCopy) == 0)
        {
            numOfIsolatedPawns++;
        }
    }


    return numOfIsolatedPawns;
}




int numOfDoubledPawns(Color ofSide, const Position& pos)
{
    int numOfDoubledPawns = 0;
    int colorDelta = (ofSide == White ? 0 : 6);

    Bitboard pawns = pos.getPieceBitboard(colorDelta);


    for (int i = 0; i < 8; i++)
    {
        Bitboard doubledPawns = pawns & files[i];

        int doubledPawnsInFileCount = bitCount(doubledPawns);

        if (doubledPawnsInFileCount > 1)
        {
            numOfDoubledPawns += doubledPawnsInFileCount - 1;
        }
    }


    return numOfDoubledPawns;
}



// returns the difference between the total sum of white passed and black passed pawns relative to eachother on a specific rank
int getPassedPawnDiff(const Position& pos, int rank)
{
    // rank is the rank where the pawns will be from white's perspective so black's passed pawns will be at 7 - rank
    Bitboard whiteInRank = ranks[rank] & pos.getPieceBitboard(wp);
    Bitboard blackInRank = ranks[7 - rank] & pos.getPieceBitboard(bp);

    int count = 0;

    while (whiteInRank) {
        int currSq = popLsbAndReturnIndex(whiteInRank);
        if (!(whitePassedMasks[currSq] & pos.getPieceBitboard(bp))) count++;
    }

    while (blackInRank) {
        int currSq = popLsbAndReturnIndex(blackInRank);
        if (!(blackPassedMasks[currSq] & pos.getPieceBitboard(wp))) count--;
    }

    return count;
}


// returns the difference between the white rooks on semi open files and the black rooks on semi open files
int getSemiOpenFileRooksDiff(const Position& pos)
{
    Bitboard wRs = pos.getPieceBitboard(wR);
    Bitboard bRs = pos.getPieceBitboard(bR);
    Bitboard wps = pos.getPieceBitboard(wp);
    Bitboard bps = pos.getPieceBitboard(bp);
    Bitboard pawns = bps | wps;

    int numOfRooksOnSemi = 0;


    while (wRs)
    {
        int currentRookSq = popLsbAndReturnIndex(wRs);
        Bitboard file = files[currentRookSq & 7];

        if ((file & pawns) == 0)
        {
            numOfRooksOnSemi++;
        }
    }


    while (bRs)
    {
        int currentRookSq = popLsbAndReturnIndex(bRs);
        Bitboard file = files[currentRookSq & 7];

        if ((file & pawns) == 0)
        {
            numOfRooksOnSemi--;
        }
    }
    return numOfRooksOnSemi;
}


// returns the difference of white bishop pair and black bishop pair
int getBishopPairDiff(const Position& pos)
{
    int bishopPairDiff = 0;
    if (bitCount(pos.getPieceBitboard(wB)) >= 2)
    {
        bishopPairDiff++;
    }
    if (bitCount(pos.getPieceBitboard(bB)) >= 2)
    {
        bishopPairDiff--;
    }
    return bishopPairDiff;
}



// tuner for bonuses
void tuner()
{
    double bestK = 1.0;
    int bestBonus = 0;
    double minError = 1.0;

    // run the tuner for a range of K values to find the K leading to the least amount of errors
    for (double K = 1.5; K <= 5.0; K += 0.05)
    {
        for (int bonus = 0; bonus <= 30; bonus++)
        {
            double error = calculateError(bonus, K);
            if (error < minError)
            {
                minError = error;
                bestK = K;
                bestBonus = bonus;
            } else { break; }
        }
        std::cout << "K: " << K << " Error: " << minError << std::endl;
    }

    std::cout << "Best K: " << bestK << " Best Bonus for semi open file rooks: " << bestBonus << std::endl;
}


