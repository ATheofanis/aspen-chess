//
// Created by theoa on 07/03/2026.
//

#include "Position.h"

#include <cassert>

#include "MoveGen.h"
#include "Zobrist.h"

// clears everything regarding a position
void Position::clearPosition()
{
    for (auto & sq : Board)
    {
        sq = NO_PIECE;
    }
    for (auto & piece : pieceBitboard) {
        piece = 0ULL;
    }
    blackPiecesBitboard = 0ULL;
    whitePiecesBitboard = 0ULL;
    occupiedSquaresBitboard = 0ULL;

    whiteToMoveFlag = true;
    enPassantSquare = NO_SQUARE;
    castleRights = 0;

}


// set board to the starting position
void Position::setStartPos()
{
    numOfPositions = 0;
    irreversiblePositionTop = 0;
    positionLogTop = 0;
    whiteToMoveFlag = true;
    isCheckmate = false;
    isStalemate = false;
    enPassantSquare = NO_SQUARE;
    castleRights = 15;
    egPstAndMaterialScore = 0;
    mgPstAndMaterialScore = 0;
    gamePhase = 24;


    // Bitboard pieces[12]; // wp, wN, wB, wR, wQ, wK - bp, bN, bB, bR, bQ, bK
    pieceBitboard[0] = 0x000000000000FF00ULL; // wp

    // wN: 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0010 =
    // 0x    0   0    0     0    0    0    0    0    0    0    0    0    0    0    4    2
    pieceBitboard[1] = 0x0000000000000042ULL; // wN

    // wB: 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0010 0100 =
    // 0x    0   0    0     0    0    0    0    0    0    0    0    0    0    0    2    4
    pieceBitboard[2] = 0x0000000000000024ULL; // wB

    pieceBitboard[3] = 0x0000000000000081ULL; // wR

    pieceBitboard[4] = 0x0000000000000008ULL; // wQ

    pieceBitboard[5] = 0x0000000000000010ULL; // wK

    // Black pieces
    pieceBitboard[6]  = 0x00FF000000000000ULL; // bP (shift all white pawns 40 (squares) bits to go from rank 2 to rank 7)

    // for the rest of black pieces shift 56 bits
    pieceBitboard[7]  = 0x4200000000000000ULL; // bN

    pieceBitboard[8]  = 0x2400000000000000ULL; // bB

    pieceBitboard[9]  = 0x8100000000000000ULL; // bR

    pieceBitboard[10] = 0x0800000000000000ULL; // bQ

    pieceBitboard[11] = 0x1000000000000000ULL; // bK


    whitePiecesBitboard = pieceBitboard[0] | pieceBitboard[1] | pieceBitboard[2] |
        pieceBitboard[3] | pieceBitboard[4] | pieceBitboard[5];

    blackPiecesBitboard = pieceBitboard[6] | pieceBitboard[7] | pieceBitboard[8] |
        pieceBitboard[9] | pieceBitboard[10] | pieceBitboard[11];

    occupiedSquaresBitboard = whitePiecesBitboard | blackPiecesBitboard;

    for (int sq = 0; sq < 64; ++sq)
    {
        Board[sq] = NO_PIECE;

        for (int i = 0; i < 12; ++i)
        {
            if (pieceBitboard[i] & (1ULL << sq))
            {
                Board[sq] = static_cast<Piece>(i);
                break;
            }
        }
    }
    zobristHash = computeZobristHash();
    pawnZobristHash = computePawnZobristHash();
}

Position::Position()
{
    setStartPos();
}

// take a fen string and set the position accordingly
void Position::loadFen(const std::string& fenString)
{
    clearPosition();

    int rank = 7; // start from the top (index 7)
    int file = 0;

    int charIndex = -1;

    for (const char& c : fenString)
    {
        charIndex++;
        if (c == ' ') break;

        if (c == '/') {
            rank--; // go down to the next rank
            file = 0; // start from the beginning of the rank
            continue;
        }
        // if character is a digit then skip that many squares ahead
        if (c >= '1' && c <= '8')
        {
            file += c - '0';
        }
        // if it is a piece
        if (charToPiece.count(c))
        {
            int sq = rank * 8 + file;
            Bitboard sqMask = 1ULL << sq;

            // get the piece type and add it to the board
            Piece piece = charToPiece.at(c);
            Board[sq] = piece;

            // and adjust bitboards
            pieceBitboard[piece] |= sqMask;
            occupiedSquaresBitboard |= sqMask;

            // if piece is white
            if (static_cast<int>(piece) < 6)
            {
                whitePiecesBitboard |= sqMask;
            } else
            {
                blackPiecesBitboard |= sqMask;
            }
            file++;
        }
    }
    charIndex++;
    // extract side to move
    if (fenString[charIndex] == 'w')
    {
        whiteToMoveFlag = true;
    } else
    {
        whiteToMoveFlag = false;
    }

    // extract castling rights
    charIndex += 2;
    while (fenString[charIndex] == '-')
    {
        charIndex++;
    }

    // loop until we hit a space to extract castling rights
    while (fenString[charIndex] != ' ')
    {
        if (fenString[charIndex] == 'K')
        {
            castleRights |= WK;
        } else if (fenString[charIndex] == 'Q')
        {
            castleRights |= WQ;
        } else if (fenString[charIndex] == 'k')
        {
            castleRights |= BK;
        } else if (fenString[charIndex] == 'q')
        {
            castleRights |= BQ;
        }
        charIndex++;
    }
    charIndex++;
    if (fenString[charIndex] == '-')
    {
        charIndex += 2;
        enPassantSquare = NO_SQUARE;
    } else
    {
        int epFile = fenString[charIndex++] - 'a';
        int epRank = (fenString[charIndex]) - '1';

        enPassantSquare = epRank * 8 + epFile;
        charIndex++;
    }

    int halfMoveClock;
    int fullMoveCounter;
    sscanf(fenString.c_str() + charIndex, "%d %d", &halfMoveClock, &fullMoveCounter);


    calculatePstAndMaterialScore();
    zobristHash = computeZobristHash();
    pawnZobristHash = computePawnZobristHash();

    numOfPositions = 0;
    irreversiblePositionTop = 0;
    previousPositions[numOfPositions++] = zobristHash;
}


void Position::makeMove(Move move)
{

    positionInfo prevPosInfo;
    prevPosInfo.prevMove = move;
    prevPosInfo.blackPiecesBitboard = blackPiecesBitboard;
    prevPosInfo.whitePiecesBitboard = whitePiecesBitboard;

    prevPosInfo.zobrist = zobristHash;
    prevPosInfo.pawnZobrist = pawnZobristHash;

    prevPosInfo.pieceCaptured = NO_PIECE;
    prevPosInfo.enPassantSquare = enPassantSquare;
    prevPosInfo.castleRights = castleRights;

    prevPosInfo.mgScore = mgPstAndMaterialScore;
    prevPosInfo.egScore = egPstAndMaterialScore;
    prevPosInfo.gamePhase = gamePhase;

    prevPosInfo.irreversiblePositionTop = irreversiblePositionTop;

    CastlingRights oldCastleRights = castleRights;

    // extract move info
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;


    // create square masks for later
    Bitboard fromMask = 1ULL << fromSquare;
    Bitboard toMask = 1ULL << toSquare;
    Bitboard moveMask = fromMask | toMask;

    Piece pieceMoved = Board[fromSquare];
    bool pieceMovedIsPawn = (pieceMoved == 6) || (pieceMoved == 0);
    if (pieceMovedIsPawn)
    {
        irreversiblePositionTop = numOfPositions;
        pawnZobristHash ^= zobristPieces[fromSquare][pieceMoved];
    }


    // update board-piece zobrist value
    zobristHash ^= zobristPieces[fromSquare][pieceMoved];

    // update white or black piece bitboard for the piece that moved
    int colorDelta; // we add this to easily calculate the piece index without checking whose turn it is down the line
    if (whiteToMoveFlag)
    {
        colorDelta = 0;
        whitePiecesBitboard ^= moveMask;

        // for WK : if the from square was from the white kingside rook's starting position then castling rights for WK sohuld be disabled, same logic for queenside


        // if enemy rook was captured update black's castle rights, if own rook moved update the corresponding castle side right
        // if a move was made that corresponds to one of the 4 rook square masks then that means we need to update a castling right, that happens by
        // shifting the bit that becomes 1 after the bitwise OR operation x amount of times to fit the castle rights 4bit encoding
        // then to properly update the castling rights just do a bitwise AND NOT operation with the current castle rights
        castleRights &= ~ ((((fromMask & wkRookMask) >> 7) | ((fromMask & wqRookMask) << 1)) | ((toMask & bkRookMask) >> 61) | ((toMask & bqRookMask) >> 53));
        if ((fromMask & e1Mask))
        {
            castleRights &= 12;
        }

        // adjust incremental score from the piece that moved by subtracting the score from its original square and adding the score from the new square (promotions handled differently)
        mgPstAndMaterialScore -= mgTable[pieceMoved][fromSquare];
        egPstAndMaterialScore -= egTable[pieceMoved][fromSquare];
        if (!(flag & 8))
        {
            if (pieceMovedIsPawn) pawnZobristHash ^= zobristPieces[toSquare][wp];
            mgPstAndMaterialScore += mgTable[pieceMoved][toSquare];
            egPstAndMaterialScore += egTable[pieceMoved][toSquare];
        }
    }
    else
    {
        colorDelta = 6;
        blackPiecesBitboard ^= moveMask;

        castleRights &= ~ ((((fromMask & bkRookMask) >> 61) | ((fromMask & bqRookMask) >> 53)) | ((toMask & wkRookMask) >> 7) | ((toMask & wqRookMask) << 1));
        if ((fromMask & e8Mask))
        {
            castleRights &= 3;
        }

        mgPstAndMaterialScore += mgTable[pieceMoved][fromSquare];
        egPstAndMaterialScore += egTable[pieceMoved][fromSquare];
        if (!(flag & 8))
        {
            if (pieceMovedIsPawn) pawnZobristHash ^= zobristPieces[toSquare][bp];
            mgPstAndMaterialScore -= mgTable[pieceMoved][toSquare];
            egPstAndMaterialScore -= egTable[pieceMoved][toSquare];
        }
    }


    // only XOR for zobrist enpassant file key if en passant square is valid
    if (enPassantSquare != NO_SQUARE)
    {
        zobristHash ^= zobristEnpassantFile[enPassantSquare % 8];
        enPassantSquare = NO_SQUARE;
    }



    // update mailbox ( remove piece moved from its starting position )

    Board[fromSquare] = NO_PIECE;

    // quiet moves are the most common with flag == 0 so adding this check will prevent a lot of necessary checks down the line
    // without this a quiet move will have to go through all the below checks for no reason
    if (flag)
    {
        // handle captures
        if (flag == 4)
        {
            irreversiblePositionTop = numOfPositions;

            Piece capPiece = Board[toSquare];
            if (capPiece % 6 == 0)
            {
                pawnZobristHash ^= zobristPieces[toSquare][capPiece];

            }
            gamePhase -= gamephaseInc[capPiece];

            prevPosInfo.pieceCaptured = capPiece; // store cap piece

            // update bitboards of moved and captured pieces
            pieceBitboard[capPiece] &= ~toMask;
            pieceBitboard[pieceMoved] ^= moveMask;

            // update mailbox
            Board[toSquare] = pieceMoved;

            // update zobrist, xor captured piece in to square
            zobristHash ^= zobristPieces[toSquare][capPiece];

            // update white/black/occupied (remove captured piece)
            if (whiteToMoveFlag)
            {
                mgPstAndMaterialScore += mgTable[capPiece][toSquare];
                egPstAndMaterialScore += egTable[capPiece][toSquare];

                blackPiecesBitboard &= ~toMask;
            } else
            {
                mgPstAndMaterialScore -= mgTable[capPiece][toSquare];
                egPstAndMaterialScore -= egTable[capPiece][toSquare];

                whitePiecesBitboard &= ~toMask;
            }

            zobristHash ^= zobristPieces[toSquare][pieceMoved];

        }
        // en passant
        else if (flag == 5)
        {
            //Piece capPawn;
            if (whiteToMoveFlag)
            {
                prevPosInfo.pieceCaptured = bp;
                int epCapSq = toSquare - 8;
                // remove captured pawn from bp bitboard and Board mailbox:
                Bitboard capPawnMask = 1ULL << (epCapSq);
                pieceBitboard[bp] &= ~capPawnMask;
                pieceBitboard[wp] ^= moveMask;
                Board[epCapSq] = NO_PIECE;
                Board[toSquare] = wp;
                // also remove from black piece bb and occupied squares bb
                blackPiecesBitboard &= ~capPawnMask;

                mgPstAndMaterialScore += mgTable[bp][epCapSq];
                egPstAndMaterialScore += egTable[bp][epCapSq];
                gamePhase -= gamephaseInc[bp];

                // update zobrist by removing ep relevant black pawn
                pawnZobristHash ^= zobristPieces[epCapSq][bp];
                zobristHash ^= zobristPieces[epCapSq][bp];
                zobristHash ^= zobristPieces[toSquare][wp];

            } else
            {
                // black makes ep
                prevPosInfo.pieceCaptured = wp;
                int epCapSq = toSquare + 8;
                // remove captured pawn from bp bitboard and Board mailbox:
                Bitboard capPawnMask = 1ULL << (epCapSq);
                pieceBitboard[wp] &= ~capPawnMask;
                pieceBitboard[bp] ^= moveMask;
                Board[epCapSq] = NO_PIECE;
                Board[toSquare] = bp;
                // also remove from white piece bb and occupied squares bb
                whitePiecesBitboard &= ~capPawnMask;

                mgPstAndMaterialScore -= mgTable[wp][epCapSq];
                egPstAndMaterialScore -= egTable[wp][epCapSq];
                gamePhase -= gamephaseInc[wp];

                pawnZobristHash ^= zobristPieces[epCapSq][wp];
                zobristHash ^= zobristPieces[epCapSq][wp];
                zobristHash ^= zobristPieces[toSquare][bp];
            }

        }
        // captures are done , now handle promotions and promo-captures
        else if (flag & 8)
        {
            // add promoted piece to mailbox
            // (colorDelta + flag % 4 + 1) gives the index of promoted piece based on the 16 bit move encoding i am using
            Piece promotedPiece = static_cast<Piece>(colorDelta + flag % 4 + 1);


            if (flag & 4) // if move is promo-capture
            {
                Piece capturedPiece = Board[toSquare];
                prevPosInfo.pieceCaptured = capturedPiece;

                pieceBitboard[capturedPiece] &= ~toMask;
                gamePhase -= gamephaseInc[capturedPiece];

                // remove captured piece with xor
                zobristHash ^= zobristPieces[toSquare][capturedPiece];

                if (whiteToMoveFlag)
                {
                    mgPstAndMaterialScore += mgTable[capturedPiece][toSquare];
                    egPstAndMaterialScore += egTable[capturedPiece][toSquare];


                    blackPiecesBitboard &= ~toMask;
                } else
                {
                    mgPstAndMaterialScore -= mgTable[capturedPiece][toSquare];
                    egPstAndMaterialScore -= egTable[capturedPiece][toSquare];

                    whitePiecesBitboard &= ~toMask;
                }
            }

            Board[toSquare] = promotedPiece; // add promoted piece to the board
            pieceBitboard[promotedPiece] |= toMask; // add promoted piece to its bitboard
            pieceBitboard[pieceMoved] &= ~fromMask;

            // update zobrist with promoted piece
            zobristHash ^= zobristPieces[toSquare][promotedPiece];

            gamePhase += (gamephaseInc[promotedPiece] - gamephaseInc[0]);
            if (whiteToMoveFlag)
            {
                mgPstAndMaterialScore += mgTable[promotedPiece][toSquare];
                egPstAndMaterialScore += egTable[promotedPiece][toSquare];
            } else
            {
                mgPstAndMaterialScore -= mgTable[promotedPiece][toSquare];
                egPstAndMaterialScore -= egTable[promotedPiece][toSquare];
            }
        }

        else if (flag == 1)
        {
            pieceBitboard[pieceMoved] ^= moveMask;
            Board[toSquare] = pieceMoved;

            int newEpSq = whiteToMoveFlag ? (toSquare-8) : (toSquare + 8);
            enPassantSquare = newEpSq;

            // add corresponding ep file to zobrist
            zobristHash ^= zobristEnpassantFile[newEpSq % 8];
            zobristHash ^= zobristPieces[toSquare][pieceMoved];
        }
        else
        {
            irreversiblePositionTop = numOfPositions;
            if (whiteToMoveFlag) // White castles
            {

                int rookStartSq = 7; // initialize with white kingside rook squares
                int rookEndSq = 5; // initialize with white kingside rook squares

                if (flag == 3) // White queenside
                {
                    rookStartSq = 0; // a1
                    rookEndSq = 3; // d1

                }

                Bitboard rookMoveMask = (1ULL << rookStartSq) | (1ULL << rookEndSq);

                pieceBitboard[wR] ^= rookMoveMask;
                pieceBitboard[wK] ^= moveMask;
                Board[rookStartSq] = NO_PIECE;
                Board[rookEndSq] = wR;
                Board[toSquare] = wK;
                whitePiecesBitboard ^= rookMoveMask;

                // add score from rook's new position
                mgPstAndMaterialScore += mgTable[wR][rookEndSq];
                egPstAndMaterialScore += egTable[wR][rookEndSq];
                // remove score from rook's previous position
                mgPstAndMaterialScore -= mgTable[wR][rookStartSq];
                egPstAndMaterialScore -= egTable[wR][rookStartSq];


                zobristHash ^= zobristPieces[rookStartSq][wR];
                zobristHash ^= zobristPieces[rookEndSq][wR];
                zobristHash ^= zobristPieces[toSquare][wK];

            }

            else // Black castles
            {
                int rookStartSq = 63; // initialize with black kingside rook squares
                int rookEndSq = 61; // initialize with black kingside rook squares

                if (flag == 3) // Black queenside
                {
                    rookStartSq = 56; // a8
                    rookEndSq = 59; // d8
                }

                Bitboard rookMoveMask = (1ULL << rookStartSq) | (1ULL << rookEndSq);

                pieceBitboard[bR] ^= rookMoveMask;
                pieceBitboard[bK] ^= moveMask;
                Board[rookStartSq] = NO_PIECE;
                Board[rookEndSq] = bR;
                Board[toSquare] = bK;
                blackPiecesBitboard ^= rookMoveMask;

                // add score from rook's new position
                mgPstAndMaterialScore -= mgTable[bR][rookEndSq];
                egPstAndMaterialScore -= egTable[bR][rookEndSq];
                // remove score from rook's previous position
                mgPstAndMaterialScore += mgTable[bR][rookStartSq];
                egPstAndMaterialScore += egTable[bR][rookStartSq];

                zobristHash ^= zobristPieces[rookStartSq][bR];
                zobristHash ^= zobristPieces[rookEndSq][bR];
                zobristHash ^= zobristPieces[toSquare][bK];

            }
        }

    } // if flag 0, quiet move
    else // quiet move : XOR moveMask with the piece that moved and update white/black bb and occupied bb
    {
        pieceBitboard[pieceMoved] ^= moveMask;
        Board[toSquare] = pieceMoved;
        zobristHash ^= zobristPieces[toSquare][pieceMoved];
    }

    zobristHash ^= zobristBlackToMove;

    whiteToMoveFlag = !whiteToMoveFlag;
    prevPositionsLog[positionLogTop++] = prevPosInfo;



    occupiedSquaresBitboard = whitePiecesBitboard | blackPiecesBitboard;

    // update zobrist castling rights , we only xor one castle right if the old differs from the new
    CastlingRights updatedRights = oldCastleRights ^ castleRights;
    if (updatedRights & WK)
    {
        zobristHash ^= zobristCastleRights[0];
    }
    if (updatedRights & WQ)
    {
        zobristHash ^= zobristCastleRights[1];
    }
    if (updatedRights & BK)
    {
        zobristHash ^= zobristCastleRights[2];
    }
    if (updatedRights & BQ)
    {
        zobristHash ^= zobristCastleRights[3];
    }


    previousPositions[numOfPositions++] = zobristHash;

}

// MAKE CAPTURE -----------------==============================
void Position::makeCapture(Move move)
{

    irreversiblePositionTop = numOfPositions;

    positionInfo prevPosInfo;
    prevPosInfo.prevMove = move;
    prevPosInfo.blackPiecesBitboard = blackPiecesBitboard;
    prevPosInfo.whitePiecesBitboard = whitePiecesBitboard;

    prevPosInfo.zobrist = zobristHash;
    prevPosInfo.pawnZobrist = pawnZobristHash;

    prevPosInfo.pieceCaptured = NO_PIECE;
    prevPosInfo.enPassantSquare = enPassantSquare;
    prevPosInfo.castleRights = castleRights;

    prevPosInfo.mgScore = mgPstAndMaterialScore;
    prevPosInfo.egScore = egPstAndMaterialScore;
    prevPosInfo.gamePhase = gamePhase;

    prevPosInfo.irreversiblePositionTop = irreversiblePositionTop;

    CastlingRights oldCastleRights = castleRights;



    // extract move info
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;


    // create square masks for later
    Bitboard fromMask = 1ULL << fromSquare;
    Bitboard toMask = 1ULL << toSquare;
    Bitboard moveMask = fromMask | toMask;

    Piece pieceMoved = Board[fromSquare];

    // update board-piece zobrist value
    zobristHash ^= zobristPieces[fromSquare][pieceMoved];
    bool pieceMovedIsPawn = (pieceMoved % 6 == 0) && (pieceMoved < 12);
    if (pieceMovedIsPawn)
    {
        pawnZobristHash ^= zobristPieces[fromSquare][pieceMoved];
    }

    // update white or black piece bitboard for the piece that moved
    int colorDelta; // we add this to easily calculate the piece index without checking whose turn it is down the line
    if (whiteToMoveFlag)
    {
        colorDelta = 0;
        whitePiecesBitboard ^= moveMask;

        // for WK : if the from square was from the white kingside rook's starting position then castling rights for WK sohuld be disabled, same logic for queenside

        // if enemy rook was captured update black's castle rights, if own rook moved update the corresponding castle side right
        // if a move was made that corresponds to one of the 4 rook square masks then that means we need to update a castling right, that happens by
        // shifting the bit that becomes 1 after the bitwise OR operation x amount of times to fit the castle rights 4bit encoding
        // then to properly update the castling rights just do a bitwise AND NOT operation with the current castle rights
        castleRights &= ~ ((((fromMask & wkRookMask) >> 7) | ((fromMask & wqRookMask) << 1)) | ((toMask & bkRookMask) >> 61) | ((toMask & bqRookMask) >> 53));
        if ((fromMask & e1Mask))
        {
            castleRights &= 12;
        }

        // adjust incremental score from the piece that moved by subtracting the score from its original square and adding the score from the new square (promotions handled differently)
        mgPstAndMaterialScore -= mgTable[pieceMoved][fromSquare];
        egPstAndMaterialScore -= egTable[pieceMoved][fromSquare];
        if (!(flag & 8))
        {
            if (pieceMovedIsPawn) pawnZobristHash ^= zobristPieces[toSquare][wp];
            mgPstAndMaterialScore += mgTable[pieceMoved][toSquare];
            egPstAndMaterialScore += egTable[pieceMoved][toSquare];
        }

    }
    else
    {
        colorDelta = 6;
        blackPiecesBitboard ^= moveMask;

        castleRights &= ~ ((((fromMask & bkRookMask) >> 61) | ((fromMask & bqRookMask) >> 53)) | ((toMask & wkRookMask) >> 7) | ((toMask & wqRookMask) << 1));
        if ((fromMask & e8Mask))
        {
            castleRights &= 3;
        }

        mgPstAndMaterialScore += mgTable[pieceMoved][fromSquare];
        egPstAndMaterialScore += egTable[pieceMoved][fromSquare];
        if (!(flag & 8))
        {
            if (pieceMovedIsPawn) pawnZobristHash ^= zobristPieces[toSquare][bp];
            mgPstAndMaterialScore -= mgTable[pieceMoved][toSquare];
            egPstAndMaterialScore -= egTable[pieceMoved][toSquare];
        }

    }

    // en passant square is set to NONE since we are making a capture move

    if (enPassantSquare != NO_SQUARE)
    {
        zobristHash ^= zobristEnpassantFile[enPassantSquare % 8];
        enPassantSquare = NO_SQUARE;
    }

    // update mailbox ( remove piece moved from its starting position )

    Board[fromSquare] = NO_PIECE;

    // quiet moves are the most common with flag == 0 so adding this check will prevent a lot of unecessary checks down the line
    // without this a quiet move will have to go through all the below checks for no reason

    // handle normal captures
    if (flag == 4)
    {

        Piece capPiece = Board[toSquare];
        if ((capPiece % 6) == 0)
        {
            pawnZobristHash ^= zobristPieces[toSquare][capPiece];
        }
        gamePhase -= gamephaseInc[capPiece];

        prevPosInfo.pieceCaptured = capPiece; // store cap piece

        // update bitboards of moved and captured pieces
        pieceBitboard[capPiece] &= ~toMask;
        pieceBitboard[pieceMoved] ^= moveMask;

        // update mailbox
        Board[toSquare] = pieceMoved;

        // update zobrist, xor captured piece in to square
        zobristHash ^= zobristPieces[toSquare][capPiece];


        // update white/black/occupied (remove captured piece)
        if (whiteToMoveFlag)
        {
            mgPstAndMaterialScore += mgTable[capPiece][toSquare];
            egPstAndMaterialScore += egTable[capPiece][toSquare];

            blackPiecesBitboard &= ~toMask;
        } else
        {
            mgPstAndMaterialScore -= mgTable[capPiece][toSquare];
            egPstAndMaterialScore -= egTable[capPiece][toSquare];

            whitePiecesBitboard &= ~toMask;
        }

        zobristHash ^= zobristPieces[toSquare][pieceMoved];
    }
    else if (flag == 5) // en passant
    {
        if (whiteToMoveFlag)
        {
            prevPosInfo.pieceCaptured = bp;
            int epCapSq = toSquare - 8;
            // remove captured pawn from bp bitboard and Board mailbox:
            Bitboard capPawnMask = 1ULL << (epCapSq);
            pieceBitboard[bp] &= ~capPawnMask;
            pieceBitboard[wp] ^= moveMask;
            Board[epCapSq] = NO_PIECE;
            Board[toSquare] = wp;
            // also remove from black piece bb and occupied squares bb
            blackPiecesBitboard &= ~capPawnMask;

            mgPstAndMaterialScore += mgTable[bp][epCapSq];
            egPstAndMaterialScore += egTable[bp][epCapSq];
            gamePhase -= gamephaseInc[bp];

            // update zobrist by removing ep relevant black pawn
            pawnZobristHash ^= zobristPieces[epCapSq][bp];
            zobristHash ^= zobristPieces[epCapSq][bp];
            zobristHash ^= zobristPieces[toSquare][wp];

        } else
        {
            // black makes ep
            prevPosInfo.pieceCaptured = wp;
            int epCapSq = toSquare + 8;
            // remove captured pawn from bp bitboard and Board mailbox:
            Bitboard capPawnMask = 1ULL << (epCapSq);
            pieceBitboard[wp] &= ~capPawnMask;
            pieceBitboard[bp] ^= moveMask;
            Board[epCapSq] = NO_PIECE;
            Board[toSquare] = bp;
            // also remove from white piece bb and occupied squares bb
            whitePiecesBitboard &= ~capPawnMask;

            mgPstAndMaterialScore -= mgTable[wp][epCapSq];
            egPstAndMaterialScore -= egTable[wp][epCapSq];
            gamePhase -= gamephaseInc[wp];

            pawnZobristHash ^= zobristPieces[epCapSq][wp];
            zobristHash ^= zobristPieces[epCapSq][wp];
            zobristHash ^= zobristPieces[toSquare][bp];
        }
    }

    // captures are done , now handle promotions and promo-captures
    else // handle promotions
    {
        // add promoted piece to mailbox
        // (colorDelta + flag % 4 + 1) gives the index of promoted piece based on the 16 bit move encoding i am using
        Piece promotedPiece = static_cast<Piece>(colorDelta + flag % 4 + 1);

        if (flag & 4) // if move is promo-capture
        {
            Piece capturedPiece = Board[toSquare];
            prevPosInfo.pieceCaptured = capturedPiece;

            pieceBitboard[capturedPiece] &= ~toMask;
            gamePhase -= gamephaseInc[capturedPiece];

            // remove captured piece with xor
            zobristHash ^= zobristPieces[toSquare][capturedPiece];

            if (whiteToMoveFlag)
            {
                mgPstAndMaterialScore += mgTable[capturedPiece][toSquare];
                egPstAndMaterialScore += egTable[capturedPiece][toSquare];

                blackPiecesBitboard &= ~toMask;
            } else
            {
                mgPstAndMaterialScore -= mgTable[capturedPiece][toSquare];
                egPstAndMaterialScore -= egTable[capturedPiece][toSquare];

                whitePiecesBitboard &= ~toMask;
            }
        }

        Board[toSquare] = promotedPiece; // add promoted piece to the board
        pieceBitboard[promotedPiece] |= toMask; // add promoted piece to its bitboard
        pieceBitboard[pieceMoved] &= ~fromMask;

        // update zobrist with promoted piece
        zobristHash ^= zobristPieces[toSquare][promotedPiece];

        gamePhase += (gamephaseInc[promotedPiece] - gamephaseInc[0]);
        if (whiteToMoveFlag)
        {
            mgPstAndMaterialScore += mgTable[promotedPiece][toSquare];
            egPstAndMaterialScore += egTable[promotedPiece][toSquare];
        } else
        {
            mgPstAndMaterialScore -= mgTable[promotedPiece][toSquare];
            egPstAndMaterialScore -= egTable[promotedPiece][toSquare];
        }
    }

    zobristHash ^= zobristBlackToMove;

    whiteToMoveFlag = !whiteToMoveFlag;
    prevPositionsLog[positionLogTop++] = prevPosInfo;

    occupiedSquaresBitboard = whitePiecesBitboard | blackPiecesBitboard;

    // update zobrist castling rights , we only xor one castle right if the old differs from the new
    CastlingRights updatedRights = oldCastleRights ^ castleRights;
    if (updatedRights & WK)
    {
        zobristHash ^= zobristCastleRights[0];
    }
    if (updatedRights & WQ)
    {
        zobristHash ^= zobristCastleRights[1];
    }
    if (updatedRights & BK)
    {
        zobristHash ^= zobristCastleRights[2];
    }
    if (updatedRights & BQ)
    {
        zobristHash ^= zobristCastleRights[3];
    }

    previousPositions[numOfPositions++] = zobristHash;

}
// MAKE CAPTURE -----------------==============================




void Position::unmakeMove()
{
    numOfPositions--;


    whiteToMoveFlag = !whiteToMoveFlag;

    positionLogTop--;
    positionInfo prevPosInfo = prevPositionsLog[positionLogTop];

    // use the info from previous position to reset bitboards, castle rights and ep square
    blackPiecesBitboard = prevPosInfo.blackPiecesBitboard;
    whitePiecesBitboard = prevPosInfo.whitePiecesBitboard;
    occupiedSquaresBitboard = blackPiecesBitboard | whitePiecesBitboard;

    zobristHash = prevPosInfo.zobrist;
    pawnZobristHash = prevPosInfo.pawnZobrist;

    enPassantSquare = prevPosInfo.enPassantSquare;
    castleRights = prevPosInfo.castleRights;

    mgPstAndMaterialScore = prevPosInfo.mgScore;
    egPstAndMaterialScore = prevPosInfo.egScore;
    gamePhase = prevPosInfo.gamePhase;

    irreversiblePositionTop = prevPosInfo.irreversiblePositionTop;

    Piece pieceCaptured = prevPosInfo.pieceCaptured;
    Move move = prevPosInfo.prevMove;


    // extract move info
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;


    // create square masks for later
    Bitboard fromMask = 1ULL << fromSquare;
    Bitboard toMask = 1ULL << toSquare;
    Bitboard moveMask = fromMask | toMask;




    if (flag > 1) // most of the time the move is quiet so add this check to avoid searching for unnecessary if flag conditions
    {
        // ALL PROMOTIONS (AND PROMO-CAPS)
        if (flag & 8)
        {

            int colorDelta = whiteToMoveFlag ? 0 : 6;
            int promoPieceIndex = colorDelta + 1 + flag % 4;

            // remove promoted piece from its bitboard
            pieceBitboard[promoPieceIndex] &= ~toMask;
            // update pawn bitboard using color delta (0 for wp, 6 for bp)
            pieceBitboard[colorDelta] |= fromMask;

            // if promo capture remove add captured piece to its bitboard
            // and also add it to the mailbox
            if (pieceCaptured != NO_PIECE)
            {

                pieceBitboard[pieceCaptured] |= toMask;
                Board[toSquare] = pieceCaptured;

            } else
            {
                Board[toSquare] = NO_PIECE;
            }
            Board[fromSquare] = static_cast<Piece>(colorDelta);

        }
        else // non-promotion moves, the most common case
        {
            Piece pieceMoved = Board[toSquare];
            pieceBitboard[pieceMoved] ^= moveMask;
            Board[fromSquare] = pieceMoved;
            // restore bitboard of enemy piece captured if move was a capture
            if (pieceCaptured != NO_PIECE)
            {
                if (flag == 5)
                {
                    if (whiteToMoveFlag)
                    {
                        int epCapturedPawnSq = toSquare - 8;
                        pieceBitboard[bp] |= (1ULL << epCapturedPawnSq);
                        Board[epCapturedPawnSq] = bp;
                        Board[toSquare] = NO_PIECE;
                    } else
                    {
                        int epCapturedPawnSq = toSquare + 8;
                        pieceBitboard[wp] |= (1ULL << epCapturedPawnSq);
                        Board[epCapturedPawnSq] = wp;
                        Board[toSquare] = NO_PIECE;
                    }
                } else
                {
                    pieceBitboard[pieceCaptured] |= toMask;
                    Board[toSquare] = pieceCaptured;
                }
            } else
            {
                Board[toSquare] = NO_PIECE;


                if (whiteToMoveFlag)
                {
                    if (flag == 2)
                    {
                        pieceBitboard[wR] ^= wkRookMoveMask;
                        Board[7] = wR;
                        Board[5] = NO_PIECE;
                    } else if (flag == 3)
                    {
                        pieceBitboard[wR] ^= wqRookMoveMask;
                        Board[0] = wR;
                        Board[3] = NO_PIECE;
                    }
                    // black castles
                } else
                {
                    if (flag == 2)
                    {
                        pieceBitboard[bR] ^= bkRookMoveMask;
                        Board[63] = bR;
                        Board[61] = NO_PIECE;
                    } else if (flag == 3)
                    {
                        pieceBitboard[bR] ^= bqRookMoveMask;
                        Board[56] = bR;
                        Board[59] = NO_PIECE;
                    }
                }



            }
        }

    }

    else // QUIET MOVES AND TWO SQUARE ADVANCES
    {

        Piece pieceMoved = Board[toSquare];
        pieceBitboard[pieceMoved] ^= moveMask;
        Board[toSquare] = NO_PIECE;
        Board[fromSquare] = pieceMoved;

    }

    //zobristHash ^= zobristBlackToMove;

    // ASSERT
    assert(pawnZobristHash == computePawnZobristHash());

}



// UNMAKE CAPTURE -------------------------================================
void Position::unmakeCapture()
{
    numOfPositions--;

    whiteToMoveFlag = !whiteToMoveFlag;

    positionLogTop--;
    positionInfo prevPosInfo = prevPositionsLog[positionLogTop];

    // use the info from previous position to reset bitboards, castle rights and ep square
    blackPiecesBitboard = prevPosInfo.blackPiecesBitboard;
    whitePiecesBitboard = prevPosInfo.whitePiecesBitboard;
    occupiedSquaresBitboard = blackPiecesBitboard | whitePiecesBitboard;

    zobristHash = prevPosInfo.zobrist;
    pawnZobristHash = prevPosInfo.pawnZobrist;

    enPassantSquare = prevPosInfo.enPassantSquare;
    castleRights = prevPosInfo.castleRights;

    mgPstAndMaterialScore = prevPosInfo.mgScore;
    egPstAndMaterialScore = prevPosInfo.egScore;
    gamePhase = prevPosInfo.gamePhase;

    irreversiblePositionTop = prevPosInfo.irreversiblePositionTop;

    Piece pieceCaptured = prevPosInfo.pieceCaptured;
    Move move = prevPosInfo.prevMove;


    // extract move info
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;


    // create square masks for later
    Bitboard fromMask = 1ULL << fromSquare;
    Bitboard toMask = 1ULL << toSquare;
    Bitboard moveMask = fromMask | toMask;





    // ALL PROMOTIONS (AND PROMO-CAPS)
    if (flag & 8)
    {
        int colorDelta = whiteToMoveFlag ? 0 : 6;
        int promoPieceIndex = colorDelta + 1 + flag % 4;

        // remove promoted piece from its bitboard
        pieceBitboard[promoPieceIndex] &= ~toMask;
        // update pawn bitboard using color delta (0 for wp, 6 for bp)
        pieceBitboard[colorDelta] |= fromMask;

        // if promo capture remove add captured piece to its bitboard
        // and also add it to the mailbox
        if (pieceCaptured != NO_PIECE)
        {
            pieceBitboard[pieceCaptured] |= toMask;
            Board[toSquare] = pieceCaptured;
        } else
        {
            Board[toSquare] = NO_PIECE;
        }
        Board[fromSquare] = static_cast<Piece>(colorDelta);
    }
    else // non-promotion moves, the most common case
    {
        Piece pieceMoved = Board[toSquare];
        pieceBitboard[pieceMoved] ^= moveMask;
        Board[fromSquare] = pieceMoved;
        // restore bitboard of enemy piece captured if move was a capture
        if (pieceCaptured != NO_PIECE)
        {
            if (flag == 5)
            {
                if (whiteToMoveFlag)
                {
                    int epCapturedPawnSq = toSquare - 8;
                    pieceBitboard[bp] |= (1ULL << epCapturedPawnSq);
                    Board[epCapturedPawnSq] = bp;
                    Board[toSquare] = NO_PIECE;
                }
                else
                {
                    int epCapturedPawnSq = toSquare + 8;
                    pieceBitboard[wp] |= (1ULL << epCapturedPawnSq);
                    Board[epCapturedPawnSq] = wp;
                    Board[toSquare] = NO_PIECE;
                }
            }
            else
            {
                pieceBitboard[pieceCaptured] |= toMask;
                Board[toSquare] = pieceCaptured;
            }
        }
        else
        {
            Board[toSquare] = NO_PIECE;

            if (whiteToMoveFlag)
            {
                if (flag == 2)
                {
                    pieceBitboard[wR] ^= wkRookMoveMask;
                    Board[7] = wR;
                    Board[5] = NO_PIECE;
                } else if (flag == 3)
                {
                    pieceBitboard[wR] ^= wqRookMoveMask;
                    Board[0] = wR;
                    Board[3] = NO_PIECE;
                }
                // black castles
            }
            else
            {
                if (flag == 2)
                {
                    pieceBitboard[bR] ^= bkRookMoveMask;
                    Board[63] = bR;
                    Board[61] = NO_PIECE;
                } else if (flag == 3)
                {
                    pieceBitboard[bR] ^= bqRookMoveMask;
                    Board[56] = bR;
                    Board[59] = NO_PIECE;
                }
            }



        }
    }

}
// UNMAKE CAPTURE -----------------------------------====================================


void Position::calculatePstAndMaterialScore()
{
    gamePhase = 0;
    mgPstAndMaterialScore = 0;
    egPstAndMaterialScore = 0;
    for (int sq = 0; sq < 64; sq++)
    {
        Piece p = Board[sq];
        if (p != NO_PIECE)
        {
            gamePhase += gamephaseInc[p];
            // white
            if (p < 6)
            {
                mgPstAndMaterialScore += mgTable[p][sq];
                egPstAndMaterialScore += egTable[p][sq];
            }
            // black
            else
            {
                mgPstAndMaterialScore -= mgTable[p][sq];
                egPstAndMaterialScore -= egTable[p][sq];
            }
        }
    }
}

// for now return zobrist hash for easier debugging might change later to set the hash directly
ZobristHash Position::computeZobristHash()
{
    ZobristHash hash = 0;

    // get hash from board pieces
    for (int sq = 0; sq < 64; sq++)
    {
        if (Board[sq] != NO_PIECE)
        {
            hash ^= zobristPieces[sq][Board[sq]];
        }
    }

    // castling rights
    // WK:
    hash ^= (castleRights & WK) * zobristCastleRights[0];

    // WQ:
    hash ^= ((castleRights & WQ) >> 1) * zobristCastleRights[1];

    // BK:
    hash ^= ((castleRights & BK) >> 2) * zobristCastleRights[2];

    // BQ:
    hash ^= ((castleRights & BQ) >> 3) * zobristCastleRights[3];

    // enpassant file
    if (enPassantSquare != NO_SQUARE)
    {
        hash ^= zobristEnpassantFile[enPassantSquare % 8];
    }

    // black to move
    if (!whiteToMoveFlag)
    {
        hash ^= zobristBlackToMove;
    }

    return hash;
}


ZobristHash Position::computePawnZobristHash()
{
    ZobristHash hash = 0;

    // get hash from board pieces
    for (int sq = 0; sq < 64; sq++)
    {
        if (Board[sq] == 0 || Board[sq] == 6)
        {
            hash ^= zobristPieces[sq][Board[sq]];
        }
    }

    return hash;
}




// SEE :
Bitboard Position::getAllAttackersToSquare(int targetSquare)
{


    Bitboard attackersBitboard = 0ULL;

    attackersBitboard |= ( pawnAttacks[Black][targetSquare] &  pieceBitboard[wp] ) | ( pawnAttacks[White][targetSquare] &  pieceBitboard[bp] );
    attackersBitboard |= knightAttacks[targetSquare] & (pieceBitboard[wN] | pieceBitboard[bN]);
    attackersBitboard |= kingAttacks[targetSquare] & (pieceBitboard[wK] | pieceBitboard[bK]);
    // occupancy attacks
    Bitboard allDiagonalAttackers = pieceBitboard[wB] | pieceBitboard[bB] | pieceBitboard[bQ] | pieceBitboard[wQ];
    Bitboard allRookAttackers = pieceBitboard[wR] | pieceBitboard[bR] | pieceBitboard[bQ] | pieceBitboard[wQ];
    attackersBitboard |= getBishopAttacks(targetSquare, occupiedSquaresBitboard ) & allDiagonalAttackers; // bishops and queens
    attackersBitboard |= getRookAttacks(targetSquare, occupiedSquaresBitboard) & allRookAttackers; // rooks and queens
    return attackersBitboard;
}


Bitboard Position::getLeastValuablePiece(Bitboard pieces, int colorDelta, int &piece) // colorDelta is 0 for white 6 for black
{

    int kingPieceIndex = 5 + colorDelta;
    int pawnIndex = colorDelta;

    for (piece = pawnIndex; piece <= kingPieceIndex; piece++)
    {
        Bitboard subset = pieces & pieceBitboard[piece];
        if (subset) return subset & -subset;
    }
    return 0;
}



// for SEE
Bitboard Position::xRayAttackersToSquare(int targetSquare, Bitboard occupancy) const
{
    Bitboard attackersBitboard = 0ULL;

    Bitboard allDiagonalAttackers = pieceBitboard[wB] | pieceBitboard[bB] | pieceBitboard[bQ] | pieceBitboard[wQ];
    attackersBitboard |= getBishopAttacks(targetSquare, occupancy ) & allDiagonalAttackers; // bishops and queens
    Bitboard allRookAttackers = pieceBitboard[wR] | pieceBitboard[bR] | pieceBitboard[bQ] | pieceBitboard[wQ];
    attackersBitboard |= getRookAttacks(targetSquare, occupancy ) & allRookAttackers; // rooks and queens


    //std::cout << "exiting RAY ATTACKERS" << std::endl;
    return attackersBitboard & occupancy;
}


/*
    1. Generate all of the attacks and x-ray attacks on the target square for both sides
    2. The “see_value” is set to the value of the initially captured piece
    3. The “trophy_value” is set to the value of the piece making the initial capture
    4. Find the least valuable opponent’s piece which is attacking the target square (and is not blocked)
    5. Reduce the see_value by the trophy_value i.e. see_value = see_value – trophy_value
    6. Set the trophy_value equal to the value of the piece found in step 4
    7. If see_value >= threshold then return TRUE (i.e. the side to_move can stand pat and still have a score which reaches the threshold)
    8. If see_value + trophy_value < threshold then return FALSE (i.e. even if the side to move captures the new piece on offer, they will not reach the threshold)
    9. Find the least valuable piece for the side-to-move which is attacking the target square (and is not blocked)
    10. Increase the see_value by the trophy_value i.e. see_value = see_value + trophy_value
    11. Set the trophy_value equal to the value of the piece found in step 9
    12. If see_value – trophy_value >= threshold then return TRUE (i.e. even of the opponent captures the new piece on offer, they will not each the threshold)
    13. If see_value < threshold then return FALSE (i.e. the opponent can stand-pat and the score is less than the threshold).
    14. While each side has potential captures go to Step 4.
*/

int Position::SEE( int toSquare, int targetPiece, int fromSquare, int attackerPiece)
{
    int gain[32];
    int depth = 0;
    Bitboard mayXRay = (whitePiecesBitboard & ~ (pieceBitboard[wN] | pieceBitboard[wK])) | (blackPiecesBitboard & ~ (pieceBitboard[bN] | pieceBitboard[bK]));
    Bitboard fromSet = 1ULL << fromSquare;
    Bitboard occupancy = occupiedSquaresBitboard;
    Bitboard attadef = getAllAttackersToSquare(toSquare);

    gain[depth] = averagePieceScore[targetPiece];


    do
    {
        depth++;
        gain[depth] = averagePieceScore[attackerPiece] - gain[depth - 1]; // speculative store, if defended
        attadef ^= fromSet; // reset bit in set to traverse
        occupancy ^= fromSet; // reset bit in temporary occupancy for x-Rays
        if (fromSet & mayXRay)
        {
            attadef |= xRayAttackersToSquare(toSquare, occupancy);
        }
        fromSet = getLeastValuablePiece(attadef, (depth & 1) * 6, attackerPiece);
    } while (fromSet);
    while (--depth)
    {
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
    }

    return gain[0];

}

// Converts a position to a fen string for general debugging and for NNUE data generation
std::string Position::boardToFen() const
{
    std::string fenStr;

    // Loop through every rank and file
    for (int rank = 7; rank >= 0; rank--)
    {
        int emptyCount = 0;
        for (int file = 0; file < 8; file++)
        {
            int sq = rank * 8 + file;
            Piece p = Board[sq];

            if (p == NO_PIECE)
            {
                emptyCount++;
            }
            else
            {
                if (emptyCount > 0)
                {
                    fenStr += std::to_string(emptyCount); // Add the number of empty squares before the piece
                    emptyCount = 0;
                }

                fenStr += pieceToCharacter[static_cast<int>(p)];
            }
        }

        if (emptyCount > 0) fenStr += std::to_string(emptyCount); // Number of empty squares after the last piece found in the rank
        if (rank > 0) fenStr += '/';
    }

    // Add the side to move character
    fenStr += whiteToMoveFlag ? " w " : " b ";

    // Check its castling right and add it to the fen string if it is legal
    std::string castlingRights;

    // White kingside
    if (castleRights & WK)
    {
        castlingRights += 'K';
    }
    // White queenside
    if (castleRights & WQ)
    {
        castlingRights += 'Q';
    }
    // Black kingside
    if (castleRights & BK)
    {
        castlingRights += 'k';
    }
    // Black queenside
    if (castleRights & BQ)
    {
        castlingRights += 'q';
    }

    // If non of the above castling rights are allowed then add '-' to the fen string
    if (castlingRights.empty()) castlingRights = "-";

    fenStr += castlingRights;
    fenStr += ' ';

    // We also add the en passant square if it exists
    if (enPassantSquare == NO_SQUARE)
    {
        fenStr += '-';
    }
    else
    {
        fenStr += SquareNames[enPassantSquare];
    }

    // For now hard coded half time and full time clocks because i have not implemented them in my position class
    fenStr += " 0 1";

    return fenStr;
}







