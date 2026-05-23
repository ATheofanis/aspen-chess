//
// Created by theoa on 12/05/2026.
//

#include "UCI.h"
#include <cassert>
#include <iostream>
#include "nnue/Nnue.h"
#include "nnue/Accumulator.h"

#include "Attacks.h"
#include "Bitboard.h"
#include "Tuning.h"


#include "LegalMoveGen.h"
#include "Position.h"
#include "PRNG.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include <vector>
#include <sstream>
#include <fstream>
#include <thread>

#include "DataGen.h"
#include "TimeManager.h"

class Position;

constexpr uint64_t MaxValue = std::numeric_limits<uint64_t>::max();

int globalMTG = 40;

// print the Unicode character
inline void printUnicode(UnicodePiece uP) {
    //std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    //std::cout << conv.to_bytes(static_cast<char32_t>(uP));
    std::cout << " ";
}

void printBoard(const Position& pos)
{
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            Piece piece = pos.getPieceFromBoard(rank * 8 + file);

            // print ranks
            if (!file)
            {
                std::cout << rank+1 << "   ";
            }

            if (piece == NO_PIECE)
            {
                std::cout << " ."; continue;
            }
            printUnicode(static_cast<UnicodePiece>(9823 - piece));
        }

        std::cout << "\n";
    }
    // print board files
    std::cout << "    a  b c  d  e  f g  h\n" << std::endl;

    // print the side to move
    std::cout <<"Side to move: ";
    if (pos.isWhiteToMove()) { std::cout << "White\n";}
    else { std::cout << "Black\n";}

    // print available castling rights
    std::cout <<"Castling rights: ";
    CastlingRights castlingRights = pos.getCastlingRights();
    std::cout << (castlingRights & WK ? "WK / " : "- / ");
    std::cout << (castlingRights & WQ ? "WQ / " : "- / ");
    std::cout << (castlingRights & BK ? "BK / " : "- / ");
    std::cout << (castlingRights & BQ ? "BQ  " : "-");

    // print en passant square
    std::cout << "\nEn Passant square: ";
    std::string epSqName = squareName((Square)pos.getEnpassantSquare());
    std::cout << epSqName;
    std::cout << "\n";
    std::cout << "\n";
}

void printMoveNoMessages(Move& move)
{

    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0xF;
    std::cout << squareName(static_cast<Square>(fromSquare)) << squareName(static_cast<Square>(toSquare));
    if (flag & 8)
    {
        int promotedPieceIndex = flag % 4;
        switch (promotedPieceIndex)
        {
        case 0:
            std::cout << 'n';
            break;
        case 1:
            std::cout << 'b';
            break;
        case 2:
            std::cout << 'r';
            break;
        default:
            std::cout << 'q';
            break;
        }

    }

}

long long Perft(Position& pos, int depth)
{
    //std::cout << "PERFT FOR MOVE:" << std::endl;
    //printMove(move);

    Move moves[256];
    int numOfMoves = 0, i;
    long long nodes = 0;

    if (depth == 0) return 1ULL;

    //std::cout << "BEFORE LEGALITY EXTRACTION" << numOfMoves << std::endl;

    legalityInformation info = getLegalityInfo(lsbIndex(pos.isWhiteToMove() ? pos.getPieceBitboard(wK) : pos.getPieceBitboard(bK)), pos.isWhiteToMove() ? White : Black, pos);

    //std::cout << " GENERATING LEGAL MOVES:" << std::endl;

    generateLegalMoves(info, pos, moves, numOfMoves);

    //std::cout << " MADE IT AFTER LEGALITY EXTRACTION AND MOVE GEN PREPARING FOR LOOP WITH " << numOfMoves << " NUM OF MOVES." << std::endl;

    for (i = 0; i < numOfMoves; i++)
    {

        //printMove(moves[i]);
        pos.makeMove(moves[i]);


        nodes += Perft(pos, depth - 1);
        pos.unmakeMove();



    }
    return nodes;
}


void dividePerft(Position& pos, int depth)
{
    Move moves[256];
    int numOfMvs = 0;


    legalityInformation info = getLegalityInfo(lsbIndex(pos.isWhiteToMove() ? pos.getPieceBitboard(wK) : pos.getPieceBitboard(bK)), pos.isWhiteToMove() ? White : Black, pos);

    //std::cout << " GENERATING LEGAL MOVES:" << std::endl;

    generateLegalMoves(info, pos, moves, numOfMvs);

    long long total = 0;
    for (int i = 0; i < numOfMvs; i++)
    {
        long long nodes = 0;
        pos.makeMove(moves[i]);
        nodes += Perft(pos, depth - 1);
        total += nodes;
        pos.unmakeMove();
        printMoveNoMessages(moves[i]);
        std::cout <<": " << nodes << "- RAW MOVE:" << moves[i] << std::endl;
    }
    std::cout << "\n Nodes searched: " << total << std::endl;

}




int depth = 8;


Move parseMove(const std::string& moveString, Position pos)
{
    Move moves[256];
    int numOfMoves = 0;

    int moveStringSize = moveString.size();

    int kingSq;
    Color allyColor;
    //int pieceColorDelta;
    if (pos.isWhiteToMove())
    {
        kingSq = lsbIndex(pos.getPieceBitboard(wK));
        allyColor = White;
        //pieceColorDelta = 0;
    } else
    {
        kingSq = lsbIndex(pos.getPieceBitboard(bK));
        allyColor = Black;
        //pieceColorDelta = 6;
    }
    legalityInformation info = getLegalityInfo(kingSq, allyColor, pos);

    generateLegalMoves(info, pos, moves, numOfMoves);


    Move startSq = (moveString[0] - 'a') + (moveString[1] - '1') * 8;
    Move endSq = (moveString[2] - 'a') + (moveString[3] - '1') * 8;
    //Move move = (startSq) | (endSq << 6);

    int promotedPiece = 0;
    if (moveStringSize > 4)
    {
        switch (moveString[4])
        {
        case 'n':
            promotedPiece = 0;
            break;
        case 'b':
            promotedPiece = 1;
            break;
        case 'r':
            promotedPiece = 2;
            break;
        default:
            promotedPiece = 3;
            break;
        }
    }

    // find the matching move and return it
    for (int i = 0; i < numOfMoves; i++)
    {
        Move currMove = moves[i];
        int fromSquare = currMove & 0x3F;
        int toSquare = (currMove >> 6) & 0x3F;
        int flag = (currMove >> 12) & 0xF;
        if (startSq == fromSquare && endSq == toSquare)
        {
            if (flag & 8)
            {
                if (moveStringSize > 4)
                {
                    int promotedPieceIndex = flag % 4;
                    if (promotedPieceIndex == promotedPiece) return currMove;
                }
            } else
            {
                return currMove;
            }
        }
    }
    // no legal move was found - problem
    return 0;
}


Position parsePosition(std::vector<std::string> tokens, int startingIndex)
{
    Position pos;

    for (int i = startingIndex; i < tokens.size(); i++)
    {
        pos.makeMove(parseMove(tokens[i], pos));
    }

    return pos;
}


void parseFenPosition(std::vector<std::string> tokens, int startingIndex, Position& pos)
{
    for (int i = startingIndex; i < tokens.size(); i++)
    {
        pos.makeMove(parseMove(tokens[i], pos));
    }
}


// returns a vector of tokens from a given command string , using istringstream and >> loop method (https://www.tutorialspoint.com/article/how-to-process-strings-using-std-istringstream)
std::vector<std::string> tokenize(const std::string& command)
{
    std::istringstream issCommand;
    issCommand.str(command);

    std::string commandStr;
    std::vector<std::string> tokens;

    while (issCommand >> commandStr)
    {
        tokens.push_back(commandStr);
    }
    return tokens;
}


void uciRunSearch(MoveSearcher& searcher, const Position& pos)
{
    // Find best Move
    int evalDummy;

    Move bestMove = searcher.findBestMove(pos, evalDummy);

    // Print best move
    std::cout << "bestmove ";
    printMoveNoMessages(bestMove);
    std::cout << std::endl;
}



inline void initializations()
{
    initLMR();
    seedingForXoshiro256aa();
    initPawnAttacks();
    initKnightAttacks();
    initKingAttacks();
    initRookAttacks();
    initBishopAttacks();
    initLineBetween();
    initZobrist();
    initializePawnStructureMasks();
    loadQuantised();
}



void LoopUCI()
{
    initializations();

    // Default hash table size is 64 megabytes
    resizeTranspositionTable(64);

    std::string command;
    Position pos;

    MoveSearcher searcher;

    std::thread searcherThread;

    while (getline(std::cin, command))
    {
        if (command.empty()) continue;

        std::vector<std::string> tokens = tokenize(command);

        // Custom command for data generation
        if (tokens[0] == "begin")
        {
            if (tokens.size() >= 2 && tokens[1] == "datagen")
            {
                NumberOfGames = std::atoi(tokens[2].c_str());
                std::string dataFile = tokens[3];

                TimeMs startingTime = now();
                MAX_NODES = dataGenMaxNodes;
                DataGenFlag = true;
                generateData(dataFile);
                TimeMs totalTime = now() - startingTime;
                std::cout << "Data generation finished. Generated " << NumberOfGames << "games in " << totalTime << " ms." << std::endl;
            }
        }
        // position command
        else if (tokens[0] == "position")
        {
            // non fen string ---------
            if (tokens.size() >= 2 && tokens[1] == "startpos")
            {
                if (tokens[2] == "moves")
                {
                    pos = parsePosition(tokens, 3);
                }
            }// non fen string --------

            // fen string --------------
            else if (tokens[1] == "fen")
            {
                bool movesFlag = false;
                int movesTokenIndex = 0;
                std::string fenString;
                for (int i = 2; i < tokens.size(); i++)
                {
                    if (tokens[i] == "moves")
                    {
                        movesTokenIndex = i+1;
                        movesFlag = true;
                        break;
                    }
                    fenString += tokens[i];
                    fenString += ' ';
                }

                pos.loadFen(fenString);
                if (movesFlag)
                {
                    parseFenPosition(tokens, movesTokenIndex, pos);
                }

                // fen string --------------
            }
        }
        else if (tokens[0] == "go")
        {
            MAX_NODES = MaxValue;
            TimeMs wtime = MaxValue;
            TimeMs btime = MaxValue;
            TimeMs winc = 0;
            TimeMs binc = 0;
            int movestogo = 40;
            bool timeEnabled = false;

            for (int i = 1; i < tokens.size(); i++)
            {
                if (tokens[i] == "wtime") // white remaining time
                {
                    timeEnabled = true;
                    wtime = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "btime") // black remaining time
                {
                    timeEnabled = true;
                    btime = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "winc") // white increment
                {
                    timeEnabled = true;
                    winc = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "binc") // black increment
                {
                    timeEnabled = true;
                    binc = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "movestogo") // moves to go passed by the GUI
                {
                    movestogo = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "depth") // change the maximum depth if the depth command is passed
                {
                    timeManager.disableTimeControl();
                    MAX_DEPTH = std::atoi(tokens[i+1].c_str());
                }
                else if (tokens[i] == "nodes") // set the maximum amount of nodes that can be searched
                {
                    timeManager.disableTimeControl();
                    MAX_NODES = std::atoi(tokens[i+1].c_str());
                }
            }

            // If time controls were enabled, which is determined based on UCI input, then start the time manager
            if (timeEnabled) {
                TimeMs myTime = (pos.isWhiteToMove()) ? wtime : btime;
                TimeMs myInc = (pos.isWhiteToMove()) ? winc : binc;
                timeManager.start(myTime, myInc, movestogo); // initialize the time manager for the side to move
            }
            // Otherwise reset the time manager, which basically turns time controls off but keeps track of current time
            // in order to continue printing search statistics, such as NPS and search time
            else
            {
                timeManager.reset();
            }

            searcher.uciStop = true;

            if (searcherThread.joinable()) searcherThread.join();

            searcher.uciStop = false;

            // Run the search using the searcher thread so that we can search while also reading UCI commands
            searcherThread = std::thread([&](){ uciRunSearch(searcher, pos); });

            globalMTG--;
            if (globalMTG <= 0)
            {
                globalMTG = 30;
            }

        }
        else if (tokens[0] == "isready")
        {
            std::cout << "readyok\n" << std::endl;
        }
        else if (tokens[0] == "uci")
        {
            std::cout << "id name Aspen 2.3.0\n";
            std::cout << "id author ATheo\n";
            std::cout << "option name Hash type spin default 64 min 1 max 32768\n"; // Default hash table size is 64 megabytes
            //std::cout << "option name LMR_Base type spin default 100 min 25 max 175\n";
            std::cout << "uciok\n";
        }
        else if (tokens[0] == "ucinewgame")
        {
            // If there is an ongoing search we must cancel to continue with the new game
            searcher.uciStop = true;

            if (searcherThread.joinable()) searcherThread.join();

            globalMTG = 100;
            clearTranspositionTable();
            searcher.newGame();
        }
        // UCI options
        else if (tokens[0] == "setoption" && tokens.size() >= 5)
        {
            if (tokens[1] == "name")
            {
                // Resize TT size option
                if ((tokens[2] == "Hash" || tokens[2] == "hash") && tokens[3] == "value")
                {
                    int TTSizeMB = std::atoi(tokens[4].c_str());
                    resizeTranspositionTable(TTSizeMB);
                }
                // Set search parameter value
                //if ((tokens[2] == "LMR_Base") && tokens[3] == "value")
                //{
                //    int base = std::atoi(tokens[4].c_str());
                //    LMR_Base = base / 100.f;
                //}
            }
        }
        // Quit the program
        else if (tokens[0] == "quit")
        {
            searcher.uciStop = true;

            if (searcherThread.joinable()) searcherThread.join();

            break;
        }
        // Immediately stop the search
        else if (tokens[0] == "stop")
        {
            searcher.uciStop = true;

            if (searcherThread.joinable()) searcherThread.join();
        }
    }
}