//
// Created by theoa on 01/05/2026.
//

#pragma once
#include "Types.h"

// Returns current time - used to calculate elapsed time
inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


// Class to handle the engine's time control calculations
class TimeManager
{
private:
    TimePoint startingTime{};
    TimePoint optimumTimeLimit{}; // This is the soft time limit cap, the search can exceed this limit
    TimePoint maximumTimeLimit{}; // This is the time limit hard cap meaning we can never exceed it in the search
    bool shouldStop{};
    bool timeEnabled = true;      // Time is disabled for certain UCI commands
public:
    // Called once at the start of every search to set the values of startingTime, optimum limit and maximum limit (and also reset the shouldStop flag)
    void start(TimePoint myTime, TimePoint myInc, int movesToGo = 25);

    // sets should stop flag to true
    void stopSearch() { shouldStop = true; }

    void disableTimeControl() { timeEnabled = false; }

    bool isTimeEnabled() { return timeEnabled; }

    // returns the bool value of should stop flag
    bool getShouldStopFlag() { return shouldStop; }


    // returns the time that has passed from the moment the search started
    TimePoint elapsedTime() const { return now() - startingTime; }

    TimePoint optimum() const { return optimumTimeLimit; }
    TimePoint maximum() const { return maximumTimeLimit; }

    bool optimumExpired() { return elapsedTime() >= optimumTimeLimit; }
    bool maximumExpired() { return elapsedTime() >= maximumTimeLimit; }

};

extern TimeManager tm;