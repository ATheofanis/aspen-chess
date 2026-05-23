<p align="center">
  <img src="https://img.shields.io/github/v/release/ATheofanis/AChess?style=flat-square&color=green&style=plastic" alt="Release">
  <img src="https://img.shields.io/badge/license-GPLv3-success?style=flat-square&color=informational&style=plastic" alt="License: GPL v3">
  <img src="https://img.shields.io/badge/C++-20-00599C?style=flat-square&logo=c%2B%2B&color=important&style=plastic" alt="C++">
</p>

# Aspen Chess Engine

Aspen is a UCI chess engine written in C++ that evaluates positions using a custom NNUE.

You can play against it online on [lichess](https://lichess.org/@/AspenBot).

## Download

You can find the latest stable release on the [Releases](../../releases) page.


## Features

### Move Generation
Fast legal move generation using magic bitboards.

### Search
Iterative deepening with aspiration windows over a negamax search with the following enhancements:
- Alpha-Beta pruning with Principal Variation Search (PVS)
- Transposition Table (TT) probing
- Null Move Pruning (NMP)
- Futility Pruning, Extended Futility Pruning, Reverse Futility Pruning
- Late Move Pruning (LMP)
- Razoring
- Late Move Reduction (LMR)
- Move ordering: Hash move, Killer moves, History heuristic, Static Exchange Evaluation (SEE)

### Evaluation

Aspen uses a (768 → 512) × 2 → 1 NNUE architecture, trained with [Bullet](https://github.com/jw1912/bullet). 
The network was trained exclusively on self-generated data from previous versions of Aspen, using a dataset of 562 million positions.

## Usage

Aspen communicates using the UCI Protocol and is therefore compatible with any Chess GUI that supports UCI such as [Arena](http://www.playwitharena.de/) or [CuteChess](https://cutechess.com/). Simply add the executable as an engine in your GUI of choice.

## Acknowledgements
- A huge thank you to Lars Hallerström for running thousands of test games to estimate Aspen's Elo
- [Eddie Sharick](https://www.youtube.com/@eddiesharick6649/videos) - his python chess engine series served as an introduction to chess programming and motivated me to continue
- [Chess Programming Wiki](https://www.chessprogramming.org/) — great resource on chess programming concepts and history
- [Talk Chess](https://talkchess.com/) — countless forums and discussions on advanced engine topics
- [Chess Programming on YouTube](https://www.youtube.com/@chessprogramming591) — didactic series that made many concepts easily understandable. His magic bitboards implementation was directly used in Aspen
- [Sebastian Lague](https://www.youtube.com/@SebastianLague) — his Chess Programming series was a great source of inspiration
- [Stockfish Team](https://stockfishchess.org/) — their open source engine was an invaluable reference for advanced concepts
- [Bullet](https://github.com/jw1912/bullet) - which I used to train Aspen's neural network
- [Jim Ablett](https://github.com/jimablett) - for kindly improving upon the previous versions of Aspen's time control, making the engine much stronger
- [Chal](https://github.com/namanthanki/chal) - a very well written chess engine that helped me improve the engine's search functions
