#include <cassert>
#include <iostream>

#include "Attacks.h"
#include "Bitboard.h"

#include <windows.h>

#include "MoveGen.h"
#include "Position.h"

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

        if (squareUnderAttack(pos.isWhiteToMove() ? lsbIndex(pos.getPieceBitboard(bK)) : lsbIndex(pos.getPieceBitboard(wK)), pos.isWhiteToMove() ? White : Black, pos, pos.getOccupiedBitboard()))
        {
            std::cout << "ATTACKED ERRORRRRRRR\n";
            std::cout << "\n PRINTING BLACK BITBOARD" << std::endl;
            printBitboard(pos.getBlackBitboard());
            std::cout << "\n PRINTING WHITE BITBOARD" << std::endl;
            printBitboard(pos.getWhiteBitboard());
            std::cout << "\n PRINTING OCCUPIED BITBOARD" << std::endl;
            printBitboard(pos.getOccupiedBitboard());
            std::cout << "\n PRINTING WHITE PAWN BITBOARD" << std::endl;
            printBitboard(pos.getPieceBitboard(wp));
            std::cout << "\n PRINTING BLACK PAWN BITBOARD" << std::endl;
            printBitboard(pos.getPieceBitboard(bp));
            printBitboard(pos.getPieceBitboard(wK));
            std::cout << "\n PRINTING THE BOARD: " << std::endl;
            printBoard(pos);
            break;
        }

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

int depth = 4;
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    initPawnAttacks();
    initKnightAttacks();
    initKingAttacks();
    initRookAttacks();
    initBishopAttacks();
    initLineBetween();

    //initMagicNumbers();

    Position pos;

    pos.loadFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");


    dividePerft(pos, depth);





}