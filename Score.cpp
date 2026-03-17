//
// Created by theoa on 10/03/2026.
//

#include "Score.h"

int scoreBoard(const Position& pos)
{
    int score = 0;
    score += pos.getTotalPSTAndMaterialScore();
    return pos.isWhiteToMove() ? score : -score;
}

int scoreQuiescenceMove(const Move& move, const Position& pos)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;

    // MVV-LVA for quiescence
    if (flag & 4)
    {
        Piece victim = wp;
        if (flag == 5)
        {
            victim = wp;
        } else
        {
            victim = pos.getPieceFromBoard(toSquare);
        }
        Piece attacker = pos.getPieceFromBoard(fromSquare);

        return 1000 + 10 * mgPieceScore[(int)(victim) % 6] - mgPieceScore[(int)(attacker) % 6];
    }
    return 0;
}

int scoreMove(const Move& move, const Position& pos, const Move& bestMove)
{
    if (move == bestMove)
    {
        return 10000;
    }

    int flag = (move >> 12) & 0x3F;

    // MVV-LVA for quiescence
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

        return 1000 + 10 * mgPieceScore[(int)(victim) % 6] - mgPieceScore[(int)(attacker) % 6];
    }
    return 0;
}