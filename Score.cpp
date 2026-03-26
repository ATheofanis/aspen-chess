//
// Created by theoa on 10/03/2026.
//

#include "Score.h"

Move killerMoves[15][2];
int historyMoves[64][64];

int scoreBoard(const Position& pos)
{
    int score = 0;
    score += pos.getTotalPSTAndMaterialScore();
    return pos.isWhiteToMove() ? score : -score;
}

int scoreQuiescenceMove(const Move& move, Position& pos, const Move& hashMove)
{
    int score = 0;
    if (move == hashMove)
    {
        return 10000;
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
        } else
        {
            victim = pos.getPieceFromBoard(toSquare);
        }
        Piece attacker = pos.getPieceFromBoard(fromSquare);

        //score += pos.SEE(toSquare, victim, fromSquare, pos.getPieceFromBoard(fromSquare)) * 10;

        return score + 1000 + 10 * averagePieceScore[(int)(victim) % 6] - averagePieceScore[(int)(attacker) % 6];

    //}


    return 0;
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