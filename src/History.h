//
// Created by theoa on 26/05/2026.
//

#pragma once
#include "Search.h"

inline int16_t* getContHistEntry(SearchStack *ss, Piece movingPiece, int toSquare, int offset)
{
    return (&(*(ss-offset)->contHistory)[movingPiece][toSquare]);
}

inline int getContHistoryScores(SearchStack* ss, Piece movingPiece, int toSquare, int ply)
{
    constexpr int offsets[3] = {1, 2, 4};
    int totalScore = 0;

    for (int offset : offsets)
    {
        if (ply >= offset && (ss-offset)->move != NO_MOVE && (ss-offset)->contHistory != nullptr)
        {
            // Get the adress of the entry from the continuation history table of the search stack
            totalScore += *getContHistEntry(ss, movingPiece, toSquare, offset);
        }
    }
    return totalScore;
}

inline void applyHistoryBonus(int16_t *entry, int bonus)
{
    *entry += bonus - *entry * abs(bonus) / MaxHistoryScore;
}

// Apply bonus/penalty to the continuation history tables
// The offsets are 1, 2 and 4. 1 is for the opponents last move, 2 is for our last move, and for is for our second last move
inline void updateContHistories(SearchStack* ss, int ply, Piece movingPiece, int toSquare, int bonus)
{
    constexpr int offsets[3] = {1, 2, 4};

    for (int offset : offsets)
    {
        if (ply >= offset && (ss-offset)->move != NO_MOVE && (ss-offset)->contHistory != nullptr)
        {
            // Get the adress of the entry from the continuation history table of the search stack
            int16_t* contHistoryEntry = getContHistEntry(ss, movingPiece, toSquare, offset);

            // Finally apply the bonus/penalty
            applyHistoryBonus(contHistoryEntry, bonus);
        }
    }
}