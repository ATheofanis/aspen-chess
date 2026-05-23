//
// Created by theoa on 01/05/2026.
//

#pragma once
#include <chrono> // Explicitly include chrono here to ensure steady_clock is available
#include "Types.h"

// Returns current time - used to calculate elapsed time
inline TimeMs now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Class to handle the engine's time control calculations
class TimeManager
{
private:
    TimeMs startingTime{};
    TimeMs optimumTimeLimit{}; // This is the soft time limit cap, the search can exceed this limit
    TimeMs maximumTimeLimit{}; // This is the time limit hard cap meaning we can never exceed it in the search
    TimeMs realTimeLeft{};

    bool shouldStop = false;
    bool timeEnabled = true;      // Time is disabled for certain UCI commands
public:
    // Called once at the start of every search to set the values of startingTime, optimum limit and maximum limit (and also reset the shouldStop flag)
    void start(TimeMs myTime, TimeMs myInc, int movesToGo = 25);

    // Sets should stop flag to true
    void stopSearch() { shouldStop = true; }

    void disableTimeControl() { timeEnabled = false; }

    bool isTimeEnabled() { return timeEnabled; }

    // Returns the bool value of should stop flag
    bool getShouldStopFlag() { return shouldStop; }


    // Returns the time that has passed from the moment the search started
    TimeMs elapsedTime() const { return now() - startingTime; }

    TimeMs optimum() const { return optimumTimeLimit; }
    TimeMs maximum() const { return maximumTimeLimit; }

    bool optimumExpired()
    {
        TimeMs elapsed = elapsedTime();
        if (elapsed >= optimumTimeLimit || elapsed > realTimeLeft / 5) return true;
        return false;
    }

    bool maximumExpired() { return elapsedTime() >= maximumTimeLimit; }

    void reset()
    {
        timeEnabled = false;
        shouldStop = false;
        startingTime = now();
    }

};

extern TimeManager timeManager;