// smartptr.cpp
// Compact tutorial demonstrating std::unique_ptr, std::shared_ptr and std::weak_ptr
// Build with: g++ -std=c++17 smartptr.cpp -o smartptr

#include <iostream>
#include <memory>
#include <vector>
#include <string>

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

int main() {
    uniquePtrExample();
    sharedPtrExample();
    weakPtrExample();
    cycleDemo();

    cout << "\nProgram end. If any destructor messages are missing for the shared_ptr cycle, that's the point of the demo.\n";
    return 0;
}
