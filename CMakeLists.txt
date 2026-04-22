cmake_minimum_required(VERSION 4.1)
project(AChess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_executable(AChess main.cpp
        Types.h
        Bitboard.cpp
        Bitboard.h
        Attacks.cpp
        Attacks.h
        MagicNumbers.cpp
        MagicNumbers.h
        Position.cpp
        Position.h
        MoveGen.cpp
        MoveGen.h
        Score.cpp
        Score.h
        Perft.cpp
        Perft.h
        Search.cpp
        Search.h
        Zobrist.cpp
        Zobrist.h
        PRNG.cpp
        PRNG.h
        TranspositionTable.cpp
        TranspositionTable.h
        Tuning.cpp
        Tuning.h
)
