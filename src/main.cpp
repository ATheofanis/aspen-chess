#include <cassert>
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
#include <fstream>

#include "DataGen.h"
#include "Time.h"

class Position;

// print the Unicode character
inline void printUnicode(UnicodePiece uP) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::cout << conv.to_bytes(static_cast<char32_t>(uP));
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
    int flag = (move >> 12) & 0x3F;
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


void findBestMoveTime(Position pos)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    int evalDummy;
    Move bestMove = (findBestMove(pos, evalDummy));
    printMove(bestMove);
    std::cout << "RAW BEST MOVE:" << bestMove << std::endl;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << duration.count() << " ms" << std::endl;
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
        int flag = (currMove >> 12) & 0x3F;
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

inline void initializations()
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




std::string startPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
std::string veryTrickyCapturesPos = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";

bool UCI_ENABLED = true;

int main(int argc, char* argv[])
{
    initializations();



    // Start the UCI loop if UCI is enabled
    if (UCI_ENABLED)
    {
        std::string command;
        Position pos;


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

                    TimePoint startingTime = now();
                    MAX_NODES = dataGenMaxNodes;
                    DataGenFlag = true;
                    generateData();
                    TimePoint totalTime = now() - startingTime;
                    std::cout << "Data generation finished. Generated " << NumberOfGames << "games in " << totalTime << " ms." << std::endl;
                }
            }
            // position command
            else if (tokens[0] == "position")
            {
                // non fen string ---------
                if (tokens.size() >= 2 && tokens[1] == "startpos")
                {
                    pos.loadFen(startPos);

                    if (tokens[2] == "moves")
                    {
                        pos = parsePosition(tokens, 3);
                    }

                    // non fen string --------
                }

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
                            movesTokenIndex = i;
                            movesFlag = true;
                            break;
                        }
                        fenString += tokens[i];
                        fenString += ' ';
                    }

                    pos.loadFen(fenString);
                    if (movesFlag)
                    {
                        parsePosition(tokens, movesTokenIndex);
                    }

                    // fen string --------------
                }

            } else if (tokens[0] == "go")
            {
                MAX_NODES = std::numeric_limits<uint64_t>::max();
                TimePoint wtime = 50000, btime = 50000, winc = 0, binc = 0;
                int movestogo = 25;

                for (int i = 1; i < tokens.size(); i++) {
                    if (tokens[i] == "wtime") // white remaining time
                    {
                        wtime = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "btime") // black remaining time
                    {
                        btime = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "winc") // white increment
                    {
                        winc = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "binc") // black increment
                    {
                        binc = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "movestogo") // moves to go passed by the GUI
                    {
                        movestogo = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "depth") // change the maximum depth if the depth command is passed
                    {
                        MAX_DEPTH = std::atoi(tokens[i+1].c_str());
                    }
                    else if (tokens[i] == "nodes") // set the maximum amount of nodes that can be searched
                    {
                        MAX_NODES = std::atoi(tokens[i+1].c_str());
                    }
                }

                // start the time manager if wtime or btime were given
                if (wtime || btime) {
                    TimePoint myTime = (pos.isWhiteToMove()) ? wtime : btime;
                    TimePoint myInc = (pos.isWhiteToMove()) ? winc : binc;
                    tm.start(myTime, myInc, movestogo); // initialize the time manager for the side to move
                }

                // PRINT BEST MOVE ------------------======================================================
                int evalDummy;
                Move bestMove = findBestMove(pos, evalDummy);

                std::cout << "bestmove ";
                printMoveNoMessages(bestMove);
                std::cout << std::endl;
                // PRINT BEST MOVE ------------------======================================================

            }
            else if (tokens[0] == "isready")
            {
                std::cout << "readyok\n" << std::endl;
            } else if (tokens[0] == "uci")
            {
                std::cout << "id name Aspen 0.1.0\n";
                std::cout << "id author ATheo\n";
                std::cout << "uciok\n";
            } else if (tokens[0] == "ucinewgame")
            {
                clearTranspositionTable();
                clearPawnTranspositionTable();
            }
            // quit command
            else if (tokens[0] == "quit")
            {
                break;
            }
        }
    }

    return 0;

}