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
int kingAttackersScore(int targetSquare, Color attackingSideColor, const Position& pos)
{
    int attackersScore = 0;
    Bitboard occupied = pos.getOccupiedBitboard();
    // delta is 6 for black and 0 for white for piece indecies
    int attackingSideDelta = (attackingSideColor == White) ? 0 : 6;

    Bitboard attackingKing    = pos.getPieceBitboard(attackingSideDelta + 5);
    Bitboard attackingQueens  = pos.getPieceBitboard(attackingSideDelta + 4);
    Bitboard attackingRooks   = pos.getPieceBitboard(attackingSideDelta + 3);
    Bitboard attackingBishops = pos.getPieceBitboard(attackingSideDelta + 2);
    Bitboard attackingKnights = pos.getPieceBitboard(attackingSideDelta + 1);
    Bitboard attackingPawns   = pos.getPieceBitboard(attackingSideDelta + 0);

    // to get the rook attacks i use the occupancy without rook pieces to account for battery attacks (for example rook behind a queen)
    Bitboard rookAttacks = getRookAttacks(targetSquare, occupied & ~(attackingRooks | attackingQueens));

    // same for diagonal attackers, a bishop behind a queen should count as an attacker
    Bitboard bishopAttacks = getBishopAttacks(targetSquare, occupied & ~(attackingBishops | attackingQueens));

    //int numOfQueenAttackers, numOfRookAttackers, numOfBishopAttackers;
    //int numOfKnightAttackers, numOfPawnAttackers;

    int numOfQueenAttackers = bitCount((rookAttacks | bishopAttacks) & attackingQueens);
    int numOfRookAttackers = bitCount(rookAttacks & attackingRooks);
    int numOfBishopAttackers = bitCount(bishopAttacks & attackingBishops);
    int numOfKnightAttackers = bitCount(knightAttacks[targetSquare] & attackingKnights);
    int numOfPawnAttackers = bitCount(pawnAttacks[attackingSideColor == White ? Black : White][targetSquare] & attackingPawns);


    attackersScore += numOfQueenAttackers * queenAttackerWeight;
    attackersScore += numOfRookAttackers * rookAttackerWeight;
    attackersScore += numOfBishopAttackers * bishopAttackerWeight;
    attackersScore += numOfKnightAttackers * knightAttackerWeight;
    attackersScore += numOfPawnAttackers * pawnAttackerWeight;


    return attackersScore;
}


// CALCULATE DOUBLED PAWN SCORE, ADD FOR WHITE SUBTRACT FOR BLACK --------------===============
int pawnStructureScore(const Position& pos)
{

    Bitboard wps = pos.getPieceBitboard(0);
    Bitboard bps = pos.getPieceBitboard(6);
    Bitboard whitePawns = wps;
    Bitboard blackPawns = bps;


    int wpPassedPawnBonus = 0;
    int bpPassedPawnBonus = 0;

    int numOfWhiteDoubled = 0;
    int numOfWhiteIsolated = 0;

    int numOfBlackDoubled = 0;
    int numOfBlackIsolated = 0;

    int score = 0;

    // white pawns
    while (whitePawns)
    {
        int currentPawnSq = popLsbAndReturnIndex(whitePawns);


        // doubled white pawns
        for (int i = 0; i < 8; i++)
        {
            Bitboard doubledWhitePawns = wps & files[i];

            int whiteDoubledInFile = bitCount(doubledWhitePawns);

            if (whiteDoubledInFile > 1)
            {
                numOfWhiteDoubled += whiteDoubledInFile - 1;
            }
        }




        // isolated white pawns
        if ((isolatedMasks[currentPawnSq] & wps) == 0)
        {
            numOfWhiteIsolated++;
        }

        // white passed pawns
        if ((whitePassedMasks[currentPawnSq] & bps) == 0)
        {
            wpPassedPawnBonus += whitePassedPawnBonus[currentPawnSq / 8];
        }

    }

    // black pawns
    while (blackPawns)
    {
        int currentPawnSq = popLsbAndReturnIndex(blackPawns);



        // doubled black pawns
        for (int i = 0; i < 8; i++)
        {
            Bitboard doubledBlackPawns = bps & files[i];

            int blackDoubledInFile = bitCount(doubledBlackPawns);

            if (blackDoubledInFile > 1)
            {
                numOfBlackDoubled += blackDoubledInFile - 1;
            }
        }




        // isolated black pawns
        if ((isolatedMasks[currentPawnSq] & bps) == 0)
        {
            numOfBlackIsolated++;
        }


        // black passed pawns
        if ((blackPassedMasks[currentPawnSq] & wps) == 0)
        {
            bpPassedPawnBonus -= blackPassedPawnBonus[currentPawnSq / 8];
        }

    }





    return score + ((numOfWhiteDoubled - numOfBlackDoubled) * 0) + (numOfWhiteIsolated - numOfBlackIsolated) * 0;

    //return score + (numOfWhiteIsolated - numOfBlackIsolated) * isolatedPawnPenalty;
    return score + ((numOfWhiteDoubled - numOfBlackDoubled) * doubledPawnPenalty) + (numOfWhiteIsolated - numOfBlackIsolated) * isolatedPawnPenalty + wpPassedPawnBonus - bpPassedPawnBonus;
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


    //return score;
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

    //if (ply > 0 && previousMoves[ply-1] != NO_MOVE)
    //{
    //    Move prevMove = previousMoves[ply-1];
    //    int prevMoveFromSquare = prevMove & 0x3F;
    //    int prevMoveToSquare = (prevMove >> 6) & 0x3F;
    //    if (counterMoves[prevMoveFromSquare][prevMoveToSquare] == move)
    //    {
    //        return 800;
    //    }
    //}

    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;

    return historyMoves[fromSquare][toSquare];
}