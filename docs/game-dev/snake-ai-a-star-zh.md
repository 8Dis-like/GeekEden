# GeekEden 技术博客 - C++ 游戏原型综合开发：贪吃蛇核心逻辑与 A* 算法自动寻路

**发布日期:** 2026-07-20
**作者:** [@8Dis-like](https://github.com/8Dis-like)

---

在经历了前四天关于 OOP 与 ECS 架构对比、智能指针内存管理、STL 底层机制以及并发编程的硬核洗礼后，Day 5 我们将把所有知识进行整合，完成一个经典游戏原型——**贪吃蛇 (Snake Game)** 的综合开发。

不仅如此，为了贴合我们**高性能游戏开发与 AI 系统**的定位，我们还将为其植入 **A* (A-Star) 寻路算法**，让这条贪吃蛇化身为“AI 蛇”，在矩阵空间中自动寻找通往食物的最短安全路径。

## 🎮 在线演示 (Live Demo)
👉 **[点击这里体验 Web 端的 C++ AI 贪吃蛇实时演算动画](https://8Dis-like.github.io/GeekEden/demos/snake_wasm/index.html)**

## 一、 核心架构设计与数据结构选择

在动手前，我们需要为游戏核心数据结构进行选型：
1. **蛇身管理 (`std::deque`)**：
   贪吃蛇的运动本质是“头部插入一个新节点，尾部删除一个老节点”。`std::vector` 在头部插入的时间复杂度是 $O(N)$，显然不合格。而 `std::deque<Point>`（双端队列）支持 $O(1)$ 的头插和尾删，是完美的底层容器。
2. **碰撞检测**：
   - **边界碰撞**：简单的坐标越界判断。
   - **自身碰撞**：遍历蛇身判定。注意由于尾部在下一帧会缩进，因此目标如果是当前尾巴，通常可以视为安全。
   - **食物碰撞**：坐标匹配，命中则积分增加，且**跳过该帧的尾删操作**（实现蛇身生长）。
3. **AI 寻路 (`std::priority_queue`)**：
   A* 算法的核心是维护一个 Open Set，优先探索 $f(n) = g(n) + h(n)$ 最小的节点。我们利用 `std::priority_queue` 配合小顶堆，使用**曼哈顿距离**作为启发式函数 (Heuristic)。

## 二、 核心代码实现

下面是完整的 C++ 游戏原型代码。它不需要任何第三方库即可在控制台直接编译运行，并呈现 AI 自动寻路的动画：

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

// 1. 定义坐标结构
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

// 为 Point 提供 Hash 函数，以便用于 unordered 容器
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

// 2. 定义方向枚举
enum class Direction { Up, Down, Left, Right };

// ==========================================
// 贪吃蛇游戏核心逻辑类
// ==========================================
class SnakeGame {
private:
    int width_, height_;
    std::deque<Point> snake_; // 使用 deque 管理蛇身，O(1) 头插尾删
    Point food_;
    Direction dir_;
    bool game_over_;
    int score_;

public:
    SnakeGame(int w, int h) : width_(w), height_(h), dir_(Direction::Right), game_over_(false), score_(0) {
        snake_.push_back({w / 2, h / 2});     // 蛇头
        snake_.push_back({w / 2 - 1, h / 2}); // 蛇身
        spawnFood();
    }

    bool isGameOver() const { return game_over_; }
    int getScore() const { return score_; }

    void spawnFood() {
        while (true) {
            food_ = { std::rand() % width_, std::rand() % height_ };
            // 确保食物不生成在蛇身上
            bool on_snake = false;
            for (const auto& p : snake_) {
                if (p == food_) { on_snake = true; break; }
            }
            if (!on_snake) break;
        }
    }

    // 设置方向，防止直接反向移动自杀
    void setDirection(Direction new_dir) {
        if (dir_ == Direction::Up && new_dir == Direction::Down) return;
        if (dir_ == Direction::Down && new_dir == Direction::Up) return;
        if (dir_ == Direction::Left && new_dir == Direction::Right) return;
        if (dir_ == Direction::Right && new_dir == Direction::Left) return;
        dir_ = new_dir;
    }

    // 游戏核心更新逻辑
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

        // 1. 边界碰撞检测
        if (next_head.x < 0 || next_head.x >= width_ || next_head.y < 0 || next_head.y >= height_) {
            game_over_ = true;
            return;
        }

        // 2. 自身碰撞检测 (除了尾巴，因为尾巴马上会缩进)
        for (size_t i = 0; i < snake_.size() - 1; ++i) {
            if (next_head == snake_[i]) {
                game_over_ = true;
                return;
            }
        }

        // 头插：蛇往前走一步
        snake_.push_front(next_head);

        // 3. 食物碰撞检测
        if (next_head == food_) {
            score_ += 10;
            spawnFood();
            // 不进行尾删，蛇身自然变长
        } else {
            // 尾删：保持长度不变
            snake_.pop_back(); 
        }
    }

    // ==========================================
    // A* 寻路算法 AI
    // ==========================================
    struct AStarNode {
        Point pt;
        int f; // f = g + h
        bool operator>(const AStarNode& o) const { return f > o.f; } // 小顶堆
    };

    int heuristic(const Point& a, const Point& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y); // 曼哈顿距离
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
                // 越界检测
                if (next.x < 0 || next.x >= width_ || next.y < 0 || next.y >= height_) continue;
                // 身体碰撞检测 (允许目标是尾巴，因为尾巴会移动)
                if (snake_body.count(next) && next != snake_.back()) continue; 

                int tentative_g = g_score[curr] + 1;
                if (g_score.find(next) == g_score.end() || tentative_g < g_score[next]) {
                    came_from[next] = curr;
                    g_score[next] = tentative_g;
                    open_set.push({next, tentative_g + heuristic(next, food_)});
                }
            }
        }

        // 回溯路径，找到走出的第一步
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

    // 简单的控制台渲染
    void render() {
        std::cout << "\033[2J\033[1;1H"; // 清屏
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
    std::srand(1024); // 固定随机种子以便复现
    SnakeGame game(20, 10);
    
    // 模拟游戏循环，执行 20 帧以演示 AI 效果
    for (int i = 0; i < 20 && !game.isGameOver(); ++i) {
        game.aiFindPathToFood(); // AI 决策
        game.update();           // 逻辑更新
        game.render();           // 画面渲染
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "Auto-play Simulation Finished. Final Score: " << game.getScore() << std::endl;
    return 0;
}
```
