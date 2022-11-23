#include "engine/arctic_engine.h"
#include <iostream>

int main()
{
    ArcticEngine engine;
    engine.initialize();
    engine.run();
    engine.cleanup();

    return 0;
}
