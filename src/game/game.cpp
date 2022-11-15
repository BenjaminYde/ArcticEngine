#include "engine/arctic_engine.h"

int main()
{
    ArcticEngine engine;
    engine.initialize();
    engine.run();
    engine.cleanup();
    
    return 0;
}
