//
// Created by theoa on 03/05/2026.
//

// This file includes functions that are used to generate data to feed to a NNUE trainer

#include "DataGen.h"
#include <cassert>
#include <fstream>
#include <iostream>

#include "Attacks.h"
#include "Bitboard.h"
#include "Tuning.h"

#include <windows.h>

#include "MoveGen.h"
#include "Position.h"
#include "PRNG.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include <vector>
#include <sstream>
#include <iomanip>

#include "Time.h"

void init()
{
    initLMR();
    clearTranspositionTable();
    clearPawnTranspositionTable();
    seedingForXoshiro256aa();
    initPSTtables();
    SetConsoleOutputCP(CP_UTF8);
    initPawnAttacks();
    initKnightAttacks();
    initKingAttacks();
    initRookAttacks();
    initBishopAttacks();
    initLineBetween();
    initZobrist();
    initializePawnStructureMasks();
}

void generateData()
{
    init();

    std::ofstream data;

    data << std::fixed << std::setprecision(1);

    TimePoint wtime = 5000000;
    TimePoint btime = 5000000;
    TimePoint winc = 5000;
    TimePoint binc = 5000;
    int movestogo = 1;

    data.open("data0.txt", std::ios::app);

    if (!data.is_open())
    {
        std::cerr << "ERROR: Failed to open data.txt !!!!!!!!\n";
        return;
    }

    for (int i = 0; i < NumberOfGames; i++)
    {

        // In the opening phase play 8 random moves in order to ensure there is enough position variety
        OPENING:

        clearTranspositionTable();
        clearPawnTranspositionTable();
        Position pos;


        // Temporarily store positions in a buffer until the end of the game where we know there wdl value
        DataGenEntry entriesBuffer[maxNumberOfMoves];
        std::memset(entriesBuffer, 0, sizeof(entriesBuffer));

        // Track the zobrist hash of each position in order to check for two fold repetition
        std::vector<ZobristHash> hashHistory;
        hashHistory.reserve(maxNumberOfMoves);
        hashHistory.push_back(pos.getZobristHash());

        for (int j = 0; j < NumOfRandomOpeningMoves; j++)
        {
            Move moves[256];
            int numOfMoves = 0;

            // First we extract the legality information for this position which is needed to generate legal moves
            Color allyColor = pos.isWhiteToMove() ? White : Black;
            int kingSquare  = lsbIndex(pos.getPieceBitboard(pos.isWhiteToMove() ? wK : bK));

            legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);

            generateLegalMoves(info, pos, moves, numOfMoves);

            if (numOfMoves == 0)
            {
                goto OPENING;
            }

            // Generate random moves using xoshiro256** to ensure randomness in the opening
            int64_t randomMoveIndex = next() % numOfMoves;
            Move randomMove = moves[randomMoveIndex];

            int randomMoveFlag = randomMove >> 12 & 0x3F;

            pos.makeMove(randomMove);

            hashHistory.push_back(pos.getZobristHash());

            tm.start(500000, 500000, 1);

            int newEntryEval = -999999;
            findBestMove(pos, newEntryEval);
            newEntryEval = pos.isWhiteToMove() ? newEntryEval : -newEntryEval;

            // Only accept openings that do not heavily favor one side
            if (abs(newEntryEval) > evalThreshold)
            {
                goto OPENING;
            }

            // Only store the position if the king is not in check and the move made was not a capture
            if (!(pos.sideToMoveIsInCheck() || (randomMoveFlag & 4)))
            {
                DataGenEntry newEntry;
                newEntry.setEvalAndFen(newEntryEval, pos.boardToFen());
                newEntry.valid = true;
                entriesBuffer[j] = newEntry;
            }
        }

        float wdl = 0.5f;
        int j;

        for (j = NumOfRandomOpeningMoves; j <= maxNumberOfMoves; j++)
        {
            if (j == maxNumberOfMoves)
            {
                wdl = 0.5f;
                break;
            }

            Move moves[256];
            int numOfMoves = 0;

            // Extract the legality information for this position which is needed to generate legal moves
            Color allyColor = pos.isWhiteToMove() ? White : Black;
            int kingSquare  = lsbIndex(pos.getPieceBitboard(pos.isWhiteToMove() ? wK : bK));

            legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);

            generateLegalMoves(info, pos, moves, numOfMoves);

            // Stop if stalemate or checkmate and set the correct wdl
            if (numOfMoves == 0)
            {
                wdl = pos.sideToMoveIsInCheck() ? (allyColor == Black ? 1.0f : 0.0f) : 0.5f;
                break;
            }

            tm.start(500000, 500000, 1);

            int newEntryEval = -999999;

            Move bestMove = findBestMove(pos, newEntryEval);
            newEntryEval = pos.isWhiteToMove() ? newEntryEval : -newEntryEval;
            int bestMoveFlag = bestMove >> 12 & 0x3F;

            // White is winning
            if (newEntryEval >= evalThreshold)
            {
                wdl = 1.0f;
                break;
            }
            // Black is winning
            if (newEntryEval <= -evalThreshold) {
                wdl = 0.0f;
                break;
            }

            if (!(pos.sideToMoveIsInCheck() || (bestMoveFlag & 4)))
            {
                DataGenEntry newEntry;
                newEntry.setEvalAndFen(newEntryEval, pos.boardToFen());
                newEntry.valid = true;
                entriesBuffer[j] = newEntry;
            }

            pos.makeMove(bestMove);

            // Two fold repetition detection
            ZobristHash currentHash = pos.getZobristHash();
            for (ZobristHash hash : hashHistory)
            {
                if (hash == currentHash)
                {
                    wdl = 0.5f;
                    goto WRITE_RESULTS;
                }
            }
            hashHistory.push_back(currentHash);
        }

        // Write the results we have found so far in the data.txt file without overwriting previous data inside the file
        WRITE_RESULTS:

        for (int entry = 0; entry < j; entry++)
        {
            if (!entriesBuffer[entry].valid) continue;
            entriesBuffer[entry].setWdl(wdl);
            data << entriesBuffer[entry].fenStr << " | " << entriesBuffer[entry].evaluation << " | " << entriesBuffer[entry].wdl << "\n";
        }

        data.flush();

        if (i % 100 == 0)
            std::cout << "Games completed: " << i << "/" << NumberOfGames << "\n";
    }
    data.close();
}