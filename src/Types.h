//
// Created by theoa on 06/03/2026.
//

// Holds the various types that the engine uses such as Bitboard, square, Color

#pragma once
#include <codecvt>
#include <cstdint>
#include <iostream>
#include <locale>
#include <unordered_map>
#include <chrono>

using Bitboard = uint64_t;
using ZobristHash = uint64_t;

constexpr int MAX_PLY = 30;


// board squares ENUM
enum Square : int {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,

    NO_SQUARE
};

// board squares to char for prints
constexpr const char* SquareNames[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

// helper to get the name of a square
inline const char* squareName(Square sq) {
    if (sq == NO_SQUARE) return "No Square";
    return SquareNames[sq];
}


inline int Q_DEPTH = 0;

enum ValueType : int
{
    VALUE_NONE
};


enum MoveType : int
{
    NO_MOVE
};

enum Piece : int {
//  0   1   2   3   4   5
    wp, wN, wB, wR, wQ, wK,
//  6   7   8   9   10  11
    bp, bN, bB, bR, bQ, bK,

    // 12
    NO_PIECE
};


// for fen string
const std::unordered_map<char, Piece> charToPiece = {
    {'P', wp}, {'N', wN}, {'B', wB}, {'R', wR}, {'Q', wQ}, {'K', wK},
    {'p', bp}, {'n', bN}, {'b', bB}, {'r', bR}, {'q', bQ}, {'k', bK}
};


// White = 0, Black = 1
enum Color {
    White, Black
};


enum { rook, bishop };

// represent moves with a 16-bit integer
// 6 first : from square
// next 6 : to square
// last 4 : move encoding flag
using Move = uint16_t;



using CastlingRights = uint8_t; // castling rights can be stored in a nibble (4 bits)
// white kingside
constexpr CastlingRights WK = 1;
// white queenside
constexpr CastlingRights WQ = 2;
// black kingside
constexpr CastlingRights BK = 4;
// black queenside
constexpr CastlingRights BQ = 8;


// for debugging
enum class UnicodePiece : char32_t {
    // white
    WhitePawn   = U'♟',  // U+265F
    WhiteKnight = U'♞',  // U+265E
    WhiteBishop = U'♝',  // U+265D
    WhiteRook   = U'♜',  // U+265C
    WhiteQueen  = U'♛',  // U+265B
    WhiteKing   = U'♚',  // U+265A


    // black
    BlackPawn   = U'♙',  // U+2659
    BlackKnight = U'♘',  // U+2658
    BlackBishop = U'♗',  // U+2657
    BlackRook   = U'♖',  // U+2656
    BlackQueen  = U'♕',  // U+2655
    BlackKing   = U'♔',  // U+2654
};



// find out if a piece is white based on its index
inline bool isWhite(Piece p)
{
    return (static_cast<int>(p) < 6);
}

// get the color of any piece
inline Color pieceColor(Piece p)
{
    if (static_cast<int>(p) < 6) return White;
    return Black;
}

// there are 3 types of nodes in the search tree, the root nodes, the principal variation nodes and the non principal variation nodes.
// each type is treated differently inside the search tree
enum class NodeType : int {
    PV, NonPV, Root
};


// enum type for transposition table hash entry
enum class Bound : uint8_t {
    // EXACT   , UPPERBOUND , LOWERBOUND
    BOUND_EXACT, BOUND_ALPHA, BOUND_BETA, BOUND_NONE

};