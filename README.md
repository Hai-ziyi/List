# List — 工业级动态数组容器

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++14](https://img.shields.io/badge/C%2B%2B-14%2F17%2F20-green.svg)]()
[![Tests](https://img.shields.io/badge/tests-16291%20passed-brightgreen.svg)](test.cpp)

三套头文件、一套测试、中英文文档。C++14 到 C++20 全覆盖，从 DevC++ 5.11 到 GCC 15 都能用。

## 特性

- **双风格 API**：`erase_val`(STL) / `remove`(Python) 并存
- **EBO 空基类优化**：`is_empty` + `is_final` 两条特化路径
- **强异常安全**：realloc 路径完整 try/catch 回滚，`emplace` 非 realloc 路径也有 undo
- **平凡类型加速**：`memcpy` 高速路径，避开构造/析构
- **`should_move` trait**：noexcept move 优先，否则 fallback copy
- **contiguous_iterator_tag**（C++20）：标准算法可走优化路径
- **自检机制**：debug 下校验不变量，带容差避免 shift_right 中间态误报
- **debug 边界检查**：`LIST_DEBUG_ASSERT`，`NDEBUG` 下零开销
- **随机对拍测试**：16291 断言，19 种操作 vs `std::vector`，0 失败

## 版本选择

| 文件 | 要求 | 适用场景 |
|---|---|---|
| `List.hpp` | C++14 | 现代编译器基础版 |
| `List_pro.hpp` | C++17/20 | `if constexpr`、`[[nodiscard]]`、`contiguous_iterator_tag`、三路比较 |
| `List_compatible.hpp` | DevC++ 5.11 (TDM-GCC 4.9.2) | 老旧环境兼容版 |

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
g++ -std=c++17 -O2 -o test_list test.cpp
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
- 大内存分配异常回滚
- **随机对拍（1000 次 vs std::vector，19 种操作）**

## 与 std::vector 的关系

List 与 `std::vector` API 兼容，但额外提供：

- Python 风格方法（`remove`、`pop`、`slice`、`push_front`）
- 内置算法（`sort`、`reverse`、`find` 等，无需 `<algorithm>`）
- 异常安全回滚路径（`std::vector` 在 `insert` 非 realloc 路径的异常安全程度由实现定义）
- Debug 自检

## 许可

MIT
