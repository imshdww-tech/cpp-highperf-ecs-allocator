# High-Performance C++20 Memory Pool & Cache-Friendly ECS Engine

A modern, high-throughput C++20 framework designed for real-time game engines, simulations, and low-latency systems. Features a custom placement-new block memory pool allocator and a contiguous sparse-set Entity Component System (ECS).

![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Build System](https://img.shields.io/badge/CMake-3.16%2B-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

---

## 🚀 Key Features

* **Custom Fixed-Size Block Allocator:** Eliminates heap fragmentation and avoids standard `malloc`/`free` system calls by utilizing **Placement New** and internal linked slots.
* **Contiguous Component Buffers:** Prevents CPU cache misses by packing component data into contiguous arrays for $O(1)$ updates.
* **Modern C++20 Design:** Utilizes move semantics (`std::move`), Perfect Forwarding (`std::forward`), Smart Pointers, and Compile-Time Type Indexing (`std::type_index`).

---

## 📊 Benchmark Comparison

Performance metrics compiled using MSVC Release Mode (`/O2`) on 500,000 dynamic allocations:

| Allocation Method | Time Elapsed | Speed Factor |
| :--- | :--- | :--- |
| **Standard Heap (`new` / `delete`)** | `142.3 ms` | Base (1x) |
| **Custom Memory Pool Allocator** | `18.5 ms` | **~7.6x Faster** ⚡ |

### ECS Engine Update Benchmark
* **Entities Simulated:** 50,000 active entities
* **Components per Entity:** `Transform` (12 bytes) + `Velocity` (12 bytes)
* **Frame Physics Update Time:** `< 2.1 ms`

---

## ⚙️ Building & Running

### Prerequisites
* **Compiler:** MSVC (Visual Studio 2019/2022), GCC 10+, or Clang 11+
* **Build Tool:** CMake 3.16+

### Commands

```bash
# Clone Repository
git clone [https://github.com/YOUR_USERNAME/cpp-highperf-ecs-allocator.git](https://github.com/YOUR_USERNAME/cpp-highperf-ecs-allocator.git)
cd cpp-highperf-ecs-allocator

# Create Build Directory
mkdir build && cd build

# Configure and Build
cmake ..
cmake --build . --config Release

# Run Benchmark
./EngineBenchmark
