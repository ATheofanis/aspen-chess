//
// Created by theoa on 07/03/2026.
//

#pragma once
#include <cassert>
#include <cstring>

#include "Attacks.h"
#include "Bitboard.h"
#include "Score.h"
#include "Zobrist.h"
#include "LegalMoveGen.h"


struct positionInfo
{
    Bitboard occupiedSquaresBitboard{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};

    ZobristHash zobrist{};

    int enPassantSquare{};
    int mgScore;
    int egScore;
    int gamePhase;

    int irreversiblePositionTop{};

    Piece pieceCaptured;
    Move prevMove{};
    CastlingRights castleRights{};
};


class Position
{
private:
    // Bitboards for board representation
    positionInfo prevPositionsLog[1024];


    Bitboard pieceBitboard[12]{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};
    Bitboard occupiedSquaresBitboard{};

    ZobristHash zobristHash{};
    ZobristHash previousPositions[1024]{}; // store all previous positions' zobrist

    Piece Board[64]{};


    int numOfPositions{}; // index used for the previousPositions array
    int irreversiblePositionTop{}; // index for last irreversible move (for 3-fold repetition check)

    int mgPstAndMaterialScore{};
    int egPstAndMaterialScore{};
    int gamePhase{};

    int enPassantSquare{};
    int positionLogTop;

    CastlingRights castleRights{};

    bool whiteToMoveFlag{};
    bool isCheckmate{};
    bool isStalemate{};

public:
    void clearPosition();

    // sets the game state to the starting position
    void setStartPos();

    void loadFen(const std::string& fenString);

    Position();

    // make move functions
    void makeMove(Move move); // any legal move
    void makeCapture(Move capture); // any legal capture

    void makeNullMove(int epSq) // null move for null move pruning
    {
        whiteToMoveFlag = !whiteToMoveFlag;
        zobristHash ^= zobristBlackToMove;
        enPassantSquare = NO_SQUARE;

        if (epSq != NO_SQUARE) zobristHash ^= zobristEnpassantFile[epSq % 8];

    }


    // unmake move functions
    void unmakeMove();
    void unmakeCapture();
    void unmakeNullMove(int epSq) // undo null move for null move pruning
    {
        whiteToMoveFlag = !whiteToMoveFlag;
        zobristHash ^= zobristBlackToMove;
        enPassantSquare = epSq;

        if (epSq != NO_SQUARE) zobristHash ^= zobristEnpassantFile[epSq % 8];
    }

    // return the bitboard of a given piece
    [[nodiscard]] constexpr Bitboard getPieceBitboard(int pieceIndex) const
    {
        return pieceBitboard[pieceIndex];
    }

    // returns a bitboard with the position of every white piece
    [[nodiscard]] constexpr Bitboard getWhiteBitboard() const
    {
        return whitePiecesBitboard;
    }

    // returns a bitboard with the position of every black piece
    [[nodiscard]] constexpr Bitboard getBlackBitboard() const
    {
        return blackPiecesBitboard;
    }

    // returns a bitboard of all occupied squares
    [[nodiscard]] constexpr Bitboard getOccupiedBitboard() const
    {
        return occupiedSquaresBitboard;
    }

    [[nodiscard]] constexpr ZobristHash getZobristHash() const
    {
        return zobristHash;
    }

    [[nodiscard]] constexpr int getNumOfMoves() const
    {
        return numOfPositions;
    }

    // returns game phase for testing
    [[nodiscard]] int getGamePhase() const { return gamePhase; }

    // returns the piece that is in square using the Board[64] mailbox
    [[nodiscard]] Piece getPieceFromBoard(int square) const { return Board[square]; }

    // true if it is whites turn to move
    [[nodiscard]] constexpr bool isWhiteToMove() const { return whiteToMoveFlag; }

    // returns castle rights
    [[nodiscard]] CastlingRights getCastlingRights() const { return castleRights; }

    // returns the square where enpassant can be played (otherwise returns NO_SQUARE)
    [[nodiscard]] int getEnpassantSquare() const { return enPassantSquare; }

    inline void calculatePstAndMaterialScore();

    constexpr int getMGPstAndMaterialScore() const { return mgPstAndMaterialScore; }
    constexpr int getEGPstAndMaterialScore() const { return egPstAndMaterialScore; }

    // return total PST and static eval using phases and mg and eg evaluations
    int getTotalPSTAndMaterialScore() const
    {
        int mgPhase = gamePhase;
        if (mgPhase > 24) mgPhase = 24; // in case of early promotion
        int egPhase = 24 - mgPhase;


        return (mgPstAndMaterialScore * mgPhase + egPstAndMaterialScore * egPhase) / 24;
    }

    // manual computation of zobrist hash for loading fen and for debugging incremental zobrist
    ZobristHash computeZobristHash();


    // check if current position has been repeated twice before (3-fold repetition rule)
    bool checkRepetition(ZobristHash currentPositionZobrist)
    {

        int repetitionsCount = 0;

        for (int i = numOfPositions - 1; i >= irreversiblePositionTop; i--)
        {
            if (currentPositionZobrist == previousPositions[i])
            {
                repetitionsCount++;
                if (repetitionsCount == 2)
                {
                    return true;
                }
            }
        }
        return false;
    }

    Bitboard getAllAttackersToSquare(int targetSquare) const;

    Bitboard getLeastValuablePiece(Bitboard pieces, int bySide, int &piece) const; // bySide is 0 for white 6 for black

    Bitboard xRayAttackersToSquare(int targetSquare, Bitboard occupancy) const;


    int SEE( int toSquare, int targetPiece, int fromSquare, int attackerPiece) const;

    // Checks if the side to move can capture the enemy king
    bool illegalBoardPosition()
    {
        int kingSquare;
        Color attackingSideColor;
        if (!whiteToMoveFlag)
        {
            kingSquare = lsbIndex(pieceBitboard[wK]);
            attackingSideColor = Black;
        } else
        {
            kingSquare = lsbIndex(pieceBitboard[bK]);
            attackingSideColor = White;
        }

        return squareUnderAttack(kingSquare, attackingSideColor, *this, occupiedSquaresBitboard);
    }


    bool sideToMoveIsInCheck()
    {
        int kingSquare;
        Color attackingSideColor;
        if (whiteToMoveFlag)
        {
            kingSquare = lsbIndex(pieceBitboard[wK]);
            attackingSideColor = Black;
        } else
        {
            kingSquare = lsbIndex(pieceBitboard[bK]);
            attackingSideColor = White;
        }

        return squareUnderAttack(kingSquare, attackingSideColor, *this, occupiedSquaresBitboard);
    }


    std::string boardToFen() const;

    bool isAttackedByPawn(int square, Color enemyColor) const
    {
        Piece enemyPawn = enemyColor == White ? wp : bp;
        Color allyColor = enemyColor == White ? Black : White;

        // Use pawn attacks precomputed table to quickly find the two diagonal squares behind the piece and check if they are occupied by a friendly pawn
        if (pawnAttacks[allyColor][square] & pieceBitboard[enemyPawn])
        {
            return true;
        }
        return false;
    }

    bool moveIsLegal(const legalityInformation& info, Move move) const
    {
        int fromSquare = getFromSquare(move);
        int toSquare = getToSquare(move);
        int moveFlag = getMoveFlag(move);
        Piece piece = Board[fromSquare];

        // If the moving piece is the white king
        if (piece == wK)
        {

            // Check the legality of white kingside castles
            if (moveFlag == 2)
            {
                // If any of the squares that the king passes through are under attack then the move is illegal
                // The squares for white kingside castles are : e1, f1, g1
                if (squareUnderAttack(e1, Black, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(f1, Black, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(g1, Black, *this, occupiedSquaresBitboard))
                {
                    return false;
                }
                return true;
            }
            // Check the legality of white queenside castles
            if (moveFlag == 3)
            {
                //If any of the squares that the king passes through are under attack then the move is illegal
                //The squares for white queenside castles are : e1, d1, c1
                if (squareUnderAttack(e1, Black, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(d1, Black, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(c1, Black, *this, occupiedSquaresBitboard))
                {
                    return false;
                }
                return true;
            }

            // For normal king moves we just need to check that the destination square is not under attack
            // We also need to pass a different occupancy than the current so that the old king's position is not taken into account,
            // which prevents edge cases where the old king's position counts as a blockade for the new position of the king
            return !(squareUnderAttack(toSquare, Black, *this, occupiedSquaresBitboard & ~(1ULL << fromSquare)));

        }

        // If the moving piece is the black king
        if (piece == bK)
        {
            // Check the legality of black kingside castles
            if (moveFlag == 2)
            {
                // If any of the squares that the king passes through are under attack then the move is illegal
                // The squares for black kingside castles are : e8, f8, g8
                if (squareUnderAttack(e8, White, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(f8, White, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(g8, White, *this, occupiedSquaresBitboard))
                {
                    return false;
                }
                return true;
            }
            // Check the legality of black queenside castles
            if (moveFlag == 3)
            {
                //If any of the squares that the king passes through are under attack then the move is illegal
                //The squares for black queenside castles are : e8, d8, c8
                if (squareUnderAttack(e8, White, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(d8, White, *this, occupiedSquaresBitboard) ||
                    squareUnderAttack(c8, White, *this, occupiedSquaresBitboard))
                {
                    return false;
                }
                return true;
            }
            // For normal king moves we just need to check that the destination square is not under attack
            // We also need to pass a different occupancy than the current so that the old king's position is not taken into account,
            // which prevents edge cases where the old king's position counts as a blockade for the new position of the king
            return !(squareUnderAttack(toSquare, White, *this, occupiedSquaresBitboard & ~(1ULL << fromSquare)));
        }

        if (info.numOfChecks >= 2) return false;

        // If the move is en-passant
        if (moveFlag == 5)
        {
            int epCapSq, kingSquare;
            Color enemyColor;
            if (piece == wp)
            {
                epCapSq = toSquare - 8;
                kingSquare = lsbIndex(pieceBitboard[wK]);
                enemyColor = Black;
            } else
            {
                epCapSq = toSquare + 8;
                kingSquare = lsbIndex(pieceBitboard[bK]);
                enemyColor = White;
            }

            Bitboard occupancyAfterEnpassant = occupiedSquaresBitboard & ~((1ULL << fromSquare) | (1ULL << epCapSq)) | (1ULL << toSquare);

            return !squareUnderAttack(kingSquare, enemyColor, *this, occupancyAfterEnpassant);

        }

        Bitboard toSquareMask = 1ULL << toSquare;
        Bitboard fromSquareMask = 1ULL << fromSquare;

        // Single check case
        if (info.numOfChecks == 1)
        {
            // If king in check then no pinned pieces can move otherwise they would expose the king
            if (fromSquareMask & info.pinned) return false;

            return ((toSquareMask & info.legalSquaresMask) != 0);
        }

        // If the piece to move is pinned
        if (info.pinned & fromSquareMask)
        {
            // If the destination square is a legal square for this specific pinned piece then the move is legal
            return ((info.pinnedPieceLegalSquares[fromSquare] & toSquareMask) != 0);
        }

        // If non of the above are true then the move is legal
        return true;
    }
};


