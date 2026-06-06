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
    int halfMoveCounter{};

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

    int halfMoveCounter{};
    int fullMoveCounter{};

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

    [[nodiscard]] constexpr bool getSideToMove() const { return whiteToMoveFlag ? White : Black; }

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
    bool repetition()
    {

        int repetitionsCount = 0;

        for (int i = numOfPositions - 1; i >= irreversiblePositionTop; i--)
        {
            if (zobristHash == previousPositions[i])
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

    bool isDraw()
    {
        // TODO: Generate king moves to see if checkmate occured right as the 50 move rule was about to happen, for now > instead of >= to account for it
        if (halfMoveCounter > 100) return true;
        //{
        //    Move kingMoves[16];
        //    generateKingMoves()
        //}

        if (repetition()) return true;

        /*
        // Dont check for draw if there are major pieces or pawns on the board
        if (pieceBitboard[wp] || pieceBitboard[bp] || pieceBitboard[bR] || pieceBitboard[wR] || pieceBitboard[wQ] || pieceBitboard[bQ]) return false;

        // Draw if there are 1 or less minor pieces on both sides
        int whiteKnights = bitCount(pieceBitboard[wN]);
        int whiteBishops = bitCount(pieceBitboard[wB]);
        int whiteMinorPieces = whiteKnights + whiteBishops;

        int blackKnights = bitCount(pieceBitboard[bN]);
        int blackBishops = bitCount(pieceBitboard[bB]);
        int blackMinorPieces = blackKnights + blackBishops;

        if (whiteMinorPieces < 2 && blackMinorPieces < 2) return true;
        */

        return false;
    }

    Bitboard getAllAttackersToSquare(int targetSquare) const;

    Bitboard getLeastValuablePiece(Bitboard pieces, int bySide, int &piece) const; // bySide is 0 for white 6 for black

    Bitboard xRayAttackersToSquare(int targetSquare, Bitboard occupancy) const;


    int SEE( int toSquare, int targetPiece, int fromSquare, int attackerPiece) const;

    // Checks if the side to move can capture the enemy king
    bool enemyKingInCheck()
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

    // Checks if a pseudo-legal move is legal
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

    // Checks if a given move is physically possible for the current position
    bool moveIsValid(Move move) const
    {
        int fromSquare = getFromSquare(move);
        int toSquare = getToSquare(move);
        int moveFlag = getMoveFlag(move);

        Bitboard toMask = 1ULL << toSquare;
        Bitboard fromMask = 1ULL << fromSquare;

        Piece movingPiece = Board[fromSquare];

        // If the starting square is empty or if the piece moving is not the same color as the side to move then the move is invalid
        if (movingPiece == 12) return false;
        if ((movingPiece < 6 && !whiteToMoveFlag) || (movingPiece >= 6 && whiteToMoveFlag)) return false;

        // Castling
        if (moveFlag == 2 || moveFlag == 3)
        {
            // Only kings can castle
            if (movingPiece != wK && movingPiece != bK)
            {
                return false;
            }

            // White king castling
            if (movingPiece == wK)
            {
                // King side
                if (moveFlag == 2) return (fromSquare == 4 && toSquare == 6) && (castleRights & WK) && ((occupiedSquaresBitboard & 96) == 0);

                // Queen side
                return (fromSquare == 4 && toSquare == 2) && (castleRights & WQ) && ((occupiedSquaresBitboard & 14) == 0);
            }
            else
            {
                // Black king side
                if (moveFlag == 2) return (fromSquare == 60 && toSquare == 62) && (castleRights & BK) && ((occupiedSquaresBitboard & 0x6000000000000000) == 0);

                // Black queen side
                return (fromSquare == 60 && toSquare == 58) && (castleRights & BQ) && ((occupiedSquaresBitboard & 0x0E00000000000000) == 0);
            }
            return false;
        }


        bool isCap = isCapture(moveFlag);
        bool enemyOccupied = toMask & (whiteToMoveFlag ? blackPiecesBitboard : whitePiecesBitboard);

        // Invalid capture
        if (isCap && !enemyOccupied && moveFlag != 5) return false;

        // Invalid quiet move if destination square is occupied
        if (!isCap && enemyOccupied) return false;

        Bitboard allyPieces = movingPiece < 6 ? whitePiecesBitboard : blackPiecesBitboard;

        // The destination square must be completely empty
        if (toMask & allyPieces) return false;

        // White pawn move
        if (movingPiece == wp)
        {
            // Promotions only on 7th rank
            if (moveFlag & 8 && fromSquare / 8 != 6) return false;
            if (moveFlag == 5) return (toSquare == enPassantSquare) && (toSquare == fromSquare + 7 || toSquare == fromSquare + 9);
            if (isCap) return (toMask & blackPiecesBitboard) && (toSquare == fromSquare + 7 || toSquare == fromSquare + 9);
            if (moveFlag == 1) return (toSquare == fromSquare + 16) && !(toMask & occupiedSquaresBitboard) && !((1ULL << (fromSquare + 8)) & occupiedSquaresBitboard) && (fromSquare / 8 == 1);
            return (toSquare == fromSquare + 8) && !(toMask & occupiedSquaresBitboard);
        }
        else if (movingPiece == bp)
        {
            if (moveFlag & 8 && fromSquare / 8 != 1) return false;
            if (moveFlag == 5) return (toSquare == enPassantSquare) && (toSquare == fromSquare - 7 || toSquare == fromSquare - 9);
            if (isCap) return (toMask & whitePiecesBitboard) && (toSquare == fromSquare - 7 || toSquare == fromSquare - 9);
            if (moveFlag == 1) return (toSquare == fromSquare - 16) && !(toMask & occupiedSquaresBitboard) && !((1ULL << (fromSquare - 8)) & occupiedSquaresBitboard) && (fromSquare / 8 == 6);
            return (toSquare == fromSquare - 8) && !(toMask & occupiedSquaresBitboard);
        }

        // Ensure non-pawns do not have pawn flags (double-push, en-passant & promotions)
        if (moveFlag == 1 || moveFlag == 5 || moveFlag & 8) return false;

        // Non-pawn piece
        if (movingPiece == wN || movingPiece == bN) return (knightAttacks[fromSquare] & toMask);
        if (movingPiece == wB || movingPiece == bB) return (getBishopAttacks(fromSquare, occupiedSquaresBitboard) & toMask);
        if (movingPiece == wR || movingPiece == bR) return (getRookAttacks(fromSquare, occupiedSquaresBitboard) & toMask);
        if (movingPiece == wQ || movingPiece == bQ) return ((getBishopAttacks(fromSquare, occupiedSquaresBitboard) | getRookAttacks(fromSquare, occupiedSquaresBitboard)) & toMask);
        // Normal king moves
        if (movingPiece == wK || movingPiece == bK) return (kingAttacks[fromSquare] & toMask);

        return false;
    }


};