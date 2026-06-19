#include "UCI.h"
#include <string>

bool UCI_ENABLED = true;
bool test = ~UCI_ENABLED;

int main(int argc, char* argv[])
{
    bool OB_Bench = false;
    if (argc > 1)
    {
        std::string argument = argv[1];
        if (argument == "bench")
        {
            OB_Bench = true;
        }
    }

    // Start the UCI loop if UCI is enabled
    if (UCI_ENABLED)
    {
        LoopUCI(OB_Bench);
    }

    return 0;
}