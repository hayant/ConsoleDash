#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "ThreadPool.h"
#include <vector>
#include <future>
#include <chrono>
#include <thread>

class GameEngine {
public:
    explicit GameEngine(ThreadPool& pool);
    
    void run();

private:
    ThreadPool& pool_;
    
    void updateInput();
    void updateGameLogic();
    void render(int frame);
};

#endif // GAMEENGINE_H
