//
// Created by theoa on 06/03/2026.
//

#include "Bitboard.h"
#include <iostream>

Bitboard lineBetween[64][64];

// function to print a bitboard
void printBitboard(Bitboard bb)
{
    std::cout << "\n";
    std::cout << "       WHITE SIDE      \n";
    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // convert file and rank into square index
            int square = rank * 8 + file;

            // print ranks
            if (!file)
            {
                std::cout << rank+1 << "   ";
            }



            // print bit state (1 or 0)
            std::cout << (getBit(bb, square) ? 1 : 0) << " ";
        }
        // print new line after every rank
        std::cout << "\n";

    }
    std::cout << "       BLACK SIDE       ";
    // print board files
    std::cout << "\n    a b c d e f g h" << std::endl;

    // print bitboard as unsigned decimal number
    std::cout << "      Bitboard: "<<  bb << std::endl;
}



void initLineBetween()
{
    for (int sq1 = 0; sq1 < 64; sq1++)
    {
        for (int sq2 = 0; sq2 < 64; sq2++)
        {
            if (sq1 == sq2) continue;

            lineBetween[sq1][sq2] = 0ULL;

            int fileSq1 = sq1 % 8; // a6 = 40
            int fileSq2 = sq2 % 8;

            int rankSq1 = sq1 / 8;
            int rankSq2 = sq2 / 8;

            int rankDelta = rankSq2 - rankSq1;
            int fileDelta = fileSq2 - fileSq1;

            if (fileDelta != 0 && rankDelta != 0 && abs(fileDelta) != abs(rankDelta)) continue;

            int fileStep = (fileDelta == 0) ? 0 : (fileDelta > 0 ? 1 : -1);
            int rankStep = (rankDelta == 0) ? 0 : (rankDelta > 0 ? 1 : -1);

            int steps = std::max(abs(fileDelta), abs(rankDelta));

            int file = fileSq1;
            int rank = rankSq1;

            for (int i = 0; i < steps; i++)
            {
                int sq = rank * 8 + file;

                lineBetween[sq1][sq2] |= (1ULL << sq);
                rank += rankStep;
                file += fileStep;
            }
            lineBetween[sq1][sq2] |= (1ULL << sq2);
            lineBetween[sq1][sq2] &= ~(1ULL << sq1);
        }
    }
}