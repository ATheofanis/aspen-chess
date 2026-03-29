//
// Created by theoa on 10/03/2026.
//

#include "Score.h"

#include "TranspositionTable.h"

Move killerMoves[15][2];
int historyMoves[64][64];

int pawnHashHIT = 0;
int pawnHashMISS = 0;

// CALCULATE DOUBLED PAWN SCORE, ADD FOR WHITE SUBTRACT FOR BLACK --------------===============
int pawnStructureScore(const Position& pos)
{

    Bitboard wps = pos.getPieceBitboard(0);
    Bitboard bps = pos.getPieceBitboard(6);
    Bitboard whitePawns = wps;
    Bitboard blackPawns = bps;


    int numOfWhiteDoubled = 0;
    int numOfWhiteIsolated = 0;

    int numOfBlackDoubled = 0;
    int numOfBlackIsolated = 0;

    int score = 0;

    while (whitePawns)
    {
        int currentPawnSq = popLsbAndReturnIndex(whitePawns);
        int file = currentPawnSq & 7;

        // doubled white pawns
        Bitboard doubledWhitePawns = wps & files[file];
        int whiteDoubledInFile = bitCount(doubledWhitePawns);
        if (whiteDoubledInFile > 1)
        {
            numOfWhiteDoubled += whiteDoubledInFile - 1;
        }

        // isolated white pawns
        if ((isolatedMasks[currentPawnSq] & wps) == 0)
        {
            numOfWhiteIsolated++;
        }

        if ((whitePassedMasks[currentPawnSq] & bps) == 0)
        {
            score += whitePassedPawnBonus[currentPawnSq / 8];
        }

    }

    while (blackPawns)
    {
        int currentPawnSq = popLsbAndReturnIndex(blackPawns);
        int file = currentPawnSq & 7;

        // doubled black pawns
        Bitboard doubledBlackPawns = bps & files[file];
        int blackDoubledInFile = bitCount(doubledBlackPawns);
        if (blackDoubledInFile > 1)
        {
            numOfBlackDoubled += blackDoubledInFile - 1;
        }

        // isolated black pawns
        if ((isolatedMasks[currentPawnSq] & bps) == 0)
        {
            numOfBlackIsolated++;
        }

        if ((blackPassedMasks[currentPawnSq] & wps) == 0)
        {
            score -= blackPassedPawnBonus[currentPawnSq / 8];
        }

    }




    //std::cout << "DOUBLED WHITE / BLACK PAWNS: " << numOfWhiteDoubled << " / " << numOfBlackDoubled << std::endl;
    //std::cout << "ISOLATED WHITE / BLACK PAWNS: " << numOfWhiteIsolated << " / " << numOfBlackIsolated << std::endl;

    //return pos.isWhiteToMove() ? ((numOfWhiteDoubled - numOfBlackDoubled) * doubledPawnPenalty) : -((numOfWhiteDoubled - numOfBlackDoubled) * doubledPawnPenalty);
    return score + ((numOfWhiteDoubled - numOfBlackDoubled) * doubledPawnPenalty) + (numOfWhiteIsolated - numOfBlackIsolated) * isolatedPawnPenalty;
}
// CALCULATE DOUBLED PAWN SCORE, ADD FOR WHITE SUBTRACT FOR BLACK --------------===============


int rookOpenFileScore(const Position& pos)
{
    Bitboard wRs = pos.getPieceBitboard(wR);
    Bitboard bRs = pos.getPieceBitboard(bR);
    Bitboard wps = pos.getPieceBitboard(wp);
    Bitboard bps = pos.getPieceBitboard(bp);
    Bitboard pawns = bps | wps;
    int whiteRookScore = 0;
    while (wRs)
    {
        int currentRookSq = popLsbAndReturnIndex(wRs);
        int file = currentRookSq & 7;
        Bitboard pawnsInFile = files[file] & pawns;
        if (pawnsInFile == 0)
        {
            whiteRookScore += openFileBonus;
        } else if (pawnsInFile == 1)
        {
            whiteRookScore += semiOpenFileBonus;
        }
    }
    int blackRookScore = 0;
    while (bRs)
    {
        int currentRookSq = popLsbAndReturnIndex(bRs);
        int file = currentRookSq & 7;
        Bitboard pawnsInFile = files[file] & pawns;
        if (pawnsInFile == 0)
        {
            blackRookScore += openFileBonus;
        } else if (pawnsInFile == 1)
        {
            blackRookScore += semiOpenFileBonus;
        }
    }
    return whiteRookScore - blackRookScore;
}


int scoreBoard(const Position& pos)
{
    int score = 0;
    score += pos.getTotalPSTAndMaterialScore();


    // pawn score using pawn hash table:
    int pawnEval = probePawnHash(pos.getPawnZobristHash());
    // first probe to see if the hash table has stored this position before
    if ( pawnEval != NO_HASH_ENTRY )
    {
        pawnHashHIT++;
        // we found the same position in the hash table with a precomputed pawn structure score, so add that to our current
        score += pawnEval;
    }
    else
    {
        pawnHashMISS++;
        // a new pawn position has been encountered, manual compute the score and store it in the pawn hash table
        int pawnScore = pawnStructureScore(pos);
        score += pawnScore;
        savePawnHash(pos.getPawnZobristHash(), pawnScore);
    }
    //score += rookOpenFileScore(pos);

    return pos.isWhiteToMove() ? score : -score;
}


int scoreQuiescenceMove(const Move& move, Position& pos, const Move& hashMove)
{
    int score = 0;
    if (move == hashMove)
    {
        return 64000;
    }

    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;

    // MVV-LVA for quiescence
    //if (flag & 4)
    //{

    Piece victim = wp;
    if (flag == 5)
    {
        victim = wp;
    }
    else
    {
        victim = pos.getPieceFromBoard(toSquare);
    }
    Piece attacker = pos.getPieceFromBoard(fromSquare);

    score = 10 * averagePieceScore[(int)(victim) % 6] - averagePieceScore[(int)(attacker) % 6];

    return score;


}

int scoreMove(const Move& move, const Position& pos, const Move& hashMove, const int& ply)
{
    if (move == hashMove)
    {
        return 10000;
    }



    int flag = (move >> 12) & 0x3F;

    // MVV-LVA for capture moves
    if (flag & 4)
    {
        int fromSquare = move & 0x3F;
        int toSquare = (move >> 6) & 0x3F;

        Piece victim = wp;
        if (flag == 5)
        {
            victim = wp;
        } else
        {
            victim = pos.getPieceFromBoard(toSquare);
        }
        Piece attacker = pos.getPieceFromBoard(fromSquare);

        return 1500 + 10 * averagePieceScore[(int)(victim) % 6] - averagePieceScore[(int)(attacker) % 6];
    }

    // killer moves (only non captures here)
    if (move == killerMoves[ply][0])
        return 900;
    if (move == killerMoves[ply][1])
        return 850;

    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;

    return historyMoves[fromSquare][toSquare];
}