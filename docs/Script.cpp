#if 0
//继承与多态
#include <bits/stdc++.h>
using namespace std;
class GameObject {
public:
    virtual ~GameObject() = default; 
    virtual void update(float dt) = 0;
    virtual void render() = 0;
};

class PlayerCharacter : public GameObject {
    float x, y;
public:
    PlayerCharacter(float x, float y) : x(x), y(y) {}
    void update(float dt) override {
         x += 5.0f * dt; 
         }
    void render() override { 
        cout << "[Render] Player at (" << x << ", " << y << ")\n"; 
        }
};

class NPC : public GameObject {
    int hp;
public:
    explicit NPC(int hp) : hp(hp) {}
    void update(float dt) override {
         hp -= static_cast<int>(dt); 
         }
    void render() override { 
        cout << "[Render] NPC HP: " << hp << "\n"; 
        }
};
int main() {
    //继承多态
    vector<unique_ptr<GameObject>>ptrs;
    ptrs.push_back(make_unique<PlayerCharacter>(10.0f,20.0f));
    ptrs.push_back(make_unique<NPC>(100));
    ptrs.push_back(make_unique<NPC>(50));
    //游戏主循环
    float deltatime=0.016f; //约60FPS
    cout<<"---Game Loop starts---\n";
    for(const auto &ptr:ptrs){
        ptr->update(deltatime);
        ptr->render();
    }
    cout <<"---Game Loop Ends---\n\n";

    cout << "Memory Analysis:";
    cout << "sizeof(GameObject):"<<sizeof(GameObject)<<"bytes\n";
    cout << "sizeof(Player):"<<sizeof(PlayerCharacter)<<"bytes\n";
    cout << "sizeof(NPC):"<<sizeof(NPC)<<"bytes\n";
    return 0;
}
#elif 0
/*
RAII
*/
#include <bits/stdc++.h>
using namespace std;

template<typename T>
class UniquePtr{
private:
    T* ptr;
public:
    explicit UniquePtr(T*p=nullptr):ptr(p){}
    UniquePtr(const UniquePtr&)=delete;  
    UniquePtr&operator=(const UniquePtr&)=delete;  
    UniquePtr(UniquePtr&&other)noexcept:ptr(other.ptr){
        other.ptr=nullptr;
    }
    UniquePtr&operator=(const UniquePtr&&other)noexcept{
        if(this!=&other){
            delete ptr;
            ptr=other.ptr;
            other.ptr=nullptr;
        }
        return *this;
    }
    ~UniquePtr(){
        delete ptr;
    }

    T& operator*()const{return *ptr;}
    T* operator->()const{return ptr;}
    T* get()const{return ptr;}
};

class sharedCounter{
private:
    int count;
public:
    sharedCounter():count(1){}
    void addref(){++count;}
    bool release(){
        if(--count==0){
            return true;
        }else return false;
    }
    int getcount()const{
        return count;
    }
};

template<typename T>
class sharedPtr{
private:
    T* ptr;
    sharedCounter* counter; 
    void cleanup(){
        if(counter&&counter->release()){
            delete ptr;
            delete counter;
        }
        ptr=nullptr;
        counter=nullptr;
    }
public:
    explicit sharedPtr(T*p=nullptr):ptr(p),counter(nullptr){
        if(ptr){
            counter=new sharedCounter();
        }
    }
    sharedPtr(const sharedPtr &other):ptr(other.ptr),counter(other.counter){
        if(counter)counter->addref();
    }
    sharedPtr&operator=(const sharedPtr&other){
        if(this!=&other){
            cleanup();
            ptr=other.ptr;
            counter=other.counter;
            if(counter)counter->addref();
        }return *this;
    }

    sharedPtr(const sharedPtr&&other):ptr(other.ptr),counter(other.counter){
        other.ptr=nullptr;
        other.counter=nullptr;
    }

    sharedPtr&operator=(const sharedPtr&&other)noexcept{
        if(this!=&other){
            cleanup();
            ptr=other.ptr;
            counter=other.counter;
            other.ptr=nullptr;
            other.counter=nullptr;
        }return *this;
    }
    ~sharedPtr(){
        cleanup();
    }
    T&operator*()const{return *ptr;}
    T&operator->()const{return ptr;}
    int use_count()const{
        return counter?counter->getcount():0;
    }
};
//declaration before definition
struct NodeA;
struct NodeB;
struct NodeA{
    shared_ptr<NodeB>bptr;
    ~NodeA(){cout<<"NodeA destroyed!\n";}
};
struct NodeB{
    shared_ptr<NodeA>aptr;
    ~NodeB(){cout<<"NodeB destroyed!\n";}
};

// template<typename T>
// class weakptr{
// private:
//     T*ptr;
//     sharedCounter counter;
// public:
//     weakptr(const sharedPtr<T>&shared):ptr(shared.get()),counter(nullptr){}
//     //too much, too complicated to implement
// };

struct Child;
struct Parent{
    shared_ptr<Child>child;
    ~Parent(){cout<<"Parent destroyed\n";}
};
struct Child{
    weak_ptr<Parent>parent;
    ~Child(){cout<<"Child destroyed\n";}
};


int main() {
    cout<<"---1. UniquePtr Test---\n";
    {
        UniquePtr<int>p1(new int(10));
        cout<<"value:"<<*p1<<endl;
        UniquePtr<int>p2=move(p1); //p1为空
        cout<<"Moved value:"<<*p2<<endl;
    }//p2析构
    cout<<"---2. sharedPtr Test---\n";
    {
        sharedPtr<int>s1(new int(20));
        {
            sharedPtr<int>s2=s1;
            cout<<"Count inside scope:"<<s1.use_count()<<endl;
        }//s2析构
            cout<<"Count outside scope:"<<s1.use_count()<<endl;
    }//s1析构
    cout<<"---3. Circular Reference---\n";
    {
        auto a=make_shared<NodeA>();
        auto b=make_shared<NodeB>();
        a->bptr=b;
        b->aptr=a;
        cout<<"a计数"<<a.use_count()<<endl; //a计数为2: main持有，b持有
        cout<<"b计数"<<b.use_count()<<endl; //b计数为2: main持有，a持有
        //main释放后，计数变为1，永远不归零，内存泄漏
        cout <<"Leak happended!No destroy message printed!\n";
    }
    cout<<"---4. Fix with weak ptr---\n";
    {
        auto p=make_shared<Parent>();
        auto c=make_shared<Child>();
        p->child=c;
        c->parent=p; //弱引用，parent计数仍为1
        cout<<"p计数"<<p.use_count()<<endl; 
        cout<<"c计数"<<c.use_count()<<endl; 
        //离开作用域,p计数归零,parent析构,释放child,c计数归零，child析构，尝试释放parent(已随p释放)
    }

 return 0;
}
#elif 0
#include <bits/stdc++.h>
using namespace std;
class helper{
public:
    int val;
    explicit helper(int val):val(val){
        cout<<"Construct helper("<<val<<")"<<endl;
    }
    helper(const helper&other):val(other.val){
        cout<<"Copy Construct helper(val="<<val<<")"<<endl;
    }
    helper(helper&&other)noexcept:val(other.val){
        cout<<"Move Constrcute helper(val="<<val<<")"<<endl;
    }
    ~helper(){}
};
int main(){
    cout << "任务 1: vector<int> 扩容策略演示" << std::endl;
    {
        vector<int>vec;
        size_t last_cap=0;
        for(int i=0;i<20;i++){
            vec.push_back(i);
            if(vec.capacity()!=last_cap){
                last_cap=vec.capacity();
                cout<<"size:"<<vec.size()<<"capacity:"<<vec.capacity()<<endl;
            }
        }
        /* 
         * 解析：
         * MSVC (Visual Studio): 通常采用 1.5倍 扩容 (1, 2, 3, 4, 6, 9, 13...)
         * GCC (Linux/G++):      通常采用 2倍 扩容 (1, 2, 4, 8, 16...)
         */
    }
    cout << "任务 2: push_back vs emplace_back" << std::endl;
    cout << "--- 测试 A: push_back(MyClass(10)) ---" << std::endl;
    {
        vector<helper>vec;
        vec.reserve(1);
        vec.push_back(helper(10));
    }
    cout << "\n--- 测试 B: emplace_back(10) ---" << std::endl;
    {
        vector<helper>vec;
        vec.reserve(1);
        vec.emplace_back(10);
    }
    cout << "任务 3: 文本单词频率统计器" << std::endl;
    {
        string text = "The quick brown fox jumps over the lazy dog. "
                           "The dog barked at the fox. The fox ran away.";
        unordered_map<string,int>wordscnt;
        istringstream stream(text);
        string word;
        while(stream >>word){
            string cleanword="";
            for(char c:word){
                if(isalpha(c))cleanword+=tolower(c);
            }
            if(!cleanword.empty())wordscnt[word]++;
        }
        vector<pair<string,int>>sortedwords(wordscnt.begin(),wordscnt.end());
        sort(sortedwords.begin(),sortedwords.end(),
        [](const auto&a,const auto&b){return a.second>b.second;});
        int count=0;
        for(const auto&pair:sortedwords){
            if(count>=10)break;
            cout<<pair.first<<":"<<pair.second<<endl;
            count++;
        }
    }
    return 0;
}
#elif 0
#include<bits/stdc++.h>
using namespace std;

class threadsafeaccount{
private:
    double balance;
    mutable mutex mtx;

public:
    explicit threadsafeaccount(double bl):balance(bl){}

    void deposit(double amount){
        lock_guard<mutex>lock(mtx);
        balance+=amount;
        cout<<"deposit +:"<<amount<<"|balance:"<<balance<<endl;
    }

    bool withdraw(double amount){
        lock_guard<mutex>lock(mtx);
        if(balance>=amount){
            balance-=amount;
            cout<<"withdraw -"<<amount<<"|balance:"<<balance<<endl;
            return true;
        }else{
            cout << "[Withdraw Failed] Insufficient funds for " << amount << endl;
            return false;
        }
    }

    double getbalance()const{
        lock_guard<mutex>lock(mtx);
        return balance;
    }
};

class taskqueue{
private:
    queue<int>que;
    mutex mtx;
    condition_variable cv;
    bool stopped=false;
public:
    void push(int val){
        {
        lock_guard<mutex>lock(mtx);
        que.push(val);
    }//释放锁
        cv.notify_one();
    }
    bool pop(int &val){
        unique_lock<mutex>lock(mtx);
        cv.wait(lock,[this]{return !que.empty()||stopped;});
        if(stopped&&que.empty())return false;
        val=que.front();
        que.pop();
        return true;
    }
    void stop(){
        {
        lock_guard<mutex>lock(mtx);
        stopped=true;
        }
    cv.notify_all();
    }
};
// 定义两把全局互斥锁，模拟两个共享资源
std::mutex mutex_a;
std::mutex mutex_b;

void demonstrateDeadlockAndFix() {
    std::cout << "=== 开始演示死锁与修复 ===" << std::endl;

    // ---------------------------------------------------------
    // 第一部分：故意制造死锁 (Deadlock Simulation)
    // 警告：运行此部分会导致程序永久卡死，建议注释掉观察效果
    // ---------------------------------------------------------
    /*
    auto task_deadlock_1 = [&]() {
        std::lock_guard<std::mutex> lock_a(mutex_a); // 线程1 先锁 A
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 增加触发死锁的概率
        std::lock_guard<std::mutex> lock_b(mutex_b); // 线程1 尝试锁 B (此时会被阻塞，因为线程2持有B)
        std::cout << "Thread 1 acquired both locks (Deadlocked)" << std::endl;
    };

    auto task_deadlock_2 = [&]() {
        std::lock_guard<std::mutex> lock_b(mutex_b); // 线程2 先锁 B
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lock_a(mutex_a); // 线程2 尝试锁 A (此时会被阻塞，因为线程1持有A)
        std::cout << "Thread 2 acquired both locks (Deadlocked)" << std::endl;
    };

    std::thread t1(task_deadlock_1);
    std::thread t2(task_deadlock_2);
    t1.join(); 
    t2.join();
    */

    std::cout << "--- 执行修复后的安全逻辑 (C++11/14 兼容版) ---" << std::endl;

    // ---------------------------------------------------------
    // 第二部分：修复死锁 (The Fix)
    // 策略：使用 std::lock 同时获取两把锁，避免循环等待
    // ---------------------------------------------------------

    // 任务 A：模拟转账或操作两个资源
    auto safe_task_1 = [&]() {
        for (int i = 0; i < 3; ++i) {
            // 【C++11/14 兼容写法 - 推荐】
            // 1. std::lock: 使用死锁避免算法同时锁定两个互斥量。
            //    如果其中一个被占用，它会释放已持有的锁并重试，从而打破“循环等待”条件。
            std::lock(mutex_a, mutex_b);

            // 2. std::lock_guard + std::adopt_lock:
            //    adopt_lock 告诉 lock_guard：“锁已经被 std::lock 拿到了，你只需要接管所有权，
            //    负责在作用域结束时自动 unlock 即可，不要再次尝试 lock。”
            std::lock_guard<std::mutex> lk_a(mutex_a, std::adopt_lock);
            std::lock_guard<std::mutex> lk_b(mutex_b, std::adopt_lock);

            // 【C++17 写法 - 仅供参考】
            // std::scoped_lock lock(mutex_a, mutex_b);

            // 模拟业务处理
            std::cout << "[Safe Task 1] Processing... Iteration " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    // 任务 B：以相反顺序请求资源，但在 std::lock 的保护下依然安全
    auto safe_task_2 = [&]() {
        for (int i = 0; i < 3; ++i) {
            // 即使这里逻辑上是想先锁 B 再锁 A，std::lock 也会处理好顺序
            std::lock(mutex_b, mutex_a); 
            
            std::lock_guard<std::mutex> lk_b(mutex_b, std::adopt_lock);
            std::lock_guard<std::mutex> lk_a(mutex_a, std::adopt_lock);

            // 模拟业务处理
            std::cout << "[Safe Task 2] Processing... Iteration " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    std::thread t_fix_1(safe_task_1);
    std::thread t_fix_2(safe_task_2);

    t_fix_1.join();
    t_fix_2.join();

    std::cout << "=== 演示结束，无死锁发生 ===" << std::endl;
}
int main(){
    cout<<"---Thread Safe Account---"<<endl;
    {
        threadsafeaccount account(1000.0);
        vector<thread>threads;
        for(int i=0;i<5;i++){
            threads.emplace_back(&threadsafeaccount::deposit,&account,100.0);
            threads.emplace_back(&threadsafeaccount::withdraw,&account,50.0);
        }
        for(auto &t:threads)t.join();
        cout<<"final balance:"<<account.getbalance()<<endl;
    }
    cout<<"---Producer-Consumer Model---"<<endl;
    {
        taskqueue tq;
        //消费者线程
        thread consumer([&tq](){
            int val;
            while(tq.pop(val)){
                cout<<"consumed:"<<val<<endl;
                this_thread::sleep_for(chrono::milliseconds(50));//模拟处理耗时
            }
            cout<<"Consumer exiting"<<endl;
        });
        //生产者线程
        for(int i=0;i<10;i++){
            tq.push(i);
            this_thread::sleep_for(chrono::milliseconds(20));//模拟处理耗时
        }
        tq.stop();
        consumer.join();
        cout<<endl;
    }
    // cout<<"---Deadlock Demo---"<<endl;
    // demonstrateDeadlockAndFix();
return 0;
}

# elif 0
#include <bits/stdc++.h>

// ================== 基础定义 ==================

// 坐标点结构体，用于表示蛇身、食物或地图上的任意位置
struct Point {
    int x, y;
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    // 【新增】显式定义不等于运算符
    bool operator!=(const Point& other) const {
        return !(*this == other); 
    }

    struct Hash {
        size_t operator()(const Point& p) const {
            return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
        }
    };
};

// 方向枚举
enum class Direction { Up, Down, Left, Right };

// 地图大小常量
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;

// ================== A* 寻路算法相关 ==================

// A* 算法中的节点结构
struct Node {
    Point pos;
    int g_cost; // 从起点到当前点的实际代价
    int f_cost; // f = g + h (总预估代价)
    
    // 优先队列默认是大顶堆，我们需要小顶堆（f_cost越小越优先）
    bool operator>(const Node& other) const {
        return f_cost > other.f_cost;
    }
};

// 计算曼哈顿距离：|x1-x2| + |y1-y2|
int manhattanDistance(const Point& a, const Point& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// ================== 贪吃蛇核心类 ==================

class SnakeGame {
private:
    // 1. 使用 deque 管理蛇身：支持 O(1) 头部插入和尾部删除
    std::deque<Point> snake_body;
    
    Point food_pos;
    Direction current_dir;
    Direction next_dir; // 缓冲下一次按键输入，防止一帧内快速按键导致自杀
    
    bool is_running;
    bool is_ai_mode; // 是否开启 AI 模式

public:
    SnakeGame() : is_running(true), is_ai_mode(true) {
        // 初始化蛇的位置（中间）
        Point start = {MAP_WIDTH / 2, MAP_HEIGHT / 2};
        snake_body.push_back(start);
        snake_body.push_back({start.x, start.y + 1});
        snake_body.push_back({start.x, start.y + 2});
        
        current_dir = Direction::Up;
        next_dir = Direction::Up;
        
        generateFood();
    }

    // 生成随机食物（不能生成在蛇身上）
    void generateFood() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis_x(0, MAP_WIDTH - 1);
        std::uniform_int_distribution<> dis_y(0, MAP_HEIGHT - 1);

        while (true) {
            Point new_food = {dis_x(gen), dis_y(gen)};
            bool on_snake = false;
            for (const auto& segment : snake_body) {
                if (segment == new_food) {
                    on_snake = true;
                    break;
                }
            }
            if (!on_snake) {
                food_pos = new_food;
                break;
            }
        }
    }

    // 获取下一步的移动方向（如果是 AI 模式）
    void updateDirectionAI() {
        Point head = snake_body.front();
        Point target = food_pos;

        // 构建障碍物集合（蛇身即障碍）
        std::unordered_set<Point, Point::Hash> obstacles(snake_body.begin(), snake_body.end());

        // 调用 A* 寻路
        Direction best_move = findPathAStar(head, target, obstacles);
        
        // 如果找不到路径（可能被围住了），尝试向安全的地方苟活一步
        if (best_move == current_dir && !isSafe(head, current_dir, obstacles)) {
             // 简单的Fallback逻辑：尝试其他三个方向
             std::vector<Direction> dirs = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
             for(auto d : dirs) {
                 if(isSafe(head, d, obstacles)) {
                     best_move = d;
                     break;
                 }
             }
        }
        
        next_dir = best_move;
    }

    // 核心移动逻辑
    void move() {
        // 更新当前方向
        current_dir = next_dir;

        Point head = snake_body.front();
        Point new_head = head;

        // 根据方向计算新坐标
        switch (current_dir) {
            case Direction::Up:    new_head.y--; break;
            case Direction::Down:  new_head.y++; break;
            case Direction::Left:  new_head.x--; break;
            case Direction::Right: new_head.x++; break;
        }

        // --- 碰撞检测 ---

        // 1. 边界碰撞
        if (new_head.x < 0 || new_head.x >= MAP_WIDTH || 
            new_head.y < 0 || new_head.y >= MAP_HEIGHT) {
            gameOver("撞墙了！");
            return;
        }

        // 2. 自身碰撞 (注意：不需要检查尾巴，因为尾巴即将移走，除非吃到食物)
        // 这里我们简化处理：先检查除了尾巴以外的身体部分
        for (size_t i = 0; i < snake_body.size() - 1; ++i) {
            if (snake_body[i] == new_head) {
                gameOver("咬到自己了！");
                return;
            }
        }

        // 将新头加入队列前端
        snake_body.push_front(new_head);

        // 3. 食物碰撞检测
        if (new_head == food_pos) {
            // 吃到食物：不移除尾巴（蛇变长），重新生成食物
            generateFood();
        } else {
            // 没吃到食物：移除尾巴，保持长度不变
            snake_body.pop_back();
        }
    }

    // 打印游戏画面
    void render() {
        system("clear"); // Linux/Mac 清屏，Windows 请用 system("cls")
        
        // 创建临时网格用于渲染
        std::vector<std::vector<char>> grid(MAP_HEIGHT, std::vector<char>(MAP_WIDTH, ' '));

        // 绘制蛇
        for (const auto& p : snake_body) {
            grid[p.y][p.x] = 'o';
        }
        // 蛇头特殊标记
        grid[snake_body.front().y][snake_body.front().x] = '@';

        // 绘制食物
        grid[food_pos.y][food_pos.x] = '*';

        // 输出
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                std::cout << grid[y][x];
            }
            std::cout << std::endl;
        }
        std::cout << "Score: " << snake_body.size() - 3 << std::endl;
        std::cout << (is_ai_mode ? "Mode: AI Auto-Pilot (A*)" : "Mode: Manual") << std::endl;
    }

    void run() {
        while (is_running) {
            if (is_ai_mode) {
                updateDirectionAI();
            }
            
            move();
            if(is_running) render();
            
            // 控制游戏速度
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

private:
    void gameOver(const std::string& msg) {
        is_running = false;
        std::cout << "\nGame Over! " << msg << std::endl;
        std::cout << "Final Length: " << snake_body.size() << std::endl;
    }

    // 辅助函数：检查某一步是否安全（不撞墙、不撞自己）
    bool isSafe(const Point& current, Direction dir, const std::unordered_set<Point, Point::Hash>& obstacles) {
        Point next = current;
        switch (dir) {
            case Direction::Up:    next.y--; break;
            case Direction::Down:  next.y++; break;
            case Direction::Left:  next.x--; break;
            case Direction::Right: next.x++; break;
        }
        // 边界检查
        if (next.x < 0 || next.x >= MAP_WIDTH || next.y < 0 || next.y >= MAP_HEIGHT) return false;
        // 障碍检查
        if (obstacles.count(next)) return false;
        return true;
    }

    // A* 寻路具体实现
    // 辅助函数：计算曼哈顿距离
int heuristic(const Point& a, const Point& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Direction findPathAStar(Point start, Point goal, const std::unordered_set<Point, Point::Hash>& obstacles) {
    // 优先队列，存储 {f_score, current_point}
    // f = g (已走步数) + h (预估剩余距离)
    struct Node {
        int f, g;
        Point pos;
        // 小顶堆：f 越小优先级越高；若 f 相同，则 h 越小越优先
        bool operator>(const Node& other) const {
            if (f != other.f) return f > other.f;
            return g < other.g; 
        }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;
    std::unordered_map<Point, Point, Point::Hash> came_from;
    std::unordered_map<Point, int, Point::Hash> g_score;

    // 初始化起点
    g_score[start] = 0;
    open_set.push({heuristic(start, goal), 0, start});

    // 定义四个方向及其对应的坐标偏移量
    // 注意：这里用 vector<pair> 替代了 C++17 的结构化绑定
    std::vector<std::pair<Direction, Point>> moves = {
        {Direction::Up,    {0, -1}},
        {Direction::Down,  {0, 1}},
        {Direction::Left,  {-1, 0}},
        {Direction::Right, {1, 0}}
    };

    while (!open_set.empty()) {
        Node current = open_set.top();
        open_set.pop();

        // 到达终点，开始回溯路径
        if (current.pos == goal) {
            Point path_point = goal;
            // 回溯直到回到起点的“下一步”
            // 使用 .at() 安全获取父节点，避免 C++14 下 operator[] 的隐式构造问题
            while (came_from.at(path_point) != start) {
                path_point = came_from.at(path_point);
            }
            
            // 根据第一步的坐标差值判断方向
            Point next_step = path_point;
            if (next_step.y < start.y) return Direction::Up;
            if (next_step.y > start.y) return Direction::Down;
            if (next_step.x < start.x) return Direction::Left;
            if (next_step.x > start.x) return Direction::Right;
            
            return current_dir; // 理论上不应执行到这里
        }

        // 遍历邻居节点
        for (const auto& move : moves) {
            Direction dir = move.first;      // C++14 写法：通过 .first 访问
            Point offset = move.second;      // C++14 写法：通过 .second 访问
            
            Point neighbor = {current.pos.x + offset.x, current.pos.y + offset.y};

            // 检查边界、障碍物
            if (neighbor.x < 0 || neighbor.x >= MAP_WIDTH || 
                neighbor.y < 0 || neighbor.y >= MAP_HEIGHT ||
                obstacles.count(neighbor)) {
                continue;
            }

            int tentative_g = current.g + 1;

            // 如果找到更短的路径，更新记录
            // 使用 count 检查是否存在，比 find 更简洁
            if (!g_score.count(neighbor) || tentative_g < g_score[neighbor]) {
                came_from[neighbor] = current.pos;
                g_score[neighbor] = tentative_g;
                int f = tentative_g + heuristic(neighbor, goal);
                open_set.push({f, tentative_g, neighbor});
            }
        }
    }

    // 找不到路径，保持当前方向或随机（防止原地不动）
    return current_dir; 
}
};
int main() {
    try {
        SnakeGame game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
#else
#include <iostream>
#include <thread>
#include <semaphore> // C++20 标准引入的 std::binary_semaphore

// 使用 C++20 的二进制信号量（只允许 0 或 1 个通行证）
std::binary_semaphore semA(1); // A 先手，初始给 1 张通行证
std::binary_semaphore semB(0); // B 初始 0 张
std::binary_semaphore semC(0); // C 初始 0 张

const int MAX_NUM = 30;

void printA() {
    for (int i = 1; i <= MAX_NUM; i += 3) {
        semA.acquire(); // 阻塞等待，直到拿到通行证
        
        std::cout << i << " ";
        
        semB.release(); // 执行完毕，给 B 发放通行证
    }
}

void printB() {
    for (int i = 2; i <= MAX_NUM; i += 3) {
        semB.acquire(); 
        
        std::cout << i << " ";
        
        semC.release(); // 给 C 发放通行证
    }
}

void printC() {
    for (int i = 3; i <= MAX_NUM; i += 3) {
        semC.acquire(); 
        
        std::cout << i << " ";
        
        semA.release(); // 给 A 发放通行证，形成闭环
    }
}

int main() {
    std::thread tA(printA);
    std::thread tB(printB);
    std::thread tC(printC);

    tA.join(); tB.join(); tC.join();
    std::cout << std::endl;
    return 0;
}
#endif