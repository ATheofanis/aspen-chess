<p align="center">
  <img src="Aspen-Logo3.png" alt="Aspen Chess Engine Logo"/>
</p>
<h1 align="center">
  Aspen Chess Engine <br/>
  <img src="https://img.shields.io/github/v/release/ATheofanis/AChess?style=flat-square&color=green&style=plastic" alt="Release">
  <img src="https://img.shields.io/badge/license-GPLv3-success?style=flat-square&color=yellow&style=plastic" alt="License: GPL v3">
  <img src="https://img.shields.io/badge/C++-20-00599C?style=flat-square&logo=c%2B%2B&color=important&style=plastic" alt="C++">
</h1>

Aspen is a UCI chess engine written in C++. <br/>
It uses a NNUE to evaluate positions, which was trained exclusively on self-generated data.

## Strength
| **Version** | [CCRL 40/15](https://computerchess.org.uk/4040/) | [CCRL Blitz](https://computerchess.org.uk/404/) |
| :---: | :---: | :--: |
| [2.3.0](https://github.com/ATheofanis/aspen-chess/releases/tag/v2.3.0) | 3250 | 3271 |
| [1.2.3](https://github.com/ATheofanis/aspen-chess/releases/tag/v1.3.0) | 2775 |  -   |
| [1.0.0](https://github.com/ATheofanis/aspen-chess/releases/tag/v1.0.0) | 2817 | 2758 |

Click [here](https://computerchess.org.uk/4040/cgi/compare_engines.cgi?family=Aspen&print=Rating+list&print=Results+table&print=LOS+table&print=Ponder+hit+table&print=Eval+difference+table&print=Comopp+gamenum+table&print=Overlap+table&print=Score+with+common+opponents) to see every version that has been tested so far



## Download

You can find the latest stable release on the [Releases](../../releases) page.

## Features

### Move Generation
Fast legal move generation using magic bitboards.

### Search
- Basic alpha-beta pruning
- Quiescence search
- Principal-Variation Search
- Late-move reductions
- Late-move pruning
- Transposition tables
- Null-move pruning
- Move ordering using MVV+CaptHist, killer moves and butterfly history
- Aspiration windows and iterative deepening
- A form of Internal iterative reductions inside LMR
- Reverse futility pruning
- Razoring
- Singular extensions
- Static exchange evaluation pruning
- Delta pruning
- Improving heuristic

### Evaluation

Aspen uses a (768 → 512) × 2 → 1 NNUE architecture, trained with [Bullet](https://github.com/jw1912/bullet). 
The network was trained exclusively on self-generated data from previous versions of Aspen, using a dataset of 562 million positions.

## Usage

Aspen is a standard UCI chess engine, so you can use any Chess GUI that supports UCI ([Arena](http://www.playwitharena.de/), [CuteChess](https://cutechess.com/), etc.)

## Acknowledgements
### **Special Thanks**
- Thank you to the testers: Lars Hallerström, Graham Banks and the CCRL organizers
- Thank you to Jim Ablett for finding weaknesses in the code and suggesting improvements
### **Engines**:
Thank you to the authors of these engines, that served as sources of inspiration for my engine
- [Stormphrax](https://github.com/Ciekce/Stormphrax) 
- [Chal](https://github.com/namanthanki/chal)
- [Stockfish](https://stockfishchess.org/)
- [Berserk](https://github.com/jhonnold/berserk)
### Tools
The tools that I used to create the engine
- [Bullet](https://github.com/jw1912/bullet) - NNUE trainer
- [Incbin](https://github.com/graphitemaster/incbin) - To embed the binary weights file of the network, to the executable

