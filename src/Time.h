//
// Created by theoa on 01/05/2026.
//

// Like stockfish, i use a time manager class to keep track of time spent on the search
// (https://github.com/official-stockfish/Stockfish/blob/master/src/timeman.h)


#pragma once
#include "Types.h"

// returns current time - used to calculate elapsed time
inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


// Class to handle the engine's time control calculations
class TimeManager
{
private:
    TimePoint startingTime{};
    TimePoint optimumTimeLimit{}; // this is the soft time limit cap, the search can exceed this limit
    TimePoint maximumTimeLimit{}; // this is the time limit hard cap meaning we can never exceed it in the search
    bool shouldStop{};
public:
    void start(TimePoint myTime, TimePoint myInc, int movesToGo = 25);

    // sets should stop flag to true
    void stopSearch() { shouldStop = true; }

    // returns the bool value of should stop flag
    bool getShouldStopFlag() { return shouldStop; }

    TimePoint elapsedTime() const { return now() - startingTime; }

    TimePoint optimum() const { return optimumTimeLimit; }
    TimePoint maximum() const { return maximumTimeLimit; }

    bool optimumExpired() { return elapsedTime() >= optimumTimeLimit; }
    bool maximumExpired() { return elapsedTime() >= maximumTimeLimit; }



};

extern TimeManager tm;