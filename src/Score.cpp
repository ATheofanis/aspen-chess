//
// Created by theoa on 10/03/2026.
//

#include "Score.h"

#include "TranspositionTable.h"

Move killerMoves[MAX_PLY][2];
int historyMoves[64][64];
//Move counterMoves[64][64]; // [fromSquare][toSquare]

int pawnHashHIT = 0;
int pawnHashMISS = 0;



// returns a score based on the number of attackers and attackers piece type on a given square
template <Color attackedKingColor>
int kingAttackersScore(const Position& pos)
{
    int numOfAttackers = 0;
    int attackerColorDelta = 0;
    int kingSquare;
    if constexpr (attackedKingColor == White) {
        attackerColorDelta = 6;
        kingSquare = lsbIndex(pos.getPieceBitboard(wK));
    }

    if constexpr (attackedKingColor == Black) {
        attackerColorDelta = 0;
        kingSquare = lsbIndex(pos.getPieceBitboard(bK));
    }

    int attackersScore = 0;

    Bitboard kingRing = kingAttacks[kingSquare];

    Bitboard occupied = pos.getOccupiedBitboard();


    Bitboard attackingQueens  = pos.getPieceBitboard(attackerColorDelta + 4);
    Bitboard aQ = attackingQueens;
    Bitboard attackingRooks   = pos.getPieceBitboard(attackerColorDelta + 3);
    Bitboard aR = attackingRooks;
    Bitboard attackingBishops = pos.getPieceBitboard(attackerColorDelta + 2);
    Bitboard aB = attackingBishops;
    Bitboard attackingKnights = pos.getPieceBitboard(attackerColorDelta + 1);
    Bitboard aK = attackingKnights;

    Bitboard rookTypeAttackers = attackingQueens | attackingRooks;

    while (aQ)
    {
        int qSquare = popLsbAndReturnIndex(aQ);
        if (getQueenAttacks(qSquare, occupied & ~rookTypeAttackers) & kingRing)
        {
            attackersScore += queenAttackerWeight;
            numOfAttackers++;
        }
    }

    while (aR)
    {
        int rSquare = popLsbAndReturnIndex(aR);
        if (getRookAttacks(rSquare, occupied & ~rookTypeAttackers) & kingRing)
        {
            attackersScore += rookAttackerWeight;
            numOfAttackers++;
        }
    }

    while (aB)
    {
        int bSquare = popLsbAndReturnIndex(aB);
        if (getBishopAttacks(bSquare, occupied & ~(attackingQueens | attackingBishops)) & kingRing)
        {
            attackersScore += bishopAttackerWeight;
            numOfAttackers++;
        }
    }

    while (aK)
    {
        int kSquare = popLsbAndReturnIndex(aK);
        if (knightAttacks[kSquare] & kingRing)
        {
            attackersScore += knightAttackerWeight;
            numOfAttackers++;
        }
    }

    if (numOfAttackers < 2) return 0;

    return SafetyTable[attackersScore];
}


// CALCULATE DOUBLED PAWN SCORE, ADD FOR WHITE SUBTRACT FOR BLACK --------------===============
int pawnStructureScore(const Position& pos)
{

    Bitboard wps = pos.getPieceBitboard(0);
    Bitboard bps = pos.getPieceBitboard(6);
    Bitboard whitePawns = wps;
    Bitboard blackPawns = bps;


    int pawnScore = 0;

    for (int i = 0; i < 8; i++) {
        int wCount = bitCount(wps & files[i]);
        if (wCount > 1)
        {
            pawnScore -= (wCount - 1) * doubledPawnPenalty;
        }

        int bCount = bitCount(bps & files[i]);
        if (bCount > 1)
        {
            pawnScore += (bCount - 1) * doubledPawnPenalty;
        }
    }

    while (wps)
    {
        int currentPawnSq = popLsbAndReturnIndex(wps);

        if (!(isolatedMasks[currentPawnSq] & whitePawns))
        {
            pawnScore += isolatedPawnPenalty;
        }

        if (!(whitePassedMasks[currentPawnSq] & blackPawns)) {
            pawnScore += whitePassedPawnBonus[currentPawnSq / 8];
        }
    }

    while (bps)
    {
        int currentPawnSq = popLsbAndReturnIndex(bps);

        if (!(isolatedMasks[currentPawnSq] & blackPawns))
        {
            pawnScore -= isolatedPawnPenalty;
        }

        if (!(blackPassedMasks[currentPawnSq] & whitePawns)) {
            pawnScore -= blackPassedPawnBonus[currentPawnSq / 8];
        }
    }

    return pawnScore;
}
// CALCULATE DOUBLED PAWN SCORE, ADD FOR WHITE SUBTRACT FOR BLACK --------------===============


int rookOpenFileScore(const Position& pos)
{
    Bitboard wRs = pos.getPieceBitboard(wR);
    Bitboard bRs = pos.getPieceBitboard(bR);
    Bitboard wps = pos.getPieceBitboard(wp);
    Bitboard bps = pos.getPieceBitboard(bp);
    Bitboard pawns = bps | wps;

    int openFileRookNetScore = 0;

    while (wRs)
    {
        int currentRookSq = popLsbAndReturnIndex(wRs);
        Bitboard file = files[currentRookSq & 7];

        if ((file & pawns) == 0)
        {
            openFileRookNetScore += openFileBonus;
        }
    }


    while (bRs)
    {
        int currentRookSq = popLsbAndReturnIndex(bRs);
        Bitboard file = files[currentRookSq & 7];

        if ((file & pawns) == 0)
        {
            openFileRookNetScore -= openFileBonus;
        }
    }


    return openFileRookNetScore;
}



int getBishopPairScore(const Position& pos)
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
    return bishopPairDiff * bishopPairBonus;
}


// NNUE evaluation of the position
int scoreBoardNNUE(const Position& pos)
{
    Accumulator acc(pos);

    // Call the NNUE evaluation function
    int NNUEscore = evaluateNNUE(acc, pos.isWhiteToMove() ? White : Black);

    return NNUEscore;
}

// Hand-crafted Evaluation score of the position
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


    score += rookOpenFileScore(pos);

    score += getBishopPairScore(pos);


    int whiteAttackScore = 0;
    if (pos.getPieceBitboard(wQ))
    {
        whiteAttackScore = kingAttackersScore<Black>(pos);
    }

    int blackAttackScore = 0;
    if (pos.getPieceBitboard(bQ))
    {
        blackAttackScore = kingAttackersScore<White>(pos);
    }
    score += whiteAttackScore - blackAttackScore;

    return pos.isWhiteToMove() ? score : -score;
}

// Function used for move ordering, specifically inside quiescent search
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


// Function used to order moves from best to worst
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