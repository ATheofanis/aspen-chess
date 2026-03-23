#include <cassert>
#include <iostream>


#include "Attacks.h"
#include "Bitboard.h"

#include <windows.h>

#include "MoveGen.h"
#include "Position.h"
#include "PRNG.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

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
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main()
{
    clearTranspositionTable();
    std::cout << "DEPTH : " << MAX_DEPTH << std::endl;

    std::string startPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string veryTrickyCapturesPos = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";

    // initializations
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
    // initializations


    //initMagicNumbers();

    Position pos;

    //Q7/ppp2k1p/3p2p1/5b2/4P1nq/2P4P/PP1P1bP1/RNB2R1K b - - 0 1
    pos.loadFen(    veryTrickyCapturesPos    );

    //pos.loadFen(    startPos    );

    //int eval = pos.getTotalPSTAndMaterialScore();
    //std::cout << "eval:" << eval << std::endl;
    //Move moves[256];
    //int numOfMoves = 0;


    //Color allyColor;
    //int kingSquare;
    //// store ally color and king location for legality info
    //if (pos.isWhiteToMove())
    //{
    //    allyColor = White;
    //    kingSquare = lsbIndex(pos.getPieceBitboard(wK));
    //}
    //else
    //{
    //    allyColor = Black;
    //    kingSquare = lsbIndex(pos.getPieceBitboard(bK));
    //}
    //legalityInformation info = getLegalityInfo(kingSquare, allyColor, pos);
//
//
    //// generate legal moves using previously calculated legality info
    //generateCaptures(info, pos, moves, numOfMoves);
//
    //for (int i =0 ; i < numOfMoves; i++)
    //{
    //    printMove(moves[i]);
    //}


    //std::cout << (WQ >> 1);
    //std::cout << (BK >> 2);
    //std::cout << (BQ >> 3);
//
    //std::cout << pos.getTotalPSTAndMaterialScore() << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();
    Move bestMove = (findBestMove(pos));
    printMove(bestMove);
    std::cout << "RAW BEST MOVE:" << bestMove << std::endl;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << duration.count() << " ms" << std::endl;
//



    //pos.makeMove(bestMove);
    //bestMove = (findBestMove(pos));
    //printMove(bestMove);
    //std::cout << "RAW BEST MOVE:" << bestMove << std::endl;
//
//
//


    //Move bestMove = 0;
    //save(25, 10, 100, Bound::BOUND_BETA, 1234);
    //int eval = probe(6, 25, 120, 110, bestMove);
    //std::cout << eval << std::endl;

}