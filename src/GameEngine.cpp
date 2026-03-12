#include "GameEngine.h"
#include <iostream>
#include <sstream>

GameEngine::GameEngine(ThreadPool& pool) : pool_(pool) {}

void GameEngine::run() {
    using clock = std::chrono::steady_clock;
    const auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS
    auto next_frame = clock::now();
    int frame_count = 0;
    const int max_frames = 10; // Run for 10 frames as demo

    while (frame_count < max_frames) {
        auto frame_start = clock::now();
        
        // 1. Update game state (main thread)
        updateInput();
        updateGameLogic();
        
        // 2. Dispatch parallel work to thread pool
        std::vector<std::future<void>> futures;
        
        // Parallel: Frustum culling simulation
        futures.push_back(pool_.enqueue_future([this, frame_count] {
            std::ostringstream oss;
            oss << "[Frame " << frame_count << "] Frustum culling on thread "
                << std::this_thread::get_id() << std::endl;
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }));
        
        // Parallel: Physics broad phase simulation
        futures.push_back(pool_.enqueue_future([this, frame_count] {
            std::ostringstream oss;
            oss << "[Frame " << frame_count << "] Physics broad phase on thread "
                << std::this_thread::get_id() << std::endl;
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }));
        
        // Parallel: AI pathfinding simulation
        futures.push_back(pool_.enqueue_future([this, frame_count] {
            std::ostringstream oss;
            oss << "[Frame " << frame_count << "] AI pathfinding on thread "
                << std::this_thread::get_id() << std::endl;
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }));
        
        // 3. Wait for all parallel work to complete
        for (auto& f : futures) {
            f.wait();
        }
        
        // 4. Render (main thread)
        render(frame_count);
        
        // 5. Frame timing - wait until next frame
        next_frame += frame_duration;
        std::this_thread::sleep_until(next_frame);
        
        frame_count++;
    }
}

void GameEngine::updateInput() {
    // Input handling (simulated)
}

void GameEngine::updateGameLogic() {
    // Game logic update (simulated)
}

void GameEngine::render(int frame) {
    std::ostringstream oss;
    oss << "[Frame " << frame << "] Rendering on main thread" << std::endl;
    std::cout << oss.str();
}
