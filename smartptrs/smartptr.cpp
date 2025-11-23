// smartptr.cpp
// Modern C++ Smart Pointers Tutorial with Advanced Features
// Build with: g++ -std=c++17 smartptr.cpp -o smartptr

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <algorithm> // for remove_if
#include <cstdio>    // for FILE, fopen, fclose, fprintf

using namespace std;

struct Widget {
    int id;
    string name;
    Widget(int i, string n = "") : id(i), name(move(n)) {
        cout << "Widget(" << id << ") constructed" << (name.empty() ? "" : (": " + name)) << '\n';
    }
    ~Widget() {
        cout << "Widget(" << id << ") destroyed" << (name.empty() ? "" : (": " + name)) << '\n';
    }
    void greet() const { cout << "Hello from Widget(" << id << ")" << (name.empty() ? "" : (": " + name)) << '\n'; }
};

void uniquePtrExample() {
    cout << "--- unique_ptr example ---\n";

    // create a unique_ptr
    unique_ptr<Widget> uptr = make_unique<Widget>(1, "one");
    uptr->greet();

    // transfer ownership with std::move
    unique_ptr<Widget> uptr2 = move(uptr);
    if (!uptr) cout << "uptr is now null after move\n";
    uptr2->greet();

    // custom deleter example (lambda)
    unique_ptr<Widget, function<void(Widget*)>> uptr3(
        new Widget(2, "two"),
        [](Widget* p){ cout << "Custom deleter called for "; delete p; }
    );
    uptr3->greet();

    cout << "End of unique_ptr scope: uptr2 and uptr3 will be destroyed automatically\n";
}

void sharedPtrExample() {
    cout << "\n--- shared_ptr example ---\n";

    // multiple owners
    shared_ptr<Widget> sp1 = make_shared<Widget>(3, "three");
    cout << "use_count after sp1 created: " << sp1.use_count() << '\n';

    {
        shared_ptr<Widget> sp2 = sp1; // shared ownership
        cout << "use_count after sp2 copy: " << sp1.use_count() << '\n';
        sp2->greet();
    }

    cout << "use_count after sp2 goes out of scope: " << sp1.use_count() << '\n';

    // reset decreases count
    sp1.reset();
    cout << "sp1 reset; no owners remain -> Widget destroyed earlier if last owner exited\n";
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
    cout << "\n--- cyclic reference demo ---\n";

    // Cycle with shared_ptr -> causes leak (destructors not called)
    {
        auto a = make_shared<NodeShared>(10);
        auto b = make_shared<NodeShared>(20);
        a->next = b;
        b->next = a; // cycle
        cout << "Created cycle with shared_ptr. When this scope ends, NodeShared destructors will NOT be called because of reference cycle.\n";
    }
    cout << "Exited scope for shared_ptr cycle. If destructors above were not printed, objects leaked due to cycle.\n";

    // Fix using weak_ptr
    {
        auto a = make_shared<NodeWeak>(30);
        auto b = make_shared<NodeWeak>(40);
        a->next = b;
        b->next = a; // no cycle of strong references
        cout << "Created links with weak_ptr; exiting scope will destroy both NodeWeak objects\n";
    }
}

void weakPtrExample() {
    cout << "\n--- weak_ptr example ---\n";

    shared_ptr<Widget> sp = make_shared<Widget>(50, "fifty");
    weak_ptr<Widget> wp = sp; // non-owning
    cout << "shared use_count: " << sp.use_count() << '\n';

    if (auto locked = wp.lock()) { // promote to shared_ptr
        cout << "Locked weak_ptr successfully. use_count: " << locked.use_count() << '\n';
        locked->greet();
    }

    sp.reset(); // destroy the owned Widget
    if (wp.expired()) cout << "weak_ptr is expired after sp.reset()\n";
}

// --- ADVANCED C++ FEATURES ---

// 1. Perfect Forwarding Factory (Modern C++ pattern)
template<typename T, typename... Args>
unique_ptr<T> makeWidget(Args&&... args) {
    // forward preserves lvalue/rvalue-ness of arguments
    return make_unique<T>(forward<Args>(args)...);
}

// 2. enable_shared_from_this - get shared_ptr from 'this'
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

// 6. Observer Pattern with weak_ptr (prevents memory leaks)
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

// 7. RAII with unique_ptr for file handling
struct FileCloser {
    void operator()(FILE* fp) const {
        if (fp) {
            cout << "Closing file\n";
            fclose(fp);
        }
    }
};

void raiiExample() {
    cout << "\n--- RAII: unique_ptr for Resource Management ---\n";
    // FILE* wrapped in unique_ptr with custom deleter
    unique_ptr<FILE, FileCloser> file(fopen("test.txt", "w"));
    if (file) {
        fprintf(file.get(), "Smart pointer RAII\n");
        cout << "File written, will auto-close on scope exit\n";
    }
    // file closes automatically via FileCloser
}

// 8. Move semantics: unique_ptr in vector (move-only type)
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

// 9. Polymorphic deleters with unique_ptr
struct Base { virtual ~Base() { cout << "~Base()\n"; } };
struct Derived : Base { ~Derived() override { cout << "~Derived()\n"; } };

void polymorphicDeletionExample() {
    cout << "\n--- Polymorphic Deletion with Smart Pointers ---\n";
    // Virtual destructor ensures correct cleanup through base pointer
    unique_ptr<Base> ptr = make_unique<Derived>();
    cout << "unique_ptr<Base> holding Derived will call ~Derived then ~Base\n";
    // Automatic cleanup calls Derived destructor first
}

int main() {
    uniquePtrExample();
    sharedPtrExample();
    weakPtrExample();
    cycleDemo();
    advancedFeatures();
    observerPatternExample();
    raiiExample();
    moveSemanticsExample();
    polymorphicDeletionExample();

    cout << "\n=== All examples complete ===\n";
    return 0;
}
