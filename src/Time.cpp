//
// Created by theoa on 01/05/2026.
//

#include "Time.h"

TimeManager tm;

// this function is called before every search to start the move timer and to calculate the optimum and maximum time limit
void TimeManager::start(TimePoint myTime, TimePoint myInc, int movesToGo)
{
    shouldStop = false;
    startingTime = now();

    TimePoint totalRemainingTime = myTime + myInc * (movesToGo-1); // remaining time is time left in addition to the total increment we can get from moves to go


    optimumTimeLimit = totalRemainingTime / movesToGo;

    // the maximum time limit must ensure the engine never runs out of time so it will never be more than half of the total time remaining
    maximumTimeLimit = std::min(optimumTimeLimit * 3, totalRemainingTime / 2);
}