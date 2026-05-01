cmake_minimum_required(VERSION 4.1)
project(AChess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_executable(AChess src/main.cpp
        src/Types.h
        src/Bitboard.cpp
        src/Bitboard.h
        src/Attacks.cpp
        src/Attacks.h
        src/MagicNumbers.cpp
        src/MagicNumbers.h
        src/Position.cpp
        src/Position.h
        src/MoveGen.cpp
        src/MoveGen.h
        src/Score.cpp
        src/Score.h
        src/Search.cpp
        src/Search.h
        src/Zobrist.cpp
        src/Zobrist.h
        src/PRNG.cpp
        src/PRNG.h
        src/TranspositionTable.cpp
        src/TranspositionTable.h
        src/Tuning.cpp
        src/Tuning.h
        src/Time.cpp
        src/Time.h
)
