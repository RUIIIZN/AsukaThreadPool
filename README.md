# AsukaThreadPool

一个包含三种线程池实现的简单 C++17 项目：

- `FixedThreadPool`：固定线程数，适合稳定负载。
- `CacheThreadPool`：弹性线程数，空闲线程可在超时后回收。
- `WorkStealingThreadPool`：每线程本地队列 + 窃取策略，减少竞争。

## 目录结构

- `ThreadPool/include/`：线程池头文件。
- `ThreadPool/src/`：线程池实现。
- `SyncQueue/`：对应的同步队列实现。
- `test/`：压力测试示例。
- `CMakeLists.txt`：构建配置。

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_STRESS_TESTS=ON
cmake --build build
```

构建输出：
- 静态库：`build/libMyThreadPool.a`
- （可选）压力测试可执行文件：`stress_fixed`、`stress_cache`、`stress_workstealing`（在 `build/` 或生成器对应目录下）

若不需要压力测试，配置时加 `-DBUILD_STRESS_TESTS=OFF`。

## 运行压力测试

```bash
cd build
./stress_fixed
./stress_cache
./stress_workstealing
```

每个测试都会输出计算/IO/混合任务下的耗时信息，可用来观察不同线程池的行为差异。

## 使用示例

```cpp
#include "ThreadPool/include/FixedThreadPool.h"

int main() {
    FixedThreadPool pool(8);
    pool.AddTask([]{ /* do work */ });
}
```
