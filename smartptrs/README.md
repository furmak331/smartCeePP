# Smart Pointers Tutorial

Concise, runnable examples of Modern C++ smart pointers with advanced features.

## Features Covered

### Basic Smart Pointers
- **unique_ptr**: Exclusive ownership, move semantics, custom deleters
- **shared_ptr**: Shared ownership, reference counting
- **weak_ptr**: Non-owning observer, cycle breaking

### Advanced C++17+ Features
- **Perfect Forwarding**: Template factory with `std::forward`
- **enable_shared_from_this**: Safe self-referencing
- **Aliasing Constructor**: shared_ptr to member with shared ownership
- **Array Support**: `shared_ptr<T[]>` with automatic `delete[]`
- **Custom Allocators**: `allocate_shared` for efficiency
- **Cycle Demo**: Memory leak prevention with weak_ptr

## Build & Run

```bash
g++ -std=c++17 smartptr.cpp -o smartptr
./smartptr
```

## Key Takeaways

1. **Use `make_unique`/`make_shared`** for exception safety
2. **unique_ptr** for single ownership (move-only)
3. **shared_ptr** when multiple owners needed (watch cycles!)
4. **weak_ptr** to break cycles and observe without owning
5. **Perfect forwarding** enables efficient generic factories
6. **enable_shared_from_this** prevents dangling pointers from `this`
