cmake_minimum_required(VERSION 4.1)
project(AspenChess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_executable(AspenChess src/main.cpp
        src/Types.h
        src/Bitboard.cpp
        src/Bitboard.h
        src/Attacks.cpp
        src/Attacks.h
        src/MagicNumbers.cpp
        src/MagicNumbers.h
        src/Position.cpp
        src/Position.h
        src/LegalMoveGen.cpp
        src/LegalMoveGen.h
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
        src/TimeManager.cpp
        src/TimeManager.h
        src/DataGen.cpp
        src/DataGen.h
        src/nnue/nnue.cpp
        src/nnue/nnue.h
        src/nnue/accumulator.cpp
        src/nnue/accumulator.h
        src/nnue/Architecture.h
        src/UCI.cpp
        src/UCI.h
        src/PseudoMoveGen.cpp
        src/PseudoMoveGen.h
        src/MovePicker.cpp
        src/MovePicker.h
        src/History.h
        src/Parameters.cpp
        src/Parameters.h
)

target_compile_options(AspenChess PRIVATE -mavx2)
