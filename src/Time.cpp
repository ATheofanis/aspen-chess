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

    realTimeLeft = myTime;

    movesToGo = std::max(1, movesToGo - 1);
    TimePoint totalIncTime = myInc * movesToGo;


    TimePoint totalTime = myTime + totalIncTime;

    // Time left without increment to prevent the engine from flagging when it thinks it has time left because of increment
    TimePoint safeTime = myTime;

    // Use a fraction of the available time
    optimumTimeLimit = std::min(safeTime / 10, totalTime * 35 / 1000); // ~5.5% per move

    // Never allow the engine to use more than one fifth of the real non-increment time left
    maximumTimeLimit = std::min(safeTime / 5, totalTime * 180 / 1000);

    // Safety for moderately low time
    if (myTime < 15000) {
        optimumTimeLimit = myTime * 120 / 1000;
        maximumTimeLimit = myTime * 200 / 1000;
    }

    // Extra safety for very low time
    if (myTime < 5000) {
        optimumTimeLimit = myTime * 40 / 1000;
        maximumTimeLimit = myTime * 60 / 1000;
    }
}
