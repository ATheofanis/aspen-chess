//
// Created by theoa on 01/05/2026.
//

#include "Time.h"

TimeManager tm;

// This function is called before every search to start the move timer and to calculate the optimum and maximum time limit
// The optimum time limit is a general guide to tell the engine when a search should probably stop.
// In tricky positions the engine is allowed to go past the optimum time limit. For this reason we also have the maximum time limit
// which the engine is not allowed to surpass at any point, in order to prevenet flagging
void TimeManager::start(TimePoint myTime, TimePoint myInc, int movesToGo)
{
    // ---- This improved time management algorithm was kindly provided by Jim Ablett. ----
    shouldStop = false;
    startingTime = now();

    TimePoint totalTime = myTime + myInc * (movesToGo > 0 ? movesToGo - 1 : 20);

    // Use a fraction of the available time
    optimumTimeLimit = totalTime * 80 / 1000; // ~8% per move
    maximumTimeLimit = std::min(totalTime / 2, totalTime * 180 / 1000);

    // Extra safety for very low time
    if (myTime < 5000) {
        optimumTimeLimit = myTime * 40 / 1000;
        maximumTimeLimit = myTime * 60 / 1000;
    }
}
