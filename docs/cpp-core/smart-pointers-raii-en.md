# GeekEden Tech Blog - C++ Smart Pointers & RAII: Memory Management in Game Dev

**Published:** 2026-07-18
**Author:** [@8Dis-like](https://github.com/8Dis-like)

---

In C++ game development, memory leaks and dangling pointers are the "number one killers" leading to server crashes and client instability. Modern C++ heavily advocates for the **RAII (Resource Acquisition Is Initialization)** paradigm, with smart pointers being the ultimate manifestation of this mechanism.

In this article, we will implement smart pointers from scratch to deeply understand the zero-overhead abstraction of `UniquePtr`, the cost of the control block in `SharedPtr`, and how `WeakPtr` resolves circular references. Finally, we will explore their engineering applications in real MMO game scenarios.

## 1. Implementing Smart Pointers from Scratch

To truly understand smart pointers, let's implement our own `UniquePtr` and a simplified `sharedPtr` equipped with a basic control block.

```cpp
#include <bits/stdc++.h>
using namespace std;

// ================= 1. UniquePtr Implementation =================
template<typename T>
class UniquePtr {
private:
    T* ptr;
public:
    explicit UniquePtr(T* p = nullptr) : ptr(p) {}
    
    // Disable copying to guarantee exclusive ownership
    UniquePtr(const UniquePtr&) = delete;  
    UniquePtr& operator=(const UniquePtr&) = delete;  
    
    // Support move semantics to transfer ownership
    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    ~UniquePtr() { delete ptr; }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    T* get() const { return ptr; }
};

// ================= 2. Simple Control Block & sharedPtr Implementation =================
class sharedCounter {
private:
    int count;
public:
    sharedCounter() : count(1) {}
    void addref() { ++count; }
    bool release() {
        if (--count == 0) return true;
        return false;
    }
    int getcount() const { return count; }
};

template<typename T>
class sharedPtr {
private:
    T* ptr;
    sharedCounter* counter; // Shared control block pointer
    
    void cleanup() {
        if (counter && counter->release()) {
            delete ptr;
            delete counter;
        }
        ptr = nullptr;
        counter = nullptr;
    }
public:
    explicit sharedPtr(T* p = nullptr) : ptr(p), counter(nullptr) {
        if (ptr) counter = new sharedCounter();
    }
    
    // Increase reference count upon copying
    sharedPtr(const sharedPtr& other) : ptr(other.ptr), counter(other.counter) {
        if (counter) counter->addref();
    }
    sharedPtr& operator=(const sharedPtr& other) {
        if (this != &other) {
            cleanup();
            ptr = other.ptr;
            counter = other.counter;
            if (counter) counter->addref();
        }
        return *this;
    }

    // Do not increase count upon moving
    sharedPtr(sharedPtr&& other) noexcept : ptr(other.ptr), counter(other.counter) {
        other.ptr = nullptr;
        other.counter = nullptr;
    }
    sharedPtr& operator=(sharedPtr&& other) noexcept {
        if (this != &other) {
            cleanup();
            ptr = other.ptr;
            counter = other.counter;
            other.ptr = nullptr;
            other.counter = nullptr;
        }
        return *this;
    }
    
    ~sharedPtr() { cleanup(); }
    
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    int use_count() const { return counter ? counter->getcount() : 0; }
};
```

---

## 2. Core Mechanics & Performance Analysis

### 1. Why is UniquePtr a Zero-Overhead Abstraction?
In the code above, `UniquePtr` internally contains **only a single raw pointer `T* ptr`**.
- **Size**: `sizeof(UniquePtr<T>) == sizeof(T*)`. It aligns perfectly with a raw pointer in memory, introducing no extra data structures.
- **Performance**: Because there are no atomic operations for reference counting, its construction, dereferencing, and destruction speeds are almost identical to using raw pointers directly (and can be fully inlined by the compiler).
- **Safety**: By using `= delete` to disable the copy constructor and copy assignment operator, it eliminates "Double Free" errors—caused by multiple pointers pointing to the same block of memory—directly at compile time.

### 2. The Cost of SharedPtr and the Control Block
To achieve "sharing", `SharedPtr` must maintain an independent **Control Block** on the heap. In our code, `sharedCounter` serves as a simplified version of this.
The standard `std::shared_ptr` control block also includes a strong reference count, a weak reference count, a custom Deleter, and an Allocator.

> [!WARNING]
> **Thread-Safety Trap**: In our `SharedCounter`, `addRef` and `release` are just ordinary `++` and `--` operations. However, in a real concurrent environment, these operations must be low-level **atomic operations (`std::atomic`)**. Otherwise, multithreading will lead to count discrepancies and crashes. This is the primary reason why `SharedPtr` is noticeably slower than `UniquePtr`.

---

## 3. Circular References: The "Number One Killer" in C++ Memory Management

In interviews and real-world development, the most frequently encountered issue is the **Circular Reference**.

### 1. The Core Problem
When Object A holds a strong reference to Object B, and Object B holds a strong reference back to Object A, both of their reference counts will remain at least 1. Even if they fall out of scope and no external variables point to them, they can never be properly destroyed. This is akin to two people holding hands while falling into a river, neither willing to let go to grab the safety rope on the shore.

```cpp
struct NodeA; struct NodeB;
struct NodeA { shared_ptr<NodeB> bptr; ~NodeA() { cout << "NodeA destroyed!\n"; } };
struct NodeB { shared_ptr<NodeA> aptr; ~NodeB() { cout << "NodeB destroyed!\n"; } };

// Circular Reference Test
{
    auto a = make_shared<NodeA>();
    auto b = make_shared<NodeB>();
    a->bptr = b;
    b->aptr = a;
    // After leaving this scope, no Destroy message is printed in the console. 
    // A severe memory leak has occurred!
}
```

### 2. The Solution: WeakPtr
A `weak_ptr` acts as an **"observer" pointer**.
- It points to an object but **does not own** it.
- It does not increment the strong reference count in the control block, thereby not preventing the object from being destroyed.
- Before it can be used, you must call the `.lock()` method to safely promote it to a `shared_ptr`. If the target object has already been destroyed, `.lock()` simply returns an empty `shared_ptr`.

```cpp
struct Child;
struct Parent {
    shared_ptr<Child> child;
    ~Parent() { cout << "Parent destroyed\n"; }
};
struct Child {
    weak_ptr<Parent> parent; // Weak reference breaks the cycle
    ~Child() { cout << "Child destroyed\n"; }
};

// Fixed Test
{
    auto p = make_shared<Parent>();
    auto c = make_shared<Child>();
    p->child = c;
    c->parent = p;
    // After leaving this scope, both p and c will destruct correctly.
}
```

---

## 4. Practical Application Scenarios in Game Development

Connecting this back to our earlier discussions on **ECS Architecture**, smart pointers have very defined rules of engagement in a game pipeline:

1. **Asset / Resource Management**
   Game data like Textures, Audio, and Meshes are typically massive and shared across multiple entities in a scene. This one-to-many sharing relationship is a perfect use case for `shared_ptr`, ensuring that the resource is automatically unloaded from memory the moment the final entity referencing it is destroyed.

2. **Component Ownership**
   In an ECS architecture, the relationship between an Entity and its Components is usually an exclusive containment, or the components are managed uniformly by Systems and memory pools. In this scenario, **`shared_ptr` should be strictly avoided**. Prefer raw pointers (whose strict lifecycles are managed by the ECS container) or `unique_ptr` to guarantee ultimate memory continuity and access performance.

3. **Scene Graph Parent-Child Nodes**
   In the hierarchical structure of a rendering tree or UI tree, parent nodes often need to hold a `shared_ptr` of their child nodes to control the rendering hierarchy. However, when a child node needs a back-reference to its parent (for event bubbling or coordinate transformations), **it must use a `weak_ptr`**. Otherwise, unloading a scene or destroying a UI panel will immediately result in a memory leak of the entire scene tree.
