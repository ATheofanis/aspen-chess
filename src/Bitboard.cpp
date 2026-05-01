//
// Created by theoa on 06/03/2026.
//

#include "Bitboard.h"
#include <iostream>

Bitboard lineBetween[64][64];

Bitboard fileMasks[64];
Bitboard rankMasks[64];
Bitboard isolatedMasks[64];
Bitboard whitePassedMasks[64];
Bitboard blackPassedMasks[64];


void initializePawnStructureMasks()
{
    // initialize rank masks
    for (int rank = 0; rank < 8; rank++)
    {
        for (int sqInRank = 0; sqInRank < 8; sqInRank++)
        {
            int square = rank * 8 + sqInRank;
            rankMasks[square] = ranks[rank];
        }
    }

    // initialize file masks
    for (int file = 0; file < 8; file++)
    {
        for (int sqInFile = 0; sqInFile < 8; sqInFile++)
        {
            int square = file + sqInFile * 8;
            fileMasks[square] = files[file];
        }
    }

    // initialize isolated pawn masks
    for (int sq = 0; sq < 64; sq++)
    {
        if ((sq & 7) == 0) // if sq mod 8 is 0 meaning it is in fileA
        {
            isolatedMasks[sq] = fileMasks[sq+1];
        } else if ((sq & 7) == 7)
        {
            isolatedMasks[sq] = fileMasks[sq-1];
        } else
        {
            isolatedMasks[sq] = fileMasks[sq+1] | fileMasks[sq-1];
        }
    }

    // initialize passed pawn masks:
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int sq = rank * 8 + file;

            Bitboard passedPawnMask = isolatedMasks[sq] | fileMasks[sq];

            Bitboard wpMask = passedPawnMask; // white pawns
            Bitboard bpMask = passedPawnMask; // black pawns


            for (int whiteRank = 0; whiteRank <= rank; whiteRank++ )
            {
                wpMask &= ~ranks[whiteRank];
            }
            for (int blackRank = 7; blackRank >= rank; blackRank-- )
            {
                bpMask &= ~ranks[blackRank];
            }

            whitePassedMasks[sq] = wpMask;
            blackPassedMasks[sq] = bpMask;
        }
    }

}




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


// Initializer for the lineBetween array. It calculates the squares between every pair of squares in the chess board.
// The line includes every square between square1 and square2 but also square2 itself
void initLineBetween()
{
    for (int sq1 = 0; sq1 < 64; sq1++)
    {
        for (int sq2 = 0; sq2 < 64; sq2++)
        {
            if (sq1 == sq2) continue;

            lineBetween[sq1][sq2] = 0ULL;

            // Calculate the file of each square
            int fileSq1 = sq1 % 8;
            int fileSq2 = sq2 % 8;

            // Calculate the rank of each square
            int rankSq1 = sq1 / 8;
            int rankSq2 = sq2 / 8;

            // Use the squares delta to move horizontally, vertically or diagonally depending on the values of rank delta and file delta
            int rankDelta = rankSq2 - rankSq1;
            int fileDelta = fileSq2 - fileSq1;

            if (fileDelta != 0 && rankDelta != 0 && abs(fileDelta) != abs(rankDelta)) continue;

            // We calculate the file and rank step which determine the direction of each step we take
            // For example if the file delta is greater than 0 it means that the file of square2 is greater than the file of square1
            // Then the file step is set to 1 because in order to reach the file of square2 coming from square1 we need to increment the current file index
            int fileStep = (fileDelta == 0) ? 0 : (fileDelta > 0 ? 1 : -1);
            int rankStep = (rankDelta == 0) ? 0 : (rankDelta > 0 ? 1 : -1);

            int steps = std::max(abs(fileDelta), abs(rankDelta));

            int file = fileSq1;
            int rank = rankSq1;

            // Loop through every square inbetween
            for (int i = 0; i < steps; i++)
            {
                int sq = rank * 8 + file;

                // Use OR to add the new square to the line between bitboard of the two squares
                lineBetween[sq1][sq2] |= (1ULL << sq);
                // increment rank and file based on the steps we calculated earlier
                rank += rankStep;
                file += fileStep;
            }
            // include the square of square2 and exclude the square of square1
            // This way we can immediately find the squares that a bishop or a rook is attacking with a simple array call
            lineBetween[sq1][sq2] |= (1ULL << sq2);
            lineBetween[sq1][sq2] &= ~(1ULL << sq1);
        }
    }
}


