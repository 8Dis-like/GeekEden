# GeekEden Tech Blog - C++ Game Prototype Development: Core Snake Logic & A* Auto-Routing

**Published:** 2026-07-20
**Author:** [@8Dis-like](https://github.com/8Dis-like)

---

After four days of intense deep dives into OOP vs. ECS architectures, smart pointer memory management, STL under-the-hood mechanisms, and concurrent programming, Day 5 focuses on integrating everything we've learned to develop a comprehensive game prototype: **The classic Snake Game**.

Moreover, to align with our focus on **High-Performance Game Development and AI Systems**, we'll inject an **A* (A-Star) Pathfinding Algorithm** into the game. This transforms our snake into an "AI Snake" that autonomously calculates the shortest safe path to its food in a grid-based space.

## 🎮 Live Demo
👉 **[Click here to experience the WebAssembly C++ AI Snake live rendering animation](https://8Dis-like.github.io/GeekEden/demos/snake_wasm/index.html)**

## 1. Core Architecture Design & Data Structure Selection

Before diving into code, we must carefully select the core data structures:
1. **Snake Body Management (`std::deque`)**:
   The essence of snake movement is "inserting a new node at the head and deleting an old node at the tail." Using `std::vector` here would result in an unacceptable $O(N)$ time complexity for head insertion. A `std::deque<Point>` (Double-Ended Queue) natively supports $O(1)$ operations at both ends, making it the perfect underlying container.
2. **Collision Detection**:
   - **Boundary Collision**: Simple coordinate out-of-bounds checks.
   - **Self Collision**: Traversing the snake's body to check for overlaps. Note that since the tail will move forward in the next frame, targeting the current tail's position is usually considered safe.
   - **Food Collision**: Coordinate matching. If matched, the score increases, and the **tail deletion is skipped for that frame** (which naturally causes the snake to grow).
3. **AI Pathfinding (`std::priority_queue`)**:
   The core of the A* algorithm is maintaining an Open Set and prioritizing nodes with the smallest $f(n) = g(n) + h(n)$. We use a `std::priority_queue` as a min-heap, employing **Manhattan Distance** as our Heuristic function.

## 2. Core Code Implementation

Below is the complete C++ game prototype code. It doesn't require any third-party libraries and can compile and run directly in your console, visually simulating the AI pathfinding animation:

```cpp
#include <iostream>
#include <deque>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <thread>
#include <chrono>

// 1. Define Coordinate Structure
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

// Provide a Hash function for Point to be used in unordered containers
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

// 2. Define Direction Enum
enum class Direction { Up, Down, Left, Right };

// ==========================================
// Core Snake Game Logic
// ==========================================
class SnakeGame {
private:
    int width_, height_;
    std::deque<Point> snake_; // Manages snake body, O(1) head/tail operations
    Point food_;
    Direction dir_;
    bool game_over_;
    int score_;

public:
    SnakeGame(int w, int h) : width_(w), height_(h), dir_(Direction::Right), game_over_(false), score_(0) {
        snake_.push_back({w / 2, h / 2});     // Head
        snake_.push_back({w / 2 - 1, h / 2}); // Body
        spawnFood();
    }

    bool isGameOver() const { return game_over_; }
    int getScore() const { return score_; }

    void spawnFood() {
        while (true) {
            food_ = { std::rand() % width_, std::rand() % height_ };
            // Ensure food doesn't spawn on the snake's body
            bool on_snake = false;
            for (const auto& p : snake_) {
                if (p == food_) { on_snake = true; break; }
            }
            if (!on_snake) break;
        }
    }

    // Set direction, preventing instant suicide by reversing
    void setDirection(Direction new_dir) {
        if (dir_ == Direction::Up && new_dir == Direction::Down) return;
        if (dir_ == Direction::Down && new_dir == Direction::Up) return;
        if (dir_ == Direction::Left && new_dir == Direction::Right) return;
        if (dir_ == Direction::Right && new_dir == Direction::Left) return;
        dir_ = new_dir;
    }

    // Core Game Update Loop
    void update() {
        if (game_over_) return;

        Point head = snake_.front();
        Point next_head = head;

        switch (dir_) {
            case Direction::Up:    next_head.y -= 1; break;
            case Direction::Down:  next_head.y += 1; break;
            case Direction::Left:  next_head.x -= 1; break;
            case Direction::Right: next_head.x += 1; break;
        }

        // 1. Boundary Collision Detection
        if (next_head.x < 0 || next_head.x >= width_ || next_head.y < 0 || next_head.y >= height_) {
            game_over_ = true;
            return;
        }

        // 2. Self Collision Detection (excluding the tail, as it will move)
        for (size_t i = 0; i < snake_.size() - 1; ++i) {
            if (next_head == snake_[i]) {
                game_over_ = true;
                return;
            }
        }

        // Head Insertion: Move one step forward
        snake_.push_front(next_head);

        // 3. Food Collision Detection
        if (next_head == food_) {
            score_ += 10;
            spawnFood();
            // Skip tail deletion, allowing the snake to grow
        } else {
            // Delete tail: Maintain current length
            snake_.pop_back(); 
        }
    }

    // ==========================================
    // A* Pathfinding AI
    // ==========================================
    struct AStarNode {
        Point pt;
        int f; // f = g + h
        bool operator>(const AStarNode& o) const { return f > o.f; } // Min-heap
    };

    int heuristic(const Point& a, const Point& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y); // Manhattan Distance
    }

    void aiFindPathToFood() {
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        std::unordered_map<Point, Point, PointHash> came_from;
        std::unordered_map<Point, int, PointHash> g_score;

        Point start = snake_.front();
        open_set.push({start, heuristic(start, food_)});
        g_score[start] = 0;

        std::unordered_set<Point, PointHash> snake_body;
        for (const auto& p : snake_) snake_body.insert(p);

        bool found = false;

        while (!open_set.empty()) {
            Point curr = open_set.top().pt;
            open_set.pop();

            if (curr == food_) { found = true; break; }

            Point neighbors[4] = { {curr.x, curr.y-1}, {curr.x, curr.y+1}, {curr.x-1, curr.y}, {curr.x+1, curr.y} };
            
            for (const auto& next : neighbors) {
                // Out-of-bounds detection
                if (next.x < 0 || next.x >= width_ || next.y < 0 || next.y >= height_) continue;
                // Body collision detection (allows moving to the tail)
                if (snake_body.count(next) && next != snake_.back()) continue; 

                int tentative_g = g_score[curr] + 1;
                if (g_score.find(next) == g_score.end() || tentative_g < g_score[next]) {
                    came_from[next] = curr;
                    g_score[next] = tentative_g;
                    open_set.push({next, tentative_g + heuristic(next, food_)});
                }
            }
        }

        // Reconstruct path to find the VERY FIRST step to take
        if (found) {
            Point curr = food_;
            while (came_from.find(curr) != came_from.end()) {
                Point prev = came_from[curr];
                if (prev == start) {
                    if (curr.x > start.x) setDirection(Direction::Right);
                    else if (curr.x < start.x) setDirection(Direction::Left);
                    else if (curr.y > start.y) setDirection(Direction::Down);
                    else if (curr.y < start.y) setDirection(Direction::Up);
                    break;
                }
                curr = prev;
            }
        }
    }

    // Simple Console Rendering
    void render() {
        std::cout << "\033[2J\033[1;1H"; // Clear screen
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                Point p{x, y};
                if (p == food_) std::cout << "★ ";
                else if (p == snake_.front()) std::cout << "O ";
                else {
                    bool is_body = false;
                    for (size_t i = 1; i < snake_.size(); ++i) {
                        if (p == snake_[i]) { is_body = true; break; }
                    }
                    if (is_body) std::cout << "o ";
                    else std::cout << ". ";
                }
            }
            std::cout << "\n";
        }
        std::cout << "Score: " << score_ << "\n";
    }
};

int main() {
    std::srand(1024); // Fixed seed for reproducible presentation
    SnakeGame game(20, 10);
    
    // Simulate game loop for 20 frames to demonstrate AI capability
    for (int i = 0; i < 20 && !game.isGameOver(); ++i) {
        game.aiFindPathToFood(); // AI makes a decision
        game.update();           // Update logic
        game.render();           // Render frame
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "Auto-play Simulation Finished. Final Score: " << game.getScore() << std::endl;
    return 0;
}
```
