/*******************************************************************************
 * smartptr.cpp
 * Modern C++ Smart Pointers Tutorial - Complete Guide with Advanced Features
 * 
 * TABLE OF CONTENTS:
 * ==================
 * 1. Basic Smart Pointers
 *    - unique_ptr: Exclusive ownership
 *    - shared_ptr: Shared ownership with reference counting
 *    - weak_ptr: Non-owning observer to break cycles
 * 
 * 2. Memory Management Patterns
 *    - Cyclic reference problems and solutions
 *    - Custom deleters for resource management
 *    - RAII (Resource Acquisition Is Initialization)
 * 
 * 3. Advanced Modern C++ Features
 *    - Perfect forwarding and variadic templates
 *    - enable_shared_from_this pattern
 *    - Aliasing constructor
 *    - Observer pattern with weak_ptr
 *    - Polymorphic deletion
 *    - Move semantics with smart pointers
 * 
 * 4. Performance & Best Practices
 *    - make_unique/make_shared vs new
 *    - allocate_shared for custom allocators
 *    - Common pitfalls and how to avoid them
 * 
 * Build: g++ -std=c++17 smartptr.cpp -o smartptr
 * Run:   ./smartptr
 ******************************************************************************/

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <functional>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

//=============================================================================
// HELPER CLASSES FOR DEMONSTRATIONS
//=============================================================================

struct Widget {
    int id;
    string name;
    
    Widget(int i, string n = "") : id(i), name(move(n)) {
        cout << "Widget(" << id << ") constructed";
        if (!name.empty()) cout << ": " << name;
        cout << '\n';
    }
    
    ~Widget() {
        cout << "Widget(" << id << ") destroyed";
        if (!name.empty()) cout << ": " << name;
        cout << '\n';
    }
    
    void greet() const { 
        cout << "Widget(" << id << ") says hello";
        if (!name.empty()) cout << ": " << name;
        cout << '\n';
    }
};

//=============================================================================
// 1. BASIC SMART POINTERS
//=============================================================================

void uniquePtrExample() {
    cout << "\n--- unique_ptr: Exclusive Ownership ---\n";

    // BEST PRACTICE: Use make_unique (exception-safe, single allocation)
    unique_ptr<Widget> uptr = make_unique<Widget>(1, "primary");
    uptr->greet();

    // Transfer ownership with std::move (copy constructor is deleted)
    unique_ptr<Widget> uptr2 = move(uptr);
    if (!uptr) cout << "uptr is now null after move\n";
    uptr2->greet();

    // Custom deleter with lambda (useful for C APIs, file handles, etc.)
    auto customDeleter = [](Widget* p) {
        cout << "Custom deleter called for Widget(" << p->id << ")\n";
        delete p;
    };
    unique_ptr<Widget, decltype(customDeleter)> uptr3(
        new Widget(2, "custom-delete"), customDeleter
    );
    uptr3->greet();

    cout << "End of scope: uptr2 and uptr3 auto-destroyed\n";
}

void sharedPtrExample() {
    cout << "\n--- shared_ptr: Shared Ownership ---\n";

    // BEST PRACTICE: Use make_shared (more efficient - single allocation)
    shared_ptr<Widget> sp1 = make_shared<Widget>(3, "shared");
    cout << "use_count after sp1 created: " << sp1.use_count() << '\n';

    {
        shared_ptr<Widget> sp2 = sp1; // Copy shares ownership
        cout << "use_count after sp2 copy: " << sp1.use_count() << '\n';
        sp2->greet();
    } // sp2 destroyed here

    cout << "use_count after sp2 scope ends: " << sp1.use_count() << '\n';

    // reset() decreases count and destroys if last owner
    sp1.reset();
    cout << "sp1.reset() called - Widget destroyed\n";
}

// Simple cycle demonstration
struct NodeShared {
    int value;
    shared_ptr<NodeShared> next;
    NodeShared(int v) : value(v) { cout << "NodeShared(" << value << ") constructed\n"; }
    ~NodeShared() { cout << "NodeShared(" << value << ") destroyed\n"; }
};

struct NodeWeak {
    int value;
    weak_ptr<NodeWeak> next; // weak pointer breaks cycle
    NodeWeak(int v) : value(v) { cout << "NodeWeak(" << value << ") constructed\n"; }
    ~NodeWeak() { cout << "NodeWeak(" << value << ") destroyed\n"; }
};

void cycleDemo() {
    cout << "\n--- Cyclic Reference Problem & Solution ---\n";

    // PROBLEM: Cycle with shared_ptr causes memory leak
    cout << "\nBAD: Circular shared_ptr references:\n";
    {
        auto a = make_shared<NodeShared>(10);
        auto b = make_shared<NodeShared>(20);
        a->next = b;
        b->next = a; // Creates cycle! use_count never reaches 0
        cout << "Cycle created. Destructors will NOT be called!\n";
    }
    cout << "Memory leak occurred (check: no destructors above)\n";

    // SOLUTION: Use weak_ptr to break the cycle
    cout << "\nGOOD: Using weak_ptr breaks the cycle:\n";
    {
        auto a = make_shared<NodeWeak>(30);
        auto b = make_shared<NodeWeak>(40);
        a->next = b;  // weak_ptr doesn't increase use_count
        b->next = a;  // No strong reference cycle
        cout << "weak_ptr used. Destructors will be called properly.\n";
    }
}

void weakPtrExample() {
    cout << "\n--- weak_ptr: Non-Owning Observer ---\n";

    shared_ptr<Widget> sp = make_shared<Widget>(50, "observed");
    weak_ptr<Widget> wp = sp; // Non-owning observer (doesn't affect use_count)
    cout << "shared use_count: " << sp.use_count() << " (weak_ptr doesn't count)\n";

    // Must lock() to access - promotes weak_ptr to shared_ptr temporarily
    if (auto locked = wp.lock()) {
        cout << "Locked weak_ptr successfully. use_count: " << locked.use_count() << '\n';
        locked->greet();
    } // locked shared_ptr destroyed here

    sp.reset(); // Destroy the owned Widget
    cout << "sp.reset() called - Widget destroyed\n";
    
    if (wp.expired()) {
        cout << "weak_ptr.expired() == true (object no longer exists)\n";
    }
}

//=============================================================================
// 3. ADVANCED MODERN C++ FEATURES
//=============================================================================

// Perfect Forwarding Factory (preserves value categories)
template<typename T, typename... Args>
unique_ptr<T> makeWidget(Args&&... args) {
    // forward<> preserves lvalue/rvalue-ness - enables perfect forwarding
    // This is a common factory pattern in modern C++
    return make_unique<T>(forward<Args>(args)...);
}

// Factory pattern returning polymorphic types
struct Shape {
    virtual ~Shape() = default;
    virtual void draw() const = 0;
};

struct Circle : Shape {
    void draw() const override { cout << "Drawing Circle\n"; }
};

struct Square : Shape {
    void draw() const override { cout << "Drawing Square\n"; }
};

unique_ptr<Shape> createShape(const string& type) {
    if (type == "circle") return make_unique<Circle>();
    if (type == "square") return make_unique<Square>();
    return nullptr;
}

// enable_shared_from_this - get shared_ptr from 'this'
struct Component : enable_shared_from_this<Component> {
    int id;
    Component(int i) : id(i) { cout << "Component(" << id << ") created\n"; }
    ~Component() { cout << "Component(" << id << ") destroyed\n"; }
    
    // Can safely return shared_ptr to self
    shared_ptr<Component> getPtr() { return shared_from_this(); }
};

void advancedFeatures() {
    cout << "\n--- Advanced Modern C++ Features ---\n";
    
    // Perfect forwarding with temporary string
    auto w1 = makeWidget<Widget>(100, "forwarded");
    w1->greet();
    
    // Factory pattern with polymorphic returns
    auto shape1 = createShape("circle");
    auto shape2 = createShape("square");
    if (shape1) shape1->draw();
    if (shape2) shape2->draw();
    
    // enable_shared_from_this usage
    auto comp = make_shared<Component>(200);
    auto ptr = comp->getPtr(); // safe self-reference
    cout << "use_count via shared_from_this: " << ptr.use_count() << "\n";
    
    // 3. Aliasing constructor - shared_ptr to member
    auto widget = make_shared<Widget>(300, "alias-test");
    shared_ptr<int> idPtr(widget, &widget->id); // shares ownership but points to id
    cout << "Aliased id: " << *idPtr << " (use_count=" << idPtr.use_count() << ")\n";
    
    // 4. Array support (C++17+)
    shared_ptr<Widget[]> arr(new Widget[2]{{400, "arr[0]"}, {401, "arr[1]"}});
    arr[0].greet();
    arr[1].greet();
    cout << "Array will auto-delete[] on destruction\n";
    
    // 5. Custom allocator (allocate_shared for efficiency)
    auto w2 = allocate_shared<Widget>(allocator<Widget>(), 500, "allocated");
    cout << "allocate_shared reduces allocations vs make_shared\n";
}

// 6. Resource cache using weak_ptr (avoids keeping objects alive)
class ResourceCache {
    map<string, weak_ptr<Widget>> cache;
public:
    shared_ptr<Widget> getOrCreate(const string& key, int id) {
        auto it = cache.find(key);
        if (it != cache.end()) {
            if (auto sp = it->second.lock()) {
                cout << "Cache hit: " << key << "\n";
                return sp;
            }
        }
        cout << "Cache miss: creating " << key << "\n";
        auto sp = make_shared<Widget>(id, key);
        cache[key] = sp;
        return sp;
    }
};

void resourceCacheExample() {
    cout << "\n--- Resource Cache with weak_ptr ---\n";
    ResourceCache cache;
    
    {
        auto res1 = cache.getOrCreate("texture_1", 100);
        auto res2 = cache.getOrCreate("texture_1", 100); // Cache hit
        cout << "Both references active. use_count: " << res1.use_count() << "\n";
    }
    
    // After scope, resources destroyed
    auto res3 = cache.getOrCreate("texture_1", 100); // Cache miss (expired)
}

// 7. Observer Pattern with weak_ptr (prevents memory leaks)
class Subject {
    vector<weak_ptr<Widget>> observers;
public:
    void attach(shared_ptr<Widget> obs) {
        observers.push_back(obs); // store weak_ptr, doesn't affect lifetime
        cout << "Observer attached. Total: " << observers.size() << '\n';
    }
    
    void notify() {
        cout << "Notifying observers...\n";
        // Remove expired weak_ptrs while notifying
        observers.erase(
            remove_if(observers.begin(), observers.end(),
                [](const weak_ptr<Widget>& wp) {
                    if (auto sp = wp.lock()) {
                        sp->greet(); // notify if still alive
                        return false;
                    }
                    return true; // remove expired
                }),
            observers.end()
        );
    }
};

void observerPatternExample() {
    cout << "\n--- Observer Pattern with weak_ptr ---\n";
    Subject subject;
    
    auto obs1 = make_shared<Widget>(600, "observer1");
    auto obs2 = make_shared<Widget>(601, "observer2");
    
    subject.attach(obs1);
    subject.attach(obs2);
    subject.notify();
    
    obs1.reset(); // observer1 destroyed
    cout << "obs1 destroyed, notifying again:\n";
    subject.notify(); // only obs2 notified
}

// 8. RAII with unique_ptr for file handling
struct FileCloser {
    void operator()(FILE* fp) const {
        if (fp) {
            cout << "Closing file\n";
            fclose(fp);
        }
    }
};

void raiiExample() {
    cout << "\n--- RAII with Custom Deleters ---\n";
    // FILE* wrapped in unique_ptr with custom deleter
    unique_ptr<FILE, FileCloser> file(fopen("test.txt", "w"));
    if (file) {
        fprintf(file.get(), "Smart pointer RAII\n");
        cout << "File written, will auto-close on scope exit\n";
    }
    // file closes automatically via FileCloser
}

// 9. Move semantics: unique_ptr in vector (move-only type)
void moveSemanticsExample() {
    cout << "\n--- Move Semantics with unique_ptr ---\n";
    vector<unique_ptr<Widget>> widgets;
    
    // Can't copy unique_ptr, must move
    widgets.push_back(make_unique<Widget>(700, "vec[0]"));
    widgets.push_back(make_unique<Widget>(701, "vec[1]"));
    
    cout << "Vector of unique_ptrs (move-only). Size: " << widgets.size() << '\n';
    for (auto& w : widgets) w->greet();
    
    // Move from vector
    auto moved = move(widgets[0]);
    cout << "Moved widgets[0] out. Is null? " << (widgets[0] == nullptr) << '\n';
}

// 10. Polymorphic deleters with unique_ptr
struct Base { virtual ~Base() { cout << "~Base()\n"; } };
struct Derived : Base { ~Derived() override { cout << "~Derived()\n"; } };

void polymorphicDeletionExample() {
    cout << "\n--- Polymorphic Deletion ---\n";
    // Virtual destructor ensures correct cleanup through base pointer
    unique_ptr<Base> ptr = make_unique<Derived>();
    cout << "unique_ptr<Base> holding Derived will call ~Derived then ~Base\n";
    // Automatic cleanup calls Derived destructor first
}

//=============================================================================
// 4. THREAD SAFETY & PERFORMANCE
//=============================================================================

// Thread-safe shared_ptr reference counting (atomic operations)
void threadSafetyExample() {
    cout << "\n--- Thread Safety with shared_ptr ---\n";
    
    auto shared = make_shared<Widget>(800, "shared-across-threads");
    cout << "shared_ptr control block uses atomic ref counting\n";
    
    // Safe: copying shared_ptr across threads
    thread t1([shared]() {
        cout << "Thread 1: use_count = " << shared.use_count() << '\n';
        shared->greet();
    });
    
    thread t2([shared]() {
        cout << "Thread 2: use_count = " << shared.use_count() << '\n';
        shared->greet();
    });
    
    t1.join();
    t2.join();
    
    cout << "Threads finished. Final use_count: " << shared.use_count() << '\n';
    cout << "NOTE: While ref-counting is thread-safe, the pointed-to object is NOT!\n";
    cout << "You still need mutex/locks to protect the Widget's data members.\n";
}

//=============================================================================
// 5. BEST PRACTICES & COMMON PITFALLS
//=============================================================================

void bestPracticesAndPitfalls() {
    cout << "\n--- Best Practices & Common Pitfalls ---\n";
    
    cout << "\nBEST PRACTICES:\n";
    cout << "  1. Prefer make_unique/make_shared over new\n";
    cout << "     - Exception safe\n";
    cout << "     - make_shared = 1 allocation (object + control block)\n";
    cout << "     - Cleaner code\n\n";
    
    cout << "  2. Use unique_ptr by default\n";
    cout << "     - Zero overhead when no custom deleter\n";
    cout << "     - Clear ownership semantics\n";
    cout << "     - Can upgrade to shared_ptr if needed\n\n";
    
    cout << "  3. Use weak_ptr to break cycles\n";
    cout << "     - Parent->Child: shared_ptr\n";
    cout << "     - Child->Parent: weak_ptr\n\n";
    
    cout << "  4. Pass smart pointers efficiently:\n";
    cout << "     - By value: transfer ownership\n";
    cout << "     - By const&: observe without copy\n";
    cout << "     - By raw pointer/reference: just use, don't manage\n\n";
    
    cout << "COMMON PITFALLS:\n";
    cout << "  - Don't create multiple shared_ptr from same raw pointer\n";
    cout << "    Widget* raw = new Widget(1);\n";
    cout << "    shared_ptr<Widget> sp1(raw);  // Control block 1\n";
    cout << "    shared_ptr<Widget> sp2(raw);  // Control block 2 -> DOUBLE DELETE!\n\n";
    
    cout << "  - Don't use shared_ptr(this) - use enable_shared_from_this\n\n";
    
    cout << "  - Don't forget virtual destructors with polymorphism\n";
    cout << "    unique_ptr<Base> p = make_unique<Derived>();\n";
    cout << "    // Without virtual ~Base(), only ~Base() is called!\n\n";
    
    cout << "  - Avoid circular shared_ptr references\n";
    cout << "    Use weak_ptr to break cycles in graphs/trees\n\n";
    
    cout << "PERFORMANCE TIPS:\n";
    cout << "  - make_shared is faster (1 allocation vs 2)\n";
    cout << "  - unique_ptr has zero overhead (when not using custom deleter)\n";
    cout << "  - shared_ptr has atomic ref-count overhead\n";
    cout << "  - Pass by const& to avoid ref-count changes\n";
    cout << "  - Reserve vector<unique_ptr> capacity to avoid moves\n";
}

int main() {
    cout << "Modern C++ Smart Pointers Tutorial\n";
    cout << "===================================\n";
    
    uniquePtrExample();
    sharedPtrExample();
    weakPtrExample();
    cycleDemo();
    advancedFeatures();
    resourceCacheExample();
    observerPatternExample();
    raiiExample();
    moveSemanticsExample();
    polymorphicDeletionExample();
    threadSafetyExample();
    bestPracticesAndPitfalls();

    cout << "\n=== All examples complete ===\n";
    return 0;
}
