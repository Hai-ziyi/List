/* test.cpp — List vs std::vector 对拍测试
 * 对每个操作，同时在 List 和 std::vector 上执行，
 * 比较结果是否一致。覆盖正常路径和边界路径。
 *
 * 编译: g++ -std=c++17 -O2 -o test_list test.cpp
 * 运行: ./test_list
 */

#include "List.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>
#include <random>
#include <sstream>
#include <functional>

/* ========== 测试基础设施 ========== */

static int  total  = 0;
static int  passed = 0;
static int  failed = 0;
static bool in_fail = false;

#define CHECK(cond, msg) do { \
    total++; \
    if (!(cond)) { \
        if (!in_fail) { failed++; in_fail = true; } \
        std::cerr << "  FAIL: " << msg << "  (" << #cond << ")" << std::endl; \
    } else { \
        if (!in_fail) passed++; \
    } \
} while(0)

template <typename A, typename B, typename Msg>
void check_eq_impl(A a, B b, const Msg& msg) {
    std::ostringstream full;
    full << msg << "  got=" << a << "  expected=" << b;
    CHECK(a == b, full.str());
}
#define CHECK_EQ(a, b) check_eq_impl(a, b, "")
#define CHECK_EQ3(a, b, msg) check_eq_impl(a, b, msg)

void section(const std::string& name) {
    in_fail = false;
    std::cout << "\n=== " << name << " ===" << std::endl;
}

void summary() {
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "总计: " << total << "  通过: " << passed << "  失败: " << failed << std::endl;
}

/* ========== 对拍核心：随机操作序列 ========== */

template <typename T>
struct Oracle {
    List<T>   l;
    std::vector<T> v;

    void verify(const std::string& ctx) {
        CHECK_EQ3(l.size(), v.size(), ctx + ": size mismatch");

        for (size_t i = 0; i < v.size(); i++) {
            CHECK_EQ3(l[i], v[i], ctx + " [" + std::to_string(i) + "] mismatch");
            CHECK_EQ3(l.at(i), v[i], ctx + " at(" + std::to_string(i) + ") mismatch");
        }
        CHECK(l.empty() == v.empty(), ctx + ": empty() mismatch");

        size_t idx = 0;
        for (auto it = l.begin(); it != l.end(); ++it, ++idx) {
            CHECK_EQ3(*it, v[idx], ctx + " iterator [" + std::to_string(idx) + "]");
        }
        CHECK_EQ3(idx, v.size(), ctx + ": iterator count mismatch");

        idx = v.size();
        for (auto it = l.rbegin(); it != l.rend(); ++it) {
            --idx;
            CHECK_EQ3(*it, v[idx], ctx + " reverse_iterator [" + std::to_string(idx) + "]");
        }

        if (!v.empty()) {
            CHECK(l.data() != nullptr, ctx + ": data() is null on non-empty");
            CHECK_EQ3(l.data()[0], v[0], ctx + ": data()[0]");
        }

        if (!v.empty()) {
            CHECK_EQ3(l.front(), v.front(), ctx + ": front()");
            CHECK_EQ3(l.back(),  v.back(),  ctx + ": back()");
        }
    }

    void run_ops(int count, std::mt19937& rng) {
        std::uniform_int_distribution<int> dist_op(0, 18);
        std::uniform_int_distribution<int> dist_val(-100, 100);

        for (int n = 0; n < count; n++) {
            int op = dist_op(rng);
            T val = static_cast<T>(dist_val(rng));
            T val2 = static_cast<T>(dist_val(rng));
            std::ostringstream ctx;
            ctx << "op#" << n;

            switch (op) {
            case 0:
                l.push_back(val);
                v.push_back(val);
                verify(ctx.str() + " push_back(" + std::to_string(val) + ")");
                break;

            case 1:
                if (!v.empty()) {
                    l.pop_back();
                    v.pop_back();
                    verify(ctx.str() + " pop_back");
                }
                break;

            case 2:
                l.push_front(val);
                v.insert(v.begin(), val);
                verify(ctx.str() + " push_front(" + std::to_string(val) + ")");
                break;

            case 3:
                if (!v.empty()) {
                    l.pop_front();
                    v.erase(v.begin());
                    verify(ctx.str() + " pop_front");
                }
                break;

            case 4:
                if (!v.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, v.size())(rng);
                    auto lit = l.begin() + static_cast<ptrdiff_t>(pos);
                    l.insert(lit, val);
                    v.insert(v.begin() + static_cast<ptrdiff_t>(pos), val);
                    verify(ctx.str() + " insert(@" + std::to_string(pos) + "," + std::to_string(val) + ")");
                }
                break;

            case 5:
                if (!v.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, v.size() - 1)(rng);
                    auto lit = l.begin() + static_cast<ptrdiff_t>(pos);
                    l.erase(lit);
                    v.erase(v.begin() + static_cast<ptrdiff_t>(pos));
                    verify(ctx.str() + " erase(@" + std::to_string(pos) + ")");
                }
                break;

            case 6:
                l.emplace_back(val);
                v.emplace_back(val);
                verify(ctx.str() + " emplace_back(" + std::to_string(val) + ")");
                break;

            case 7:
                if (!v.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, v.size())(rng);
                    auto lit = l.begin() + static_cast<ptrdiff_t>(pos);
                    l.emplace(lit, val);
                    v.emplace(v.begin() + static_cast<ptrdiff_t>(pos), val);
                    verify(ctx.str() + " emplace(@" + std::to_string(pos) + "," + std::to_string(val) + ")");
                }
                break;

            case 8: {
                size_t new_sz = v.empty() ? 3 : v.size() * 2;
                l.resize(new_sz, val);
                v.resize(new_sz, val);
                verify(ctx.str() + " resize(larger)");
                break;
            }

            case 9:
                if (!v.empty() && v.size() > 1) {
                    size_t new_sz = v.size() / 2;
                    l.resize(new_sz);
                    v.resize(new_sz);
                    verify(ctx.str() + " resize(smaller)");
                }
                break;

            case 10: {
                size_t n = std::uniform_int_distribution<size_t>(0, 8)(rng);
                l.assign(n, val);
                v.assign(n, val);
                verify(ctx.str() + " assign(" + std::to_string(n) + "," + std::to_string(val) + ")");
                break;
            }

            case 11:
                if (!v.empty()) {
                    l.reserve(v.size() * 3);
                    v.reserve(v.size() * 3);
                    verify(ctx.str() + " reserve");
                    l.shrink_to_fit();
                    v.shrink_to_fit();
                    verify(ctx.str() + " shrink_to_fit");
                }
                break;

            case 12:
                l.clear();
                v.clear();
                verify(ctx.str() + " clear");
                break;

            case 13:
                if (!v.empty()) {
                    T target = v[v.size() / 2];
                    bool lr = l.remove(target);
                    auto vit = std::find(v.begin(), v.end(), target);
                    bool vr = (vit != v.end());
                    if (vr) v.erase(vit);
                    CHECK_EQ3(lr, vr, ctx.str() + " remove(" + std::to_string(target) + ") return");
                    verify(ctx.str() + " remove(" + std::to_string(target) + ")");
                }
                break;

            case 14:
                if (!v.empty()) {
                    T target = v[v.size() / 3];
                    size_t old_v = v.size();
                    v.erase(std::remove(v.begin(), v.end(), target), v.end());
                    size_t lc = l.erase_all(target);
                    CHECK_EQ3(lc, old_v - v.size(), ctx.str() + " erase_all(" + std::to_string(target) + ") count");
                    verify(ctx.str() + " erase_all(" + std::to_string(target) + ")");
                }
                break;

            case 15:
                if (v.size() >= 2) {
                    l.sort();
                    std::sort(v.begin(), v.end());
                    verify(ctx.str() + " sort");

                    l.reverse();
                    std::reverse(v.begin(), v.end());
                    verify(ctx.str() + " reverse");
                }
                break;

            case 16:
                if (!v.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, v.size() - 1)(rng);
                    T lv = l.pop(pos);
                    T vv = v[pos];
                    v.erase(v.begin() + static_cast<ptrdiff_t>(pos));
                    CHECK_EQ3(lv, vv, ctx.str() + " pop(@" + std::to_string(pos) + ") value");
                    verify(ctx.str() + " pop(@" + std::to_string(pos) + ")");
                }
                break;

            case 17: {
                List<T> l2; l2.push_back(val); l2.push_back(val2);
                std::vector<T> v2; v2.push_back(val); v2.push_back(val2);
                l.swap(l2);
                v.swap(v2);
                verify(ctx.str() + " swap(this)");
                CHECK_EQ3(l2.size(), v2.size(), ctx.str() + " swap(other) size");
                for (size_t i = 0; i < v2.size(); i++)
                    CHECK_EQ3(l2[i], v2[i], ctx.str() + " swap(other) [" + std::to_string(i) + "]");
                l.swap(l2);
                v.swap(v2);
                break;
            }

            case 18: {
                List<T> l_copy(l);
                std::vector<T> v_copy(v);
                CHECK_EQ3(l_copy.size(), v_copy.size(), ctx.str() + " copy ctor size");
                for (size_t i = 0; i < v_copy.size(); i++)
                    CHECK_EQ3(l_copy[i], v_copy[i], ctx.str() + " copy ctor [" + std::to_string(i) + "]");
                break;
            }
            }
        }
    }
};

/* ========== 具体测试场景 ========== */

void test_basic() {
    section("基本操作");

    List<int> l;
    CHECK(l.empty(), "empty after default ctor");
    CHECK_EQ(l.size(), 0);

    l.push_back(10);
    CHECK(!l.empty(), "not empty after push_back");
    CHECK_EQ(l[0], 10);

    l.push_back(20);
    l.push_back(30);
    CHECK_EQ(l.size(), 3);

    CHECK_EQ3(l.front(), 10, "front()");
    CHECK_EQ3(l.back(), 30, "back()");
}

void test_initializer_list() {
    section("初始化列表构造");

    List<int> a = {1, 2, 3, 4, 5};
    CHECK_EQ(a.size(), 5);
    CHECK_EQ(a[0], 1); CHECK_EQ(a[4], 5);

    List<int> b({10, 20});
    CHECK_EQ(b.size(), 2);
    CHECK_EQ(b[0], 10); CHECK_EQ(b[1], 20);
}

void test_range_ctor() {
    section("范围构造");

    std::vector<int> src = {5, 10, 15, 20};
    List<int> l(src.begin(), src.end());
    CHECK_EQ(l.size(), 4);
    CHECK_EQ(l[0], 5); CHECK_EQ(l[3], 20);
}

void test_copy_and_move() {
    section("拷贝与移动");

    List<int> a = {1, 2, 3};
    List<int> b(a);
    CHECK_EQ(b.size(), 3);
    CHECK_EQ(b[0], 1);

    List<int> c(std::move(a));
    CHECK_EQ(c.size(), 3);
    CHECK(a.empty(), "moved-from is empty");
    CHECK_EQ(c[1], 2);

    List<int> d = {7, 8};
    d = c;
    CHECK_EQ(d.size(), 3);
    CHECK_EQ(d[2], 3);

    List<int> e = {0};
    e = std::move(d);
    CHECK_EQ(e.size(), 3);
}

void test_exception_safety() {
    section("异常安全（at/front/back on empty）");

    List<int> empty;
    bool caught = false;
    try { empty.at(0); } catch (std::out_of_range&) { caught = true; }
    CHECK(caught, "at(0) on empty throws");

    caught = false;
    try { empty.front(); } catch (std::out_of_range&) { caught = true; }
    CHECK(caught, "front() on empty throws");

    caught = false;
    try { empty.back(); } catch (std::out_of_range&) { caught = true; }
    CHECK(caught, "back() on empty throws");

    caught = false;
    try { empty.pop_back(); } catch (std::out_of_range&) { caught = true; }
    CHECK(caught, "pop_back() on empty throws");

    caught = false;
    try { empty.pop_front(); } catch (std::out_of_range&) { caught = true; }
    CHECK(caught, "pop_front() on empty throws");
}

void test_algorithms() {
    section("内置算法");

    List<int> l = {3, 1, 4, 1, 5, 9, 2, 6};
    l.sort();
    CHECK(l.is_sorted(), "sort ascending");
    CHECK_EQ(l[0], 1);

    l.reverse();
    CHECK_EQ(l[0], 9);
    CHECK_EQ(l[l.size()-1], 1);

    auto idx = l.find(5);
    CHECK(idx >= 0, "find(5) found");
    CHECK_EQ(l[static_cast<size_t>(idx)], 5);

    CHECK(l.contains(2), "contains(2)");
    CHECK(!l.contains(99), "contains(99)");

    l.sort();
    CHECK(l.binary_search(5), "binary_search(5) on sorted");
    CHECK(!l.binary_search(99), "binary_search(99) on sorted");

    auto sliced = l.slice(1, 4);
    CHECK_EQ(sliced.size(), 3);
    CHECK_EQ(sliced[0], l[1]);

    List<int> r = {1, 2, 3, 2, 4};
    bool found = r.remove(2);
    CHECK(found, "remove(2) returned true");
    CHECK_EQ(r.size(), 4);
    CHECK_EQ(r[1], 3);

    List<int> s = {5, 5, 5};
    size_t cnt = s.erase_val(5);
    CHECK_EQ(cnt, 1);
    CHECK_EQ(s.size(), 2);

    List<int> t = {1, 2, 1, 3, 1};
    cnt = t.erase_all(1);
    CHECK_EQ(cnt, 3);
    CHECK_EQ(t.size(), 2);
    CHECK_EQ(t[0], 2); CHECK_EQ(t[1], 3);
}

void test_remove_if() {
    section("remove_if / erase_if");

    List<int> l = {1, 2, 3, 4, 5, 6};
    bool found = l.remove_if([](int x) { return x == 3; });
    CHECK(found, "remove_if found 3");
    CHECK(!l.contains(3), "3 removed");

    size_t cnt = l.erase_all_if([](int x) { return x % 2 == 0; });
    CHECK_EQ(cnt, 3);
    CHECK_EQ(l.size(), 2);  // {1, 5}
    CHECK_EQ(l[0], 1);
    CHECK_EQ(l[1], 5);
}

void test_strings() {
    section("string 类型");

    List<std::string> l;
    l.push_back("hello");
    l.push_back("world");
    CHECK_EQ(l.size(), 2);
    CHECK_EQ(l[0], "hello");

    l.sort();
    CHECK_EQ(l[0], "hello");
    CHECK_EQ(l[1], "world");

    l.reverse();
    CHECK_EQ(l[0], "world");
}

void test_stress_oracle() {
    section("随机对拍（1000 次操作）");

    std::mt19937 rng(42);
    Oracle<int> oracle;
    oracle.run_ops(1000, rng);

    std::cout << "  对拍完成，最终 size=" << oracle.l.size() << std::endl;
}

void test_operator_compare() {
    section("比较运算符");

    List<int> a = {1, 2, 3};
    List<int> b = {1, 2, 3};
    List<int> c = {1, 2, 4};
    List<int> d = {1, 2};

    CHECK(a == b, "a == b");
    CHECK(!(a != b), "!(a != b)");
    CHECK(a != c, "a != c");
    CHECK(a > d, "a > d (size)");
    CHECK(c > a, "c > a (value)");
    CHECK(d < a, "d < a");
}

void test_std_swap() {
    section("std::swap 特化");

    List<int> a = {1, 2};
    List<int> b = {3, 4, 5};

    using std::swap;
    swap(a, b);

    CHECK_EQ(a.size(), 3);
    CHECK_EQ(a[2], 5);
    CHECK_EQ(b.size(), 2);
    CHECK_EQ(b[1], 2);
}

void test_concat() {
    section("连接操作");

    List<int> a = {1, 2};
    List<int> b = {3, 4};
    auto c = a + b;
    CHECK_EQ(c.size(), 4);
    CHECK_EQ(c[0], 1);
    CHECK_EQ(c[3], 4);

    a += b;
    CHECK_EQ(a.size(), 4);
}

/* ========== 异常安全测试 ========== */

struct ThrowOnCopy {
    int val;
    static int copy_count;
    static int throw_after;
    ThrowOnCopy(int v = 0) : val(v) {}
    ThrowOnCopy(const ThrowOnCopy& o) : val(o.val) {
        if (++copy_count > throw_after && throw_after >= 0)
            throw std::runtime_error("simulated copy failure");
    }
    ThrowOnCopy(ThrowOnCopy&& o) noexcept : val(o.val) { o.val = -1; }
    ThrowOnCopy& operator=(const ThrowOnCopy& o) {
        val = o.val;
        if (++copy_count > throw_after && throw_after >= 0)
            throw std::runtime_error("simulated copy failure");
        return *this;
    }
    ThrowOnCopy& operator=(ThrowOnCopy&& o) noexcept {
        val = o.val; o.val = -1; return *this;
    }
    ~ThrowOnCopy() = default;
    bool operator!=(const ThrowOnCopy& o) const { return val != o.val; }
    bool operator<(const ThrowOnCopy& o) const { return val < o.val; }
};
int ThrowOnCopy::copy_count = 0;
int ThrowOnCopy::throw_after = -1;

void test_exception_safety_rollback() {
    section("异常安全回滚（ThrowOnCopy）");

    // 多元素插入失败时，已有插入要回滚
    ThrowOnCopy::throw_after = -1;
    ThrowOnCopy::copy_count = 0;

    List<ThrowOnCopy> l;
    for (int i = 0; i < 10; i++) l.emplace_back(i);
    CHECK_EQ(l.size(), 10);

    // 插入已有变量（lvalue → 走拷贝构造），第一次拷贝就抛
    ThrowOnCopy::copy_count = 0;
    ThrowOnCopy::throw_after = 0;

    ThrowOnCopy val99(99);
    bool caught = false;
    try {
        l.insert(l.cbegin() + 2, val99);
    } catch (std::runtime_error&) {
        caught = true;
    }
    CHECK(caught, "insert(lvalue) threw on copy failure");
    CHECK_EQ(l.size(), 10);
    CHECK_EQ(l[0].val, 0);
    CHECK_EQ(l[1].val, 1);
    CHECK_EQ(l[2].val, 2);
    CHECK_EQ(l[9].val, 9);

    // push_back 回滚
    ThrowOnCopy::copy_count = 0;
    ThrowOnCopy::throw_after = 0;
    ThrowOnCopy val100(100);
    caught = false;
    try {
        l.push_back(val100);
    } catch (std::runtime_error&) {
        caught = true;
    }
    CHECK(caught, "push_back(lvalue) threw on copy failure");
    CHECK_EQ(l.size(), 10);
}

void test_exception_safety_alloc() {
    section("异常安全分配器（std::string 内部异常）");

    // std::string 的复制可能因内存分配抛异常
    // 验证 List 在批量插入过程中抛异常时回滚
    List<std::string> l;
    for (int i = 0; i < 5; i++) l.emplace_back("hello");

    // 模拟一个超长字符串触发分配失败
    std::string big(1ULL << 25, 'x');  // 32MB 字符串
    bool caught = false;
    try {
        l.push_back(big);  // 可能抛 bad_alloc
    } catch (std::exception&) {
        caught = true;
    }
    // 如果抛了，size 应该不变
    if (caught) {
        CHECK_EQ(l.size(), 5);
    } else {
        CHECK_EQ(l.size(), 6);
    }
}

int main() {
    std::cout << "List 容器对拍测试" << std::endl;
    std::cout << "===================" << std::endl;

    test_basic();
    test_initializer_list();
    test_range_ctor();
    test_copy_and_move();
    test_exception_safety();
    test_algorithms();
    test_remove_if();
    test_strings();
    test_operator_compare();
    test_std_swap();
    test_concat();
    test_exception_safety_rollback();
    test_exception_safety_alloc();
    test_stress_oracle();

    summary();

    return failed > 0 ? 1 : 0;
}
