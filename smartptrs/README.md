# smartptr.cpp — Smart pointer tutorial

This is a compact C++ tutorial demonstrating std::unique_ptr, std::shared_ptr and std::weak_ptr.

What you'll learn:
- Creating and transferring ownership with `std::unique_ptr`.
- Sharing ownership with `std::shared_ptr` and checking `use_count()`.
- Using `std::weak_ptr` to observe an object without owning it and breaking reference cycles.
- A small demo showing how a cycle of `shared_ptr` prevents destruction and how `weak_ptr` fixes it.

Build and run (on Windows with g++/MinGW or WSL bash):

```bash
g++ -std=c++17 smartptr.cpp -o smartptr
./smartptr
```

Notes:
- The program prints construction/destruction messages to show when objects are freed.
- The shared_ptr cycle demo intentionally creates a reference cycle to illustrate leaking behavior (destructors for those objects won't run).

Feel free to tweak the code in `smartptr.cpp` to try more variations.
