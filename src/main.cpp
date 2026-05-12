#include "UCI.h"


bool UCI_ENABLED = true;
bool test = ~UCI_ENABLED;

int main(int argc, char* argv[])
{
    // Start the UCI loop if UCI is enabled
    if (UCI_ENABLED)
    {
        LoopUCI();
    }

    return 0;
}