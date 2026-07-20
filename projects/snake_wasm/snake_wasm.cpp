#include <emscripten.h>
#include <deque>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Define Point structure
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

// Hash for Point
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

enum class Direction { Up, Down, Left, Right };

// Core Logic
class SnakeGame {
private:
    int width_, height_;
    std::deque<Point> snake_;
    Point food_;
    Direction dir_;
    bool game_over_;
    int score_;

public:
    SnakeGame(int w, int h) : width_(w), height_(h), dir_(Direction::Right), game_over_(false), score_(0) {
        std::srand(std::time(nullptr));
        snake_.push_back({w / 2, h / 2});
        snake_.push_back({w / 2 - 1, h / 2});
        spawnFood();
    }

    bool isGameOver() const { return game_over_; }
    int getScore() const { return score_; }
    const std::deque<Point>& getSnake() const { return snake_; }
    Point getFood() const { return food_; }

    void spawnFood() {
        while (true) {
            food_ = { std::rand() % width_, std::rand() % height_ };
            bool on_snake = false;
            for (const auto& p : snake_) {
                if (p == food_) { on_snake = true; break; }
            }
            if (!on_snake) break;
        }
    }

    void setDirection(Direction new_dir) {
        if (dir_ == Direction::Up && new_dir == Direction::Down) return;
        if (dir_ == Direction::Down && new_dir == Direction::Up) return;
        if (dir_ == Direction::Left && new_dir == Direction::Right) return;
        if (dir_ == Direction::Right && new_dir == Direction::Left) return;
        dir_ = new_dir;
    }

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

        if (next_head.x < 0 || next_head.x >= width_ || next_head.y < 0 || next_head.y >= height_) {
            game_over_ = true; return;
        }

        for (size_t i = 0; i < snake_.size() - 1; ++i) {
            if (next_head == snake_[i]) { game_over_ = true; return; }
        }

        snake_.push_front(next_head);

        if (next_head == food_) {
            score_ += 10;
            spawnFood();
        } else {
            snake_.pop_back(); 
        }
    }

    struct AStarNode {
        Point pt;
        int f;
        bool operator>(const AStarNode& o) const { return f > o.f; }
    };

    int heuristic(const Point& a, const Point& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
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

            Point neighbors[4] = {{curr.x, curr.y-1}, {curr.x, curr.y+1}, {curr.x-1, curr.y}, {curr.x+1, curr.y}};
            
            for (const auto& next : neighbors) {
                if (next.x < 0 || next.x >= width_ || next.y < 0 || next.y >= height_) continue;
                if (snake_body.count(next) && next != snake_.back()) continue; 

                int tentative_g = g_score[curr] + 1;
                if (g_score.find(next) == g_score.end() || tentative_g < g_score[next]) {
                    came_from[next] = curr;
                    g_score[next] = tentative_g;
                    open_set.push({next, tentative_g + heuristic(next, food_)});
                }
            }
        }

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
};

// Global Instance for WASM
SnakeGame* global_game = nullptr;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void init_game(int w, int h) {
        if (global_game) delete global_game;
        global_game = new SnakeGame(w, h);
    }

    EMSCRIPTEN_KEEPALIVE
    void tick() {
        if (global_game && !global_game->isGameOver()) {
            global_game->aiFindPathToFood();
            global_game->update();
        }
    }

    EMSCRIPTEN_KEEPALIVE
    bool is_game_over() { return global_game ? global_game->isGameOver() : true; }

    EMSCRIPTEN_KEEPALIVE
    int get_score() { return global_game ? global_game->getScore() : 0; }

    EMSCRIPTEN_KEEPALIVE
    int get_snake_length() { return global_game ? global_game->getSnake().size() : 0; }

    // Export Memory for JS Array Access
    EMSCRIPTEN_KEEPALIVE
    void get_snake_body(int* out_array) {
        if (!global_game) return;
        const auto& snake = global_game->getSnake();
        int idx = 0;
        for (const auto& p : snake) {
            out_array[idx++] = p.x;
            out_array[idx++] = p.y;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    int get_food_x() { return global_game ? global_game->getFood().x : -1; }

    EMSCRIPTEN_KEEPALIVE
    int get_food_y() { return global_game ? global_game->getFood().y : -1; }
}
