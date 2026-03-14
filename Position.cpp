//
// Created by theoa on 07/03/2026.
//

#include "Position.h"

#include <cassert>

#include "MoveGen.h"

// clears everything regarding a position
void Position::clearPosition()
{
    for (auto & sq : Board)
    {
        sq = NO_PIECE;
    }
    for (unsigned long long & piece : pieceBitboard) {
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
    positionLogTop = 0;
    whiteToMoveFlag = true;
    isCheckmate = false;
    isStalemate = false;
    enPassantSquare = NO_SQUARE;
    castleRights = 15;
    pstScore = 0;
    eval = 0;


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
        if (charToPiece.contains(c))
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
            if (static_cast<int>(piece) <= 5)
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

}


void Position::makeMove(Move move)
{
    assert(whitePiecesBitboard == (pieceBitboard[wp]|pieceBitboard[wN]|pieceBitboard[wB]|pieceBitboard[wR]|pieceBitboard[wQ]|pieceBitboard[wK]));
    assert(blackPiecesBitboard == (pieceBitboard[bp]|pieceBitboard[bN]|pieceBitboard[bB]|pieceBitboard[bR]|pieceBitboard[bQ]|pieceBitboard[bK]));

    assert(occupiedSquaresBitboard = whitePiecesBitboard | blackPiecesBitboard);


    positionInfo prevPosInfo;
    prevPosInfo.prevMove = move;
    prevPosInfo.blackPiecesBitboard = blackPiecesBitboard;
    prevPosInfo.whitePiecesBitboard = whitePiecesBitboard;
    prevPosInfo.occupiedSquaresBitboard = occupiedSquaresBitboard;
    prevPosInfo.pieceCaptured = NO_PIECE;
    //prevPosInfo.eval = eval;
    //prevPosInfo.pstScore = pstScore;
    prevPosInfo.enPassantSquare = enPassantSquare;
    prevPosInfo.castleRights = castleRights;



    // extract move info
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0x3F;


    // create square masks for later
    Bitboard fromMask = 1ULL << fromSquare;
    Bitboard toMask = 1ULL << toSquare;
    Bitboard moveMask = fromMask | toMask;

    // update white or black piece bitboard for the piece that moved
    int colorDelta; // we add this to easily calculate the piece index without checking whose turn it is down the line
    if (whiteToMoveFlag)
    {
        colorDelta = 0;
        whitePiecesBitboard ^= moveMask;

        // for WK : if the from square was from the white kingside rook's starting position then castling rights for WK sohuld be disabled, same logic for queenside

        //auto updatedCastleRights = (((fromMask & wkRookMask) >> 7) | ((fromMask & wqRookMask) << 1));
        //updatedCastleRights |= (((toMask & bkRookMask) >> 61) | ((toMask & bqRookMask) >> 56));

        // if enemy rook was captured update black's castle rights, if own rook moved update the corresponding castle side right
        // if a move was made that corresponds to one of the 4 rook square masks then that means we need to update a castling right, that happens by
        // shifting the bit that becomes 1 after the bitwise OR operation x amount of times to fit the castle rights 4bit encoding
        // then to properly update the castling rights just do a bitwise AND NOT operation with the current castle rights
        castleRights &= ~ ((((fromMask & wkRookMask) >> 7) | ((fromMask & wqRookMask) << 1)) | ((toMask & bkRookMask) >> 61) | ((toMask & bqRookMask) >> 53));
        if ((fromMask & e1Mask))
        {
            castleRights &= 12;
        }
    } else
    {
        colorDelta = 6;
        blackPiecesBitboard ^= moveMask;

        castleRights &= ~ ((((fromMask & bkRookMask) >> 61) | ((fromMask & bqRookMask) >> 53)) | ((toMask & wkRookMask) >> 7) | ((toMask & wqRookMask) << 1));
        if ((fromMask & e8Mask))
        {
            castleRights &= 3;
        }
    }

    if (flag != 1)
    {
        enPassantSquare = NO_SQUARE;
    }


    // update mailbox ( remove piece moved from its starting position )
    Piece pieceMoved = Board[fromSquare];

    Board[fromSquare] = NO_PIECE;

    // quiet moves are the most common with flag == 0 so adding this check will prevent a lot of unecessary checks down the line
    // without this a quiet move will have to go through all the below checks for no reason
    if (flag)
    {
        // handle captures
        // CORRECT ----------------------------------------------------------------
        if (flag == 4)
        {
            //std::cout << "ENTERED NON EP CAPTURES\n";

            Piece capPiece = Board[toSquare];
            prevPosInfo.pieceCaptured = capPiece; // store cap piece

            // update bitboards of moved and captured pieces
            pieceBitboard[capPiece] &= ~toMask;
            pieceBitboard[pieceMoved] ^= moveMask;

            // update mailbox
            Board[toSquare] = pieceMoved;

            // update white/black/occupied (remove captured piece)
            if (whiteToMoveFlag)
            {
                blackPiecesBitboard &= ~toMask;
            } else
            {
                whitePiecesBitboard &= ~toMask;
            }

        } // CORRECT ----------------------------------------------------------------

        // INCORRECT (?) ----------------------------------------------------------------
        else if (flag == 5) // en passant
        {
            //std::cout << "\nMAKING EN PASSANT MOVE\n";
            //Piece capPawn;
            if (whiteToMoveFlag)
            {
                prevPosInfo.pieceCaptured = bp;
                // remove captured pawn from bp bitboard and Board mailbox:
                Bitboard capPawnMask = 1ULL << (toSquare - 8);
                pieceBitboard[bp] &= ~capPawnMask;
                pieceBitboard[wp] ^= moveMask;
                Board[toSquare - 8] = NO_PIECE;
                Board[toSquare] = wp;
                // also remove from black piece bb and occupied squares bb
                blackPiecesBitboard &= ~capPawnMask;

            } else
            {
                // black makes ep
                prevPosInfo.pieceCaptured = wp;
                // remove captured pawn from bp bitboard and Board mailbox:
                Bitboard capPawnMask = 1ULL << (toSquare + 8);
                pieceBitboard[wp] &= ~capPawnMask;
                pieceBitboard[bp] ^= moveMask;
                Board[toSquare + 8] = NO_PIECE;
                Board[toSquare] = bp;
                // also remove from white piece bb and occupied squares bb
                whitePiecesBitboard &= ~capPawnMask;


            }

        } // (IN)CORRECT ----------------------------------------------------------------


        // captures are done , now handle promotions and promo-captures
        else if (flag & 8) // handle promotions
        {
            // add promoted piece to mailbox
            // (colorDelta + flag % 4 + 1) gives the index of promoted piece based on the 16 bit move encoding i am using
            Piece promotedPiece = static_cast<Piece>(colorDelta + flag % 4 + 1);
            Piece capturedPiece = Board[toSquare];

            prevPosInfo.pieceCaptured = capturedPiece;


            Board[toSquare] = promotedPiece; // add promoted piece to the board
            pieceBitboard[promotedPiece] |= toMask; // add promoted piece to its bitboard
            pieceBitboard[pieceMoved] &= ~fromMask;

            if (flag & 4) // if move is promo-capture
            {
                pieceBitboard[capturedPiece] &= ~toMask;
                if (whiteToMoveFlag)
                {
                    blackPiecesBitboard &= ~toMask;
                } else
                {
                    whitePiecesBitboard &= ~toMask;
                }
            }
        }
        else if (flag == 1)
        {
            //std::cout << "FLAG 1 --------------------------------------------";
            pieceBitboard[pieceMoved] ^= moveMask;
            Board[toSquare] = pieceMoved;
            enPassantSquare = whiteToMoveFlag ? (toSquare - 8) : (toSquare + 8);
        }
        else
        {
            if (whiteToMoveFlag) // White castles
            {

                //castleRights &= 12;
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

            }

            else // Black castles
            {
                //castleRights &= 3;
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
            }
        }

    } // if flag 0, quiet move
    else // quiet move -> ^= moveMask with the piece that moved and update white/black bb and occupied bb
    {
        pieceBitboard[pieceMoved] ^= moveMask;
        Board[toSquare] = pieceMoved;
    }

    whiteToMoveFlag = !whiteToMoveFlag;
    prevPositionsLog[positionLogTop++] = prevPosInfo;



    assert(whitePiecesBitboard == (pieceBitboard[wp]|pieceBitboard[wN]|pieceBitboard[wB]|pieceBitboard[wR]|pieceBitboard[wQ]|pieceBitboard[wK]));
    assert(blackPiecesBitboard == (pieceBitboard[bp]|pieceBitboard[bN]|pieceBitboard[bB]|pieceBitboard[bR]|pieceBitboard[bQ]|pieceBitboard[bK]));

    occupiedSquaresBitboard = whitePiecesBitboard | blackPiecesBitboard;



}


void Position::unmakeMove()
{
    assert(whitePiecesBitboard == (pieceBitboard[wp]|pieceBitboard[wN]|pieceBitboard[wB]|pieceBitboard[wR]|pieceBitboard[wQ]|pieceBitboard[wK]));
    whiteToMoveFlag = !whiteToMoveFlag;

    positionLogTop--;
    positionInfo prevPosInfo = prevPositionsLog[positionLogTop];

    // use the info from previous position to reset bitboards, castle rights and ep square
    occupiedSquaresBitboard = prevPosInfo.occupiedSquaresBitboard;
    blackPiecesBitboard = prevPosInfo.blackPiecesBitboard;
    whitePiecesBitboard = prevPosInfo.whitePiecesBitboard;
    enPassantSquare = prevPosInfo.enPassantSquare;
    castleRights = prevPosInfo.castleRights;

    Piece pieceCaptured = prevPosInfo.pieceCaptured;
    Move move = prevPosInfo.prevMove;

    //printMove(move);

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
            //std::cout << "INSIDE PROMO s----------------------------------------------------------------";
            //std::cout << pieceCaptured;
            //std::cout << "\n";
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
                //std::cout << "INSIDE PROMO CAP----------------------------------------------------------------";
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

        //if (flag == 1) std::cout << "FLAG IS 1";
        //std::cout << Board[toSquare] << std::endl;
        Piece pieceMoved = Board[toSquare];
        pieceBitboard[pieceMoved] ^= moveMask;
        Board[toSquare] = NO_PIECE;
        Board[fromSquare] = pieceMoved;

    }
    assert(whitePiecesBitboard == (pieceBitboard[wp]|pieceBitboard[wN]|pieceBitboard[wB]|pieceBitboard[wR]|pieceBitboard[wQ]|pieceBitboard[wK]));
    assert(blackPiecesBitboard == (pieceBitboard[bp]|pieceBitboard[bN]|pieceBitboard[bB]|pieceBitboard[bR]|pieceBitboard[bQ]|pieceBitboard[bK]));

}
