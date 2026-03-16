//
// Created by theoa on 07/03/2026.
//

#pragma once
#include "Position.h"

struct legalityInformation
{
    Bitboard pinners{};
    Bitboard pinned{};
    Bitboard checkers{};
    Bitboard legalSquaresMask{};
    Bitboard pinnedPieceLegalSquares[64] = {};
    int numOfChecks{};
};

legalityInformation getLegalityInfo(int kingSquare, Color allyColor, const Position& pos);

void generateWhitePawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves);
void generateBlackPawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves);

void generateKnightMoves(const legalityInformation& info, Bitboard knightPos, Bitboard allyPieces, Bitboard enemyPieces, Move moves[], int& numOfMoves);
void generateBishopMoves(const legalityInformation& info, Bitboard bishopPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateRookMoves(const legalityInformation& info, Bitboard rookPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateQueenMoves(const legalityInformation& info, Bitboard queenPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateKingMoves(const legalityInformation& info, Bitboard kingPos, Bitboard allyPieces, Bitboard enemyPieces, const Position& pos, Move moves[], int& numOfMoves);


void generateWhiteKingMoves(const Position& pos, Move moves[], int& numOfMoves);
void generateBlackKingMoves(const Position& pos, Move moves[], int& numOfMoves);


void generateLegalMoves(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves);


void generateCaptures(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves);
//inline void printUnicode(UnicodePiece uP) {
//    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
//    std::cout << conv.to_bytes(static_cast<char32_t>(uP));
//    std::cout << " ";
//}
//
//void printBoard(const Position& pos)
//{
//    for (int rank = 0; rank < 8; rank++)
//    {
//        for (int file = 0; file < 8; file++)
//        {
//            Piece piece = pos.getPieceFromBoard(rank * 8 + file);
//
//            // print ranks
//            if (!file)
//            {
//                std::cout << rank+1 << "   ";
//            }
//
//            if (piece == NO_PIECE)
//            {
//                std::cout << " ."; continue;
//            }
//            printUnicode(static_cast<UnicodePiece>(9823 - piece));
//        }
//
//        std::cout << "\n";
//    }
//    // print board files
//    std::cout << "    a  b c  d  e  f g  h\n" << std::endl;
//
//    // print the side to move
//    std::cout <<"Side to move: ";
//    if (pos.isWhiteToMove()) { std::cout << "White\n";}
//    else { std::cout << "Black\n";}
//
//    // print available castling rights
//    std::cout <<"Castling rights: ";
//    CastlingRights castlingRights = pos.getCastlingRights();
//    std::cout << (castlingRights & WK ? "WK / " : "- / ");
//    std::cout << (castlingRights & WQ ? "WQ / " : "- / ");
//    std::cout << (castlingRights & BK ? "BK / " : "- / ");
//    std::cout << (castlingRights & BQ ? "BQ  " : "-");
//
//    // print en passant square
//    std::cout << "\nEn Passant square: ";
//    std::string epSqName = squareName((Square)pos.getEnpassantSquare());
//    std::cout << epSqName;
//    std::cout << "\n";
//    std::cout << "\n";
//}

inline void printMove(const Move move)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;
    std::cout << "\nMove: Starting Square : " << squareName(static_cast<Square>(fromSquare)) << "\nEnding square: " << squareName(static_cast<Square>(toSquare)) << "Flag: " << flag << "\n";
}