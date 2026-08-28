# List — 工业级动态数组容器

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++14](https://img.shields.io/badge/C%2B%2B-14%2F17%2F20-green.svg)]()
[![CI](https://github.com/Hai-ziyi/List/actions/workflows/ci.yml/badge.svg)](https://github.com/Hai-ziyi/List/actions/workflows/ci.yml)
[![Tests](https://img.shields.io/badge/tests-16368%20passed-brightgreen.svg)](test.cpp)

三套头文件、一套测试、中英文文档。C++14 到 C++20 全覆盖，从 DevC++ 5.11 到 GCC 15 都能用。

## 特性

- **双风格 API**：`erase_val`(STL) / `remove`(Python) 并存
- **EBO 空基类优化**：`is_empty` + `is_final` 两条特化路径
- **强异常安全**：realloc 插入路径对可拷贝类型走拷贝（与 `std::vector` 一致），异常后元素序列逐位保留；`emplace` 非 realloc 路径有 undo 回滚
- **平凡类型加速**：`memcpy` 高速路径，避开构造/析构
- **`should_move` trait**：noexcept move 优先，否则 fallback copy；重分配插入路径独立使用 `is_copy_constructible` 判断
- **contiguous_iterator_tag**（C++20）：标准算法可走优化路径
- **自检机制**：debug 下严格校验不变量（`sz_ > cap_` 即报错），无容差
- **debug 边界检查**：`LIST_DEBUG_ASSERT`，`NDEBUG` 下零开销
- **随机对拍测试**：16368 断言，19 种操作 vs `std::vector`，0 失败

## 版本选择

| 文件 | 要求 | 适用场景 | 状态 |
|---|---|---|---|
| `List.hpp` | C++14 | 现代编译器基础版 | **当前维护版本（v1.1.0）** |
| `List_pro.hpp` | C++17/20 | `if constexpr`、`[[nodiscard]]`、`contiguous_iterator_tag`、三路比较 | **legacy**：另一套独立实现，本次缺陷修复未同步，不推荐新项目使用 |
| `List_compatible.hpp` | DevC++ 5.11 (TDM-GCC 4.9.2) | 老旧环境兼容版 | **legacy**：面向 GCC 4.9.2，本机无法回归，含未修复的已知缺陷（含旧 shift_right 坏回滚），不建议使用 |

> 发布物与 `test.cpp` 引用的都是 `List.hpp`。自 v1.1.0 起，**安全底线类修复（`next_cap` 增长、`check_invariants` 严格化、`grow` 守卫、`max_size` 与溢出检查）已同步到三份头文件**；但 `shift_right` 回滚重写与重分配强保证这两个复杂修复仅同步到 `List.hpp`，`List_pro.hpp` / `List_compatible.hpp` 未含（另一套实现，且本机无法回归），后续版本将收敛为单一实现。

## 快速上手

```cpp
#include "List.hpp"
#include <iostream>

int main() {
    List<int> a = {3, 1, 4, 1, 5};
    a.sort();
    a.reverse();
    a.push_back(9);
    a.remove(1);            // Python 风格：删除第一个匹配
    a.erase_all(4);         // STL 风格：删除全部匹配，返回数量

    for (auto& x : a) std::cout << x << " ";
    // 输出: 9 5 3 1

    // 双风格 API 共存
    a.push_front(0);        // Python 风格
    a.insert(a.begin(), -1); // STL 风格

    auto sub = a.slice(1, 3); // Python 风格切片
    return 0;
}
```

## API 速览

### 构造 / 赋值

```cpp
List<int> a;                       // 空
List<int> b(5, 42);                // [42, 42, 42, 42, 42]
List<int> c = {1, 2, 3};           // 初始化列表
List<int> d(c);                    // 拷贝
List<int> e(std::move(c));         // 移动
List<int> f(begin, end);           // 迭代器范围
a = {1, 2};                        // 初始化列表赋值
a = b;                             // 拷贝赋值
a = std::move(b);                  // 移动赋值（noexcept 条件）
```

### 元素访问

| API | 说明 |
|---|---|
| `operator[](i)` | 不检查范围（debug 下 `LIST_DEBUG_ASSERT`） |
| `at(i)` | 越界抛 `std::out_of_range` |
| `front()` / `back()` | 空容器抛异常 |
| `data()` | 裸指针 |

### 容量

| API | 说明 |
|---|---|
| `empty()` / `size()` / `capacity()` | 基础查询 |
| `reserve(n)` | 预留空间 |
| `shrink_to_fit()` | 缩容至当前大小 |
| `clear()` | 清空（noexcept） |
| `max_size()` | 最大容量 |

### 修改器（双风格）

| Python 风格 | STL 风格 | 说明 |
|---|---|---|
| `push_back(x)` | — | 尾部添加 |
| `push_front(x)` | — | 头部添加 |
| `pop_back()` | — | 删除尾部 |
| `pop_front()` | — | 删除头部 |
| — | `insert(pos, x)` | 指定位置插入 |
| — | `emplace(pos, args...)` | 原位构造 |
| — | `emplace_back(args...)` | 尾部原位构造 |
| — | `erase(pos)` / `erase(first, last)` | 按位置删除 |
| `remove(val)` → `bool` | `erase_val(val)` → `size_type` | 删除第一个匹配 |
| `remove_all(val)` | `erase_all(val)` → `size_type` | 删除全部匹配 |
| `remove_if(pred)` → `bool` | `erase_if(pred)` → `size_type` | 按条件删第一个 |
| `remove_all_if(pred)` | `erase_all_if(pred)` → `size_type` | 按条件删全部 |
| `pop(index=npos)` → `T` | — | 取出并删除指定位置 |
| `resize(n)` / `resize(n, val)` | — | 改变大小 |

### 内置算法

| API | 说明 |
|---|---|
| `sort()` / `sort(comp)` | 排序，默认升序 |
| `stable_sort()` | 稳定排序 |
| `is_sorted()` | 是否已排序 |
| `reverse()` | 反转 |
| `rotate_left(n)` / `rotate_right(n)` | 循环移位 |
| `unique()` / `unique_all()` | 去重 |
| `find(val)` / `rfind(val)` → `difference_type` | 查找（-1 表示未找到） |
| `find_if(pred)` → `difference_type` | 条件查找 |
| `contains(val)` → `bool` | 是否包含 |
| `count(val)` / `count_if(pred)` | 计数 |
| `binary_search(val)` | 二分查找（已排序） |
| `lower_bound(val)` / `upper_bound(val)` | 二分边界 |
| `equal_range(val)` | 等值范围 |
| `slice(start, end, step)` | 切片（Python 风格） |
| `for_each(f)` | 遍历 |
| `fill(val)` | 填充 |
| `generate(gen)` | 生成 |
| `iota(start)` | 递增填充 |
| `shuffle(rng)` | 随机打乱 |
| `swap(other)` | 交换 |
| `copy()` | 拷贝副本 |
| `replace` / `replace_all` / `replace_if` / `replace_all_if` | 替换 |
| `all_of` / `any_of` / `none_of` | 谓词判断 |
| `accumulate(init, op)` / `accumulate(init)` | 累积 |
| `min()` / `max()` | 最小/最大值 |

### 运算符

`==` `!=` `<` `>` `<=` `>=` `+` `+=`，全部 `noexcept`。

## 测试

```bash
# 编译并运行对拍测试
# 编译命令（与 CI 一致的严格告警配置）
g++ -std=c++17 -O2 -Wall -Wextra -Werror -o test_list test.cpp
./test_list

# 跳过 debug 自检（跑得更快）
g++ -std=c++17 -O2 -DLIST_NO_SELF_CHECK -o test_list test.cpp
./test_list
```

测试覆盖：

- 基本操作（构造、push/pop、front/back）
- 初始化列表 / 范围构造
- 拷贝与移动（含 moved-from 状态验证）
- 异常安全（at/front/back/pop 空容器）
- 内置算法（sort、reverse、find、binary_search、slice 等）
- 双风格 API 一致性（remove/erase_val、remove_all/erase_all 等）
- string 类型（非平凡类型完整路径）
- 比较运算符 / swap / 连接
- ThrowOnCopy 异常回滚验证
- ThrowOnMove（不可拷贝、move 构造抛）shift_right 回滚路径
- `throw_after = N` 拷贝中途失败回滚
- 重分配强保证：异常后元素序列逐位校验（emplace/insert/insert_range 三条 realloc 路径）
- 确定性 bad_alloc（自定义分配器指定次数后抛），替代非确定性大内存测试
- shrink_to_fit 缩到 1 后 push_back 回归（防 `next_cap` 缺陷复发）
- **随机对拍（1000 次 vs std::vector，19 种操作）**

## CI 与 sanitizer 验证

GitHub Actions 矩阵：g++ / clang++ 双编译器 × C++14/17/20 × debug（`-O0` 无 NDEBUG，跑严格自检）/ release（`-O2 -DNDEBUG`，跑优化路径）/ sanitize（`-O1 -g -DNDEBUG -fsanitize=address,undefined`）。全部配置带 `-Wall -Wextra -Werror`，编译失败或测试非零退出码即失败。

sanitize 配置显式带 `-DNDEBUG`：关闭 debug 自检（`check_invariants`）后，ASan/UBSan 直接抓 release 语义下的堆越界（如 `next_cap` 缺陷曾在 NDEBUG 下静默越界），覆盖 debug 自检抓不到的场景。

本机（MinGW GCC 15.2）无 libasan/libubsan，ASan/UBSan 实证在 CI 的 ubuntu runner 上执行（`ASAN_OPTIONS=detect_leaks=1`、`UBSAN_OPTIONS=halt_on_error=1`）。

## 版本历史

### v1.1.0（当前）

- 修复 `next_cap()` 增长缺陷：`shrink_to_fit()` 缩到 1 后 `push_back` 在 release 模式下堆越界写（`base + (base >> 1)` 在 base=1 时不增长）
- 修复重分配插入路径强异常保证：`emplace_realloc` / `insert_realloc` / `insert_range_realloc` 对可拷贝类型改走拷贝（此前对 `std::string` 等 nothrow-move 类型先 move 再构造新元素，异常时旧值永久丢失）
- 重写 `shift_right_impl` 非平凡分支回滚：旧实现搬移后立即销毁源、回滚时对未构造对象调析构（UB）且继续前搬而非回滚；新实现未初始化区 construct + 重叠区 move 赋值，异常时源元素完整保留
- 严格化 `check_invariants()`：去掉 `sz_ > cap_ + 15` 容差，改为严格 `sz_ > cap_`
- 新增安全守卫（对齐 `std::vector`）：`grow` 增长下限守卫 + `max_size` 检查（`std::length_error`）；`reserve`/重分配插入路径 `max_size` 检查；`insert` 的 `sz_+n` 溢出检查、`resize`/`assign` 的 `max_size` 检查
- **安全底线修复同步三份头文件**：`next_cap`、`check_invariants` 严格化、`grow` 守卫、`max_size`/溢出检查在 `List_pro.hpp` / `List_compatible.hpp` 同样修复；复杂修复（回滚重写、重分配强保证）仅 `List.hpp`
- 新增异常安全测试：ThrowOnMove、`throw_after = N`、重分配强保证逐位校验、确定性 bad_alloc、shrink→push_back 回归、max_size 守卫
- 新增 CI（`.github/workflows/ci.yml`）：双编译器 × 三标准 × 三配置，`-Wall -Wextra -Werror`，sanitize 配置带 `-DNDEBUG`

## 与 std::vector 的关系

List 与 `std::vector` API 兼容，但额外提供：

- Python 风格方法（`remove`、`pop`、`slice`、`push_front`）
- 内置算法（`sort`、`reverse`、`find` 等，无需 `<algorithm>`）
- 异常安全回滚路径（`std::vector` 在 `insert` 非 realloc 路径的异常安全程度由实现定义）
- Debug 自检

## 许可

MIT
