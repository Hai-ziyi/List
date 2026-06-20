/* List_compatible.hpp -- Industrial-grade dynamic array container (DevC++ 5.11 Compatible)
 * Requires: Partial C++14 support (TDM-GCC 4.9.2)
 * Note: This version includes polyfills for missing C++14 features in old compilers.
 */
#ifndef LIST_COMPATIBLE_HPP
#define LIST_COMPATIBLE_HPP

#include <cstddef>
#include <cstring>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <memory>
#include <iterator>
#include <limits>
#include <functional>
#include <numeric>
#include <random>

/* ========== Self-checking infrastructure (active unless NDEBUG) ========== */
#ifndef LIST_NO_SELF_CHECK
#  ifdef NDEBUG
#    define LIST_NO_SELF_CHECK
#  endif
#endif

#ifndef LIST_ASSERT_INV
#  ifdef LIST_NO_SELF_CHECK
#    define LIST_ASSERT_INV() ((void)0)
#  else
#    define LIST_ASSERT_INV() do { check_invariants(); } while (0)
#  endif
#endif

/* ========== Polyfill for std::is_trivially_copyable (missing in GCC < 5) ========== */
#if defined(__GNUC__) && __GNUC__ < 5
namespace std {
template<typename T>
struct is_trivially_copyable : public integral_constant<bool,
    __has_trivial_copy(T) && __has_trivial_destructor(T)> {};
}
#endif

/* ========== Forward declaration ========== */
template <typename T, typename Alloc>
class List;

/* ========== Detail: EBO allocator base ========== */
namespace list_detail {

template <typename Alloc, bool IsEmpty>
struct AllocBaseImpl;

template <typename Alloc>
struct AllocBaseImpl<Alloc, true> : private Alloc {
    AllocBaseImpl() noexcept(noexcept(Alloc()))
        : Alloc() {}
    explicit AllocBaseImpl(const Alloc& a) noexcept
        : Alloc(a) {}
    explicit AllocBaseImpl(Alloc&& a) noexcept
        : Alloc(std::move(a)) {}

    Alloc&       get() noexcept       { return *this; }
    const Alloc& get() const noexcept { return *this; }
};

template <typename Alloc>
struct AllocBaseImpl<Alloc, false> {
    Alloc alloc_;

    AllocBaseImpl() noexcept(noexcept(Alloc()))
        : alloc_() {}
    explicit AllocBaseImpl(const Alloc& a) noexcept
        : alloc_(a) {}
    explicit AllocBaseImpl(Alloc&& a) noexcept
        : alloc_(std::move(a)) {}

    Alloc&       get() noexcept       { return alloc_; }
    const Alloc& get() const noexcept { return alloc_; }
};

template <typename Alloc>
using AllocBase = AllocBaseImpl<Alloc,
    std::is_empty<Alloc>::value && !std::is_final<Alloc>::value>;

} // namespace list_detail

/* ========== Core template ========== */
template <typename T, typename Alloc = std::allocator<T>>
class List : private list_detail::AllocBase<Alloc> {
    using alloc_base = list_detail::AllocBase<Alloc>;
    using alloc_traits = std::allocator_traits<Alloc>;

public:
    /* ---- Type aliases ---- */
    using value_type             = T;
    using allocator_type         = Alloc;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = typename alloc_traits::pointer;
    using const_pointer          = typename alloc_traits::const_pointer;

    /* ---- Iterator classes ---- */
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator() noexcept : ptr_(nullptr) {}
        explicit iterator(pointer p) noexcept : ptr_(p) {}

        reference   operator*() const noexcept { return *ptr_; }
        pointer     operator->() const noexcept { return ptr_; }
        reference   operator[](difference_type n) const noexcept { return ptr_[n]; }

        iterator& operator++() noexcept { ++ptr_; return *this; }
        iterator  operator++(int) noexcept { iterator t(*this); ++ptr_; return t; }
        iterator& operator--() noexcept { --ptr_; return *this; }
        iterator  operator--(int) noexcept { iterator t(*this); --ptr_; return t; }

        iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        iterator  operator+(difference_type n) const noexcept { return iterator(ptr_ + n); }
        iterator  operator-(difference_type n) const noexcept { return iterator(ptr_ - n); }
        difference_type operator-(const iterator& o) const noexcept { return ptr_ - o.ptr_; }

        bool operator==(const iterator& o) const noexcept { return ptr_ == o.ptr_; }
        bool operator!=(const iterator& o) const noexcept { return ptr_ != o.ptr_; }
        bool operator< (const iterator& o) const noexcept { return ptr_ <  o.ptr_; }
        bool operator> (const iterator& o) const noexcept { return ptr_ >  o.ptr_; }
        bool operator<=(const iterator& o) const noexcept { return ptr_ <= o.ptr_; }
        bool operator>=(const iterator& o) const noexcept { return ptr_ >= o.ptr_; }

        friend iterator operator+(difference_type n, const iterator& it) noexcept {
            return iterator(it.ptr_ + n);
        }
    private:
        pointer ptr_;
        friend class List;
    };

    class const_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        const_iterator() noexcept : ptr_(nullptr) {}
        explicit const_iterator(pointer p) noexcept : ptr_(p) {}
        const_iterator(const iterator& it) noexcept : ptr_(it.operator->()) {}

        reference   operator*() const noexcept { return *ptr_; }
        pointer     operator->() const noexcept { return ptr_; }
        reference   operator[](difference_type n) const noexcept { return ptr_[n]; }

        const_iterator& operator++() noexcept { ++ptr_; return *this; }
        const_iterator  operator++(int) noexcept { const_iterator t(*this); ++ptr_; return t; }
        const_iterator& operator--() noexcept { --ptr_; return *this; }
        const_iterator  operator--(int) noexcept { const_iterator t(*this); --ptr_; return t; }

        const_iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        const_iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        const_iterator  operator+(difference_type n) const noexcept { return const_iterator(ptr_ + n); }
        const_iterator  operator-(difference_type n) const noexcept { return const_iterator(ptr_ - n); }
        difference_type operator-(const const_iterator& o) const noexcept { return ptr_ - o.ptr_; }

        bool operator==(const const_iterator& o) const noexcept { return ptr_ == o.ptr_; }
        bool operator!=(const const_iterator& o) const noexcept { return ptr_ != o.ptr_; }
        bool operator< (const const_iterator& o) const noexcept { return ptr_ <  o.ptr_; }
        bool operator> (const const_iterator& o) const noexcept { return ptr_ >  o.ptr_; }
        bool operator<=(const const_iterator& o) const noexcept { return ptr_ <= o.ptr_; }
        bool operator>=(const const_iterator& o) const noexcept { return ptr_ >= o.ptr_; }

        friend const_iterator operator+(difference_type n, const const_iterator& it) noexcept {
            return const_iterator(it.ptr_ + n);
        }
    private:
        pointer ptr_;
        friend class List;
    };

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    enum class Order { Ascending, Descending };

    /* ========== Constructors / Destructor ========== */
    List() noexcept(noexcept(Alloc()))
        : List(Alloc()) {}

    explicit List(const Alloc& a) noexcept
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {}

    explicit List(size_type n, const Alloc& a = Alloc())
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        if (n == 0) return;
        reserve(n);
        for (; sz_ < n; ++sz_) alloc_traits::construct(alloc(), &data_[sz_]);
        LIST_ASSERT_INV();
    }

    List(size_type n, const T& val, const Alloc& a = Alloc())
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        if (n == 0) return;
        reserve(n);
        for (; sz_ < n; ++sz_) alloc_traits::construct(alloc(), &data_[sz_], val);
        LIST_ASSERT_INV();
    }

    List(std::initializer_list<T> il, const Alloc& a = Alloc())
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        if (il.size() == 0) return;
        reserve(il.size());
        for (const T& x : il) alloc_traits::construct(alloc(), &data_[sz_++], x);
        LIST_ASSERT_INV();
    }

    template <typename InputIt,
              typename = typename std::enable_if<
                  std::is_base_of<std::input_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>::value>::type>
    List(InputIt first, InputIt last, const Alloc& a = Alloc())
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        init_from_range(first, last,
            typename std::iterator_traits<InputIt>::iterator_category());
    }

    List(const List& o)
        : alloc_base(alloc_traits::select_on_container_copy_construction(o.alloc())),
          data_(nullptr), sz_(0), cap_(0) {
        if (o.sz_ == 0) return;
        reserve(o.sz_);
        copy_data(data_, o.data_, o.sz_, typename std::is_trivially_copyable<T>::type());
        sz_ = o.sz_;
        LIST_ASSERT_INV();
    }

    List(const List& o, const Alloc& a)
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        if (o.sz_ == 0) return;
        reserve(o.sz_);
        for (; sz_ < o.sz_; ++sz_)
            alloc_traits::construct(alloc(), &data_[sz_], o.data_[sz_]);
        LIST_ASSERT_INV();
    }

    List(List&& o) noexcept
        : alloc_base(std::move(o.alloc())),
          data_(o.data_), sz_(o.sz_), cap_(o.cap_) {
        o.data_ = nullptr; o.sz_ = 0; o.cap_ = 0;
    }

    List(List&& o, const Alloc& a)
        : alloc_base(a), data_(nullptr), sz_(0), cap_(0) {
        if (a == o.alloc()) {
            data_ = o.data_; sz_ = o.sz_; cap_ = o.cap_;
            o.data_ = nullptr; o.sz_ = 0; o.cap_ = 0;
        } else {
            reserve(o.sz_);
            for (; sz_ < o.sz_; ++sz_)
                alloc_traits::construct(alloc(), &data_[sz_], std::move(o.data_[sz_]));
        }
        LIST_ASSERT_INV();
    }

    ~List() {
        destroy_all();
        raw_dealloc(data_, cap_);
    }

    /* ========== Assignment ========== */
    List& operator=(const List& o) {
        if (this == &o) return *this;
        propagate_assign_alloc(o,
            typename alloc_traits::propagate_on_container_copy_assignment::type());
        if (o.sz_ <= cap_) {
            size_type i = 0;
            for (; i < o.sz_ && i < sz_; ++i) data_[i] = o.data_[i];
            for (; i < o.sz_; ++i) alloc_traits::construct(alloc(), &data_[i], o.data_[i]);
            for (; i < sz_; ++i) destroy_at(&data_[i]);
            sz_ = o.sz_;
            LIST_ASSERT_INV();
            return *this;
        }
        auto new_data = raw_alloc(o.sz_);
        size_type i = 0;
        try {
            for (; i < o.sz_; ++i)
                alloc_traits::construct(alloc(), &new_data[i], o.data_[i]);
        } catch (...) {
            for (size_type j = 0; j < i; ++j) destroy_at(&new_data[j]);
            raw_dealloc(new_data, o.sz_);
            throw;
        }
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = new_data; sz_ = o.sz_; cap_ = o.sz_;
        LIST_ASSERT_INV();
        return *this;
    }

    List& operator=(List&& o)
        noexcept(alloc_traits::propagate_on_container_move_assignment::value ||
                 alloc_traits::is_always_equal::value) {
        if (this == &o) return *this;
        return move_assign_impl(o,
            typename alloc_traits::propagate_on_container_move_assignment::type());
    }

    List& operator=(std::initializer_list<T> il) {
        assign(il.begin(), il.end());
        return *this;
    }

    /* ---- assign ---- */
    void assign(size_type n, const T& val) {
        if (n > cap_) {
            List tmp(n, val, alloc());
            tmp.swap(*this);
        } else {
            size_type i = 0;
            for (; i < sz_ && i < n; ++i) data_[i] = val;
            for (; i < n; ++i) alloc_traits::construct(alloc(), &data_[i], val);
            for (size_type j = n; j < sz_; ++j) destroy_at(&data_[j]);
            sz_ = n;
        }
        LIST_ASSERT_INV();
    }

    template <typename InputIt,
              typename = typename std::enable_if<
                  std::is_base_of<std::input_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>::value>::type>
    void assign(InputIt first, InputIt last) {
        assign_range(first, last,
            typename std::iterator_traits<InputIt>::iterator_category());
    }

    void assign(std::initializer_list<T> il) {
        assign(il.begin(), il.end());
    }

    /* ========== Allocator access ========== */
    allocator_type get_allocator() const noexcept { return alloc(); }

    /* ========== Element access ========== */
    reference at(size_type i) {
        if (i >= sz_) throw std::out_of_range("List::at");
        return data_[i];
    }
    const_reference at(size_type i) const {
        if (i >= sz_) throw std::out_of_range("List::at");
        return data_[i];
    }
    reference       operator[](size_type i) noexcept       { return data_[i]; }
    const_reference operator[](size_type i) const noexcept { return data_[i]; }

    reference front() {
        if (empty()) throw std::out_of_range("List::front");
        return data_[0];
    }
    const_reference front() const {
        if (empty()) throw std::out_of_range("List::front");
        return data_[0];
    }
    reference back() {
        if (empty()) throw std::out_of_range("List::back");
        return data_[sz_ - 1];
    }
    const_reference back() const {
        if (empty()) throw std::out_of_range("List::back");
        return data_[sz_ - 1];
    }

    pointer       data() noexcept       { return data_; }
    const_pointer data() const noexcept { return data_; }

    /* ========== Iterators ========== */
    iterator               begin() noexcept        { return iterator(data_); }
    const_iterator         begin() const noexcept  { return const_iterator(data_); }
    const_iterator         cbegin() const noexcept { return const_iterator(data_); }
    iterator               end() noexcept          { return iterator(data_ + sz_); }
    const_iterator         end() const noexcept    { return const_iterator(data_ + sz_); }
    const_iterator         cend() const noexcept   { return const_iterator(data_ + sz_); }
    reverse_iterator       rbegin() noexcept       { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept{ return const_reverse_iterator(end()); }
    reverse_iterator       rend() noexcept         { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept   { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept  { return const_reverse_iterator(begin()); }

    /* ========== Capacity ========== */
    bool      empty() const noexcept    { return sz_ == 0; }
    size_type size() const noexcept     { return sz_; }
    size_type capacity() const noexcept { return cap_; }

    size_type max_size() const noexcept {
        return std::min(alloc_traits::max_size(alloc()),
                        static_cast<size_type>(std::numeric_limits<difference_type>::max()));
    }

    void reserve(size_type new_cap) {
        if (new_cap <= cap_) return;
        auto new_data = raw_alloc(new_cap);
        relocate(new_data, new_cap, typename std::is_nothrow_move_constructible<T>::type());
    }

    void shrink_to_fit() {
        if (sz_ == cap_) return;
        if (sz_ == 0) { raw_dealloc(data_, cap_); data_ = nullptr; cap_ = 0; return; }
        auto new_data = raw_alloc(sz_);
        relocate(new_data, sz_, typename std::is_nothrow_move_constructible<T>::type());
    }

    void clear() noexcept { destroy_all(); }

    /* ========== Modifiers ========== */
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        if (sz_ >= cap_) grow();
        T* p = &data_[sz_];
        alloc_traits::construct(alloc(), p, std::forward<Args>(args)...);
        ++sz_;
        LIST_ASSERT_INV();
        return *p;
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx > sz_) throw std::out_of_range("List::emplace");
        if (sz_ >= cap_) {
            auto new_cap = next_cap();
            if (new_cap < sz_ + 1) new_cap = sz_ + 1;
            auto new_data = raw_alloc(new_cap);
            emplace_realloc(new_data, new_cap, idx, std::forward<Args>(args)...);
            return iterator(data_ + idx);
        }
        if (idx < sz_) shift_right(idx, 1);
        alloc_traits::construct(alloc(), &data_[idx], std::forward<Args>(args)...);
        ++sz_;
        LIST_ASSERT_INV();
        return iterator(data_ + idx);
    }

    template <typename... Args>
    reference emplace_front(Args&&... args) {
        emplace(cbegin(), std::forward<Args>(args)...);
        return front();
    }

    iterator insert(const_iterator pos, const T& val) { return emplace(pos, val); }
    iterator insert(const_iterator pos, T&& val)      { return emplace(pos, std::move(val)); }

    iterator insert(const_iterator pos, size_type n, const T& val) {
        if (n == 0) return iterator(data_ + (pos - cbegin()));
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx > sz_) throw std::out_of_range("List::insert");
        auto needed = sz_ + n;
        if (needed > cap_) {
            auto new_cap = std::max(next_cap(), needed);
            auto new_data = raw_alloc(new_cap);
            insert_realloc(new_data, new_cap, idx, n, val);
            return iterator(data_ + idx);
        }
        shift_right(idx, n);
        size_type i = 0;
        try {
            for (; i < n; ++i)
                alloc_traits::construct(alloc(), &data_[idx + i], val);
        } catch (...) {
            for (size_type j = 0; j < i; ++j) destroy_at(&data_[idx + j]);
            undo_shift_right(idx, n);
            throw;
        }
        sz_ += n;
        LIST_ASSERT_INV();
        return iterator(data_ + idx);
    }

    template <typename InputIt,
              typename = typename std::enable_if<
                  std::is_base_of<std::input_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>::value>::type>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        return insert_range(pos, first, last,
            typename std::iterator_traits<InputIt>::iterator_category());
    }

    iterator insert(const_iterator pos, std::initializer_list<T> il) {
        return insert(pos, il.begin(), il.end());
    }

    void push_back(const T& val) { emplace_back(val); }
    void push_back(T&& val)      { emplace_back(std::move(val)); }

    void pop_back() {
        if (empty()) throw std::out_of_range("List::pop_back");
        destroy_at(&data_[--sz_]);
        LIST_ASSERT_INV();
    }

    void push_front(const T& val) { emplace(cbegin(), val); }
    void push_front(T&& val)      { emplace(cbegin(), std::move(val)); }

    void pop_front() {
        if (empty()) throw std::out_of_range("List::pop_front");
        erase(cbegin());
    }

    iterator erase(const_iterator pos) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx >= sz_) throw std::out_of_range("List::erase");
        erase_impl(idx, typename std::is_trivially_copyable<T>::type());
        --sz_;
        LIST_ASSERT_INV();
        return iterator(data_ + idx);
    }

    iterator erase(const_iterator first, const_iterator last) {
        size_type beg = static_cast<size_type>(first - cbegin());
        size_type end = static_cast<size_type>(last - cbegin());
        if (beg > end || end > sz_) throw std::out_of_range("List::erase");
        size_type n = end - beg;
        if (n == 0) return iterator(data_ + beg);
        erase_range_impl(beg, end, n, typename std::is_trivially_copyable<T>::type());
        sz_ -= n;
        LIST_ASSERT_INV();
        return iterator(data_ + beg);
    }

    void remove(const T& val) {
        for (size_type i = 0; i < sz_; ++i) {
            if (data_[i] == val) {
                erase(cbegin() + static_cast<difference_type>(i));
                return;
            }
        }
        throw std::out_of_range("List::remove");
    }

    void remove_all(const T& val) {
        size_type j = 0;
        for (size_type i = 0; i < sz_; ++i) {
            if (!(data_[i] == val)) {
                if (i != j) data_[j] = std::move(data_[i]);
                ++j;
            }
        }
        for (size_type i = j; i < sz_; ++i) destroy_at(&data_[i]);
        sz_ = j;
        LIST_ASSERT_INV();
    }

    T pop(size_type index = npos) {
        if (sz_ == 0) throw std::out_of_range("List::pop");
        if (index == npos) index = sz_ - 1;
        if (index >= sz_) throw std::out_of_range("List::pop");
        T res = std::move(data_[index]);
        erase(cbegin() + static_cast<difference_type>(index));
        return res;
    }

    void resize(size_type n) {
        if (n < sz_) {
            for (size_type i = n; i < sz_; ++i) destroy_at(&data_[i]);
            sz_ = n;
            LIST_ASSERT_INV();
            return;
        }
        if (n > cap_) reserve(n);
        for (; sz_ < n; ++sz_) alloc_traits::construct(alloc(), &data_[sz_]);
        LIST_ASSERT_INV();
    }

    void resize(size_type n, const T& val) {
        if (n < sz_) {
            for (size_type i = n; i < sz_; ++i) destroy_at(&data_[i]);
            sz_ = n;
            LIST_ASSERT_INV();
            return;
        }
        if (n > cap_) reserve(n);
        for (; sz_ < n; ++sz_) alloc_traits::construct(alloc(), &data_[sz_], val);
        LIST_ASSERT_INV();
    }

    void swap(List& o)
        noexcept(alloc_traits::propagate_on_container_swap::value ||
                 alloc_traits::is_always_equal::value) {
        if (alloc_traits::propagate_on_container_swap::value) {
            using std::swap;
            swap(alloc(), o.alloc());
        }
        using std::swap;
        swap(data_, o.data_);
        swap(sz_, o.sz_);
        swap(cap_, o.cap_);
    }

    /* ========== Algorithms ========== */
    void sort(Order order = Order::Ascending) {
        if (order == Order::Ascending)
            std::sort(begin(), end());
        else
            std::sort(begin(), end(), std::greater<T>());
    }

    template <typename Compare>
    void sort(Compare comp) { std::sort(begin(), end(), comp); }

    void stable_sort() { std::stable_sort(begin(), end()); }

    template <typename Compare>
    void stable_sort(Compare comp) { std::stable_sort(begin(), end(), comp); }

    bool is_sorted(Order order = Order::Ascending) const noexcept {
        if (sz_ <= 1) return true;
        if (order == Order::Ascending) {
            for (size_type i = 1; i < sz_; ++i)
                if (data_[i] < data_[i - 1]) return false;
        } else {
            for (size_type i = 1; i < sz_; ++i)
                if (data_[i] > data_[i - 1]) return false;
        }
        return true;
    }

    template <typename Compare>
    bool is_sorted(Compare comp) const noexcept {
        if (sz_ <= 1) return true;
        for (size_type i = 1; i < sz_; ++i)
            if (comp(data_[i], data_[i - 1])) return false;
        return true;
    }

    void reverse() noexcept { std::reverse(data_, data_ + sz_); }

    void rotate_left(size_type n) {
        if (sz_ <= 1) return;
        n %= sz_;
        if (n == 0) return;
        std::rotate(data_, data_ + n, data_ + sz_);
    }

    void rotate_right(size_type n) {
        if (sz_ <= 1) return;
        n %= sz_;
        if (n == 0) return;
        std::rotate(data_, data_ + sz_ - n, data_ + sz_);
    }

    void swap_at(size_type i, size_type j) {
        if (i >= sz_ || j >= sz_) throw std::out_of_range("List::swap_at");
        using std::swap;
        swap(data_[i], data_[j]);
    }

    void unique() {
        if (sz_ <= 1) return;
        size_type j = 0;
        for (size_type i = 1; i < sz_; ++i) {
            if (!(data_[i] == data_[j])) {
                ++j;
                if (i != j) data_[j] = std::move(data_[i]);
            }
        }
        for (size_type i = j + 1; i < sz_; ++i) destroy_at(&data_[i]);
        sz_ = j + 1;
        LIST_ASSERT_INV();
    }

    void unique_all() {
        if (sz_ <= 1) return;
        sort();
        unique();
    }

    difference_type find(const T& val) const noexcept {
        for (size_type i = 0; i < sz_; ++i)
            if (data_[i] == val) return static_cast<difference_type>(i);
        return static_cast<difference_type>(-1);
    }

    template <typename Pred>
    difference_type find_if(Pred pred) const {
        for (size_type i = 0; i < sz_; ++i)
            if (pred(data_[i])) return static_cast<difference_type>(i);
        return static_cast<difference_type>(-1);
    }

    difference_type rfind(const T& val) const noexcept {
        for (size_type i = sz_; i-- > 0;)
            if (data_[i] == val) return static_cast<difference_type>(i);
        return static_cast<difference_type>(-1);
    }

    template <typename Pred>
    difference_type rfind_if(Pred pred) const {
        for (size_type i = sz_; i-- > 0;)
            if (pred(data_[i])) return static_cast<difference_type>(i);
        return static_cast<difference_type>(-1);
    }

    size_type index(const T& val) const {
        for (size_type i = 0; i < sz_; ++i)
            if (data_[i] == val) return i;
        throw std::out_of_range("List::index");
    }

    size_type count(const T& val) const noexcept {
        size_type c = 0;
        for (size_type i = 0; i < sz_; ++i)
            if (data_[i] == val) ++c;
        return c;
    }

    template <typename Pred>
    size_type count_if(Pred pred) const {
        size_type c = 0;
        for (size_type i = 0; i < sz_; ++i)
            if (pred(data_[i])) ++c;
        return c;
    }

    bool contains(const T& val) const noexcept { return find(val) != static_cast<difference_type>(-1); }

    template <typename Pred>
    bool remove_if(Pred pred) {
        for (size_type i = 0; i < sz_; ++i) {
            if (pred(data_[i])) {
                erase(cbegin() + static_cast<difference_type>(i));
                return true;
            }
        }
        return false;
    }
        }
        throw std::out_of_range("List::remove_if");
    }

    template <typename Pred>
    void remove_all_if(Pred pred) {
        size_type j = 0;
        for (size_type i = 0; i < sz_; ++i) {
            if (!pred(data_[i])) {
                if (i != j) data_[j] = std::move(data_[i]);
                ++j;
            }
        }
        for (size_type i = j; i < sz_; ++i) destroy_at(&data_[i]);
        sz_ = j;
        LIST_ASSERT_INV();
    }

    void replace(const T& old_val, const T& new_val) {
        for (size_type i = 0; i < sz_; ++i) {
            if (data_[i] == old_val) {
                data_[i] = new_val;
                return;
            }
        }
    }

    void replace_all(const T& old_val, const T& new_val) {
        for (size_type i = 0; i < sz_; ++i)
            if (data_[i] == old_val) data_[i] = new_val;
    }

    template <typename Pred>
    void replace_if(Pred pred, const T& new_val) {
        for (size_type i = 0; i < sz_; ++i) {
            if (pred(data_[i])) {
                data_[i] = new_val;
                return;
            }
        }
    }

    template <typename Pred>
    void replace_all_if(Pred pred, const T& new_val) {
        for (size_type i = 0; i < sz_; ++i)
            if (pred(data_[i])) data_[i] = new_val;
    }

    size_type lower_bound(const T& val) const {
        size_type l = 0, r = sz_;
        while (l < r) {
            size_type m = l + (r - l) / 2;
            if (data_[m] < val) l = m + 1;
            else r = m;
        }
        return l;
    }

    size_type upper_bound(const T& val) const {
        size_type l = 0, r = sz_;
        while (l < r) {
            size_type m = l + (r - l) / 2;
            if (val < data_[m]) r = m;
            else l = m + 1;
        }
        return l;
    }

    bool binary_search(const T& val) const {
        size_type i = lower_bound(val);
        return i < sz_ && !(val < data_[i]);
    }

    std::pair<size_type, size_type> equal_range(const T& val) const {
        return std::make_pair(lower_bound(val), upper_bound(val));
    }

    void fill(const T& val) { for (size_type i = 0; i < sz_; ++i) data_[i] = val; }

    template <typename UnaryFunc>
    List& for_each(UnaryFunc f) {
        for (size_type i = 0; i < sz_; ++i) f(data_[i]);
        return *this;
    }

    template <typename UnaryFunc>
    List& for_each(UnaryFunc f) const {
        for (size_type i = 0; i < sz_; ++i) f(data_[i]);
        return const_cast<List&>(*this);
    }

    template <typename Gen>
    void generate(Gen g) { for (size_type i = 0; i < sz_; ++i) data_[i] = g(); }

    void iota(T value) { for (size_type i = 0; i < sz_; ++i) data_[i] = value++; }

    template <typename RNG>
    void shuffle(RNG&& rng) { std::shuffle(data_, data_ + sz_, std::forward<RNG>(rng)); }

    template <typename Pred>
    bool all_of(Pred pred) const {
        for (size_type i = 0; i < sz_; ++i)
            if (!pred(data_[i])) return false;
        return true;
    }

    template <typename Pred>
    bool any_of(Pred pred) const {
        for (size_type i = 0; i < sz_; ++i)
            if (pred(data_[i])) return true;
        return false;
    }

    template <typename Pred>
    bool none_of(Pred pred) const {
        for (size_type i = 0; i < sz_; ++i)
            if (pred(data_[i])) return false;
        return true;
    }

    T min() const {
        if (empty()) throw std::out_of_range("List::min");
        T res = data_[0];
        for (size_type i = 1; i < sz_; ++i)
            if (data_[i] < res) res = data_[i];
        return res;
    }

    T max() const {
        if (empty()) throw std::out_of_range("List::max");
        T res = data_[0];
        for (size_type i = 1; i < sz_; ++i)
            if (data_[i] > res) res = data_[i];
        return res;
    }

    T sum() const {
        if (empty()) throw std::out_of_range("List::sum");
        T res = data_[0];
        for (size_type i = 1; i < sz_; ++i) res += data_[i];
        return res;
    }

    T product() const {
        if (empty()) throw std::out_of_range("List::product");
        T res = data_[0];
        for (size_type i = 1; i < sz_; ++i) res *= data_[i];
        return res;
    }

    double average() const {
        if (empty()) throw std::out_of_range("List::average");
        return static_cast<double>(sum()) / static_cast<double>(sz_);
    }

    T median() const {
        if (empty()) throw std::out_of_range("List::median");
        List tmp(*this);
        tmp.sort();
        if (sz_ % 2 == 1) return tmp[sz_ / 2];
        return (tmp[sz_ / 2 - 1] + tmp[sz_ / 2]) / T(2);
    }

    template <typename BinaryOp>
    T accumulate(T init, BinaryOp op) const {
        for (size_type i = 0; i < sz_; ++i)
            init = op(init, data_[i]);
        return init;
    }

    T accumulate(T init = T()) const {
        for (size_type i = 0; i < sz_; ++i)
            init += data_[i];
        return init;
    }

    List slice(size_type s, size_type e, size_type step = 1) const {
        if (step == 0) throw std::invalid_argument("List::slice: step cannot be zero");
        if (s > sz_) s = sz_;
        if (e > sz_) e = sz_;
        List res;
        if (s >= e) return res;
        size_type est = (e > s) ? ((e - s - 1) / step + 1) : 0;
        res.reserve(est);
        for (size_type i = s; i < e; i += step)
            res.emplace_back(data_[i]);
        LIST_ASSERT_INV();
        return res;
    }

    List copy() const { return List(*this); }

    bool starts_with(const List& o) const {
        if (o.sz_ > sz_) return false;
        for (size_type i = 0; i < o.sz_; ++i)
            if (data_[i] != o.data_[i]) return false;
        return true;
    }

    bool ends_with(const List& o) const {
        if (o.sz_ > sz_) return false;
        for (size_type i = 0; i < o.sz_; ++i)
            if (data_[sz_ - o.sz_ + i] != o.data_[i]) return false;
        return true;
    }

    List operator+(const List& o) const {
        List res;
        res.reserve(sz_ + o.sz_);
        for (auto& x : *this) res.emplace_back(x);
        for (auto& x : o) res.emplace_back(x);
        return res;
    }

    List& operator+=(const List& o) {
        auto needed = sz_ + o.sz_;
        if (needed > cap_) reserve(needed);
        for (auto& x : o) emplace_back(x);
        return *this;
    }

    bool operator==(const List& o) const noexcept {
        if (sz_ != o.sz_) return false;
        for (size_type i = 0; i < sz_; ++i)
            if (data_[i] != o.data_[i]) return false;
        return true;
    }
    bool operator!=(const List& o) const noexcept { return !(*this == o); }
    bool operator<(const List& o) const noexcept {
        size_type n = std::min(sz_, o.sz_);
        for (size_type i = 0; i < n; ++i) {
            if (data_[i] < o.data_[i]) return true;
            if (o.data_[i] < data_[i]) return false;
        }
        return sz_ < o.sz_;
    }
    bool operator>(const List& o) const noexcept { return o < *this; }
    bool operator<=(const List& o) const noexcept { return !(o < *this); }
    bool operator>=(const List& o) const noexcept { return !(*this < o); }

    /* ========== Self-checking (debug only) ========== */
#ifndef LIST_NO_SELF_CHECK
    void check_invariants() const {
        if (sz_ > cap_)
            throw std::logic_error("List invariant violated: size > capacity");
        if (cap_ == 0 && data_ != nullptr)
            throw std::logic_error("List invariant violated: cap==0 but data!=nullptr");
        if (cap_ > 0 && data_ == nullptr)
            throw std::logic_error("List invariant violated: cap>0 but data==nullptr");
    }
#endif

private:
    /* ---- Constants ---- */
    static constexpr size_type min_cap = 4;
    static constexpr size_type npos    = static_cast<size_type>(-1);

    /* ---- Members ---- */
    pointer   data_;
    size_type sz_;
    size_type cap_;

    /* ---- Allocator access (EBO-aware) ---- */
    Alloc&       alloc() noexcept       { return alloc_base::get(); }
    const Alloc& alloc() const noexcept { return alloc_base::get(); }

    /* ---- Raw allocation ---- */
    pointer raw_alloc(size_type n) {
        if (n == 0) return nullptr;
        return alloc_traits::allocate(alloc(), n);
    }
    void raw_dealloc(pointer p, size_type n) noexcept {
        if (p) alloc_traits::deallocate(alloc(), p, n);
    }

    /* ---- Destroy helpers (tag dispatching) ---- */
    static void destroy_at_impl(pointer /*p*/, std::true_type) {}
    static void destroy_at_impl(pointer p, std::false_type) { p->~T(); }
    static void destroy_at(pointer p) noexcept {
        destroy_at_impl(p, typename std::is_trivially_destructible<T>::type());
    }

    void destroy_all_impl(std::true_type) noexcept {}
    void destroy_all_impl(std::false_type) noexcept {
        for (size_type i = 0; i < sz_; ++i) data_[i].~T();
    }
    void destroy_all() noexcept {
        destroy_all_impl(typename std::is_trivially_destructible<T>::type());
        sz_ = 0;
    }

    /* ---- Move-or-copy helper for reallocation ---- */
    using should_move = typename std::conditional<
        std::is_nothrow_move_constructible<T>::value || !std::is_copy_constructible<T>::value,
        std::true_type, std::false_type>::type;

    static T&&       pick_move(T& x, std::true_type) noexcept  { return static_cast<T&&>(x); }
    static const T&  pick_move(T& x, std::false_type) noexcept { return x; }
    static auto      pick_move(T& x) -> decltype(pick_move(x, should_move())) {
        return pick_move(x, should_move());
    }

    /* ---- Trivial copy helpers ---- */
    void copy_data(pointer dst, const_pointer src, size_type n, std::true_type) {
        std::memcpy(dst, src, n * sizeof(T));
    }
    void copy_data(pointer dst, const_pointer src, size_type n, std::false_type) {
        for (size_type i = 0; i < n; ++i)
            alloc_traits::construct(alloc(), &dst[i], src[i]);
    }

    /* ---- Propagate alloc helpers ---- */
    void propagate_assign_alloc(const List& o, std::true_type) {
        if (alloc() != o.alloc()) {
            destroy_all();
            raw_dealloc(data_, cap_);
            data_ = nullptr; cap_ = 0;
            alloc() = o.alloc();
        }
    }
    void propagate_assign_alloc(const List&, std::false_type) {}

    List& move_assign_impl(List& o, std::true_type) {
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = nullptr; cap_ = 0;
        alloc() = std::move(o.alloc());
        data_ = o.data_; sz_ = o.sz_; cap_ = o.cap_;
        o.data_ = nullptr; o.sz_ = 0; o.cap_ = 0;
        LIST_ASSERT_INV();
        return *this;
    }
    List& move_assign_impl(List& o, std::false_type) {
        if (alloc() == o.alloc()) {
            destroy_all();
            raw_dealloc(data_, cap_);
            data_ = nullptr; cap_ = 0;
            data_ = o.data_; sz_ = o.sz_; cap_ = o.cap_;
            o.data_ = nullptr; o.sz_ = 0; o.cap_ = 0;
        } else {
            if (o.sz_ > cap_) {
                destroy_all();
                raw_dealloc(data_, cap_);
                data_ = raw_alloc(o.sz_);
                cap_ = o.sz_;
            } else {
                destroy_all();
            }
            sz_ = 0;
            for (; sz_ < o.sz_; ++sz_)
                alloc_traits::construct(alloc(), &data_[sz_], std::move(o.data_[sz_]));
        }
        LIST_ASSERT_INV();
        return *this;
    }

    /* ---- Erase implementations ---- */
    void erase_impl(size_type idx, std::true_type) {
        std::memmove(data_ + idx, data_ + idx + 1, (sz_ - idx - 1) * sizeof(T));
    }
    void erase_impl(size_type idx, std::false_type) {
        destroy_at(data_ + idx);
        for (size_type i = idx; i + 1 < sz_; ++i)
            data_[i] = std::move(data_[i + 1]);
        destroy_at(data_ + sz_ - 1);
    }

    void erase_range_impl(size_type beg, size_type /*end*/, size_type n, std::true_type) {
        std::memmove(data_ + beg, data_ + beg + n, (sz_ - beg - n) * sizeof(T));
    }
    void erase_range_impl(size_type beg, size_type /*end*/, size_type n, std::false_type) {
        for (size_type i = beg; i + n < sz_; ++i)
            data_[i] = std::move(data_[i + n]);
        for (size_type i = sz_ - n; i < sz_; ++i)
            destroy_at(&data_[i]);
    }

    /* ---- Shift right (make room) ---- */
    void shift_right(size_type from, size_type count) {
        shift_right_impl(from, count, typename std::is_trivially_copyable<T>::type());
    }
    void shift_right_impl(size_type from, size_type count, std::true_type) {
        std::memmove(data_ + from + count, data_ + from, (sz_ - from) * sizeof(T));
    }
    void shift_right_impl(size_type from, size_type count, std::false_type) {
        size_type i = sz_;
        try {
            for (; i > from; --i) {
                alloc_traits::construct(alloc(), &data_[i + count - 1], std::move(data_[i - 1]));
                destroy_at(&data_[i - 1]);
            }
        } catch (...) {
            for (size_type j = sz_ + count - 1; j >= i + count - 1 && j >= count; --j) {
                destroy_at(&data_[j]);
                if (j == i + count - 1) break;
            }
            for (size_type j = from; j < i; ++j) {
                if (j + count >= sz_ + count) break;
                alloc_traits::construct(alloc(), &data_[j + count], std::move(data_[j]));
                destroy_at(&data_[j]);
            }
            throw;
        }
    }

    /* ---- Undo shift_right (for exception rollback) ---- */
    void undo_shift_right(size_type idx, size_type n) {
        undo_shift_right_impl(idx, n, typename std::is_trivially_copyable<T>::type());
    }
    void undo_shift_right_impl(size_type idx, size_type n, std::true_type) {
        std::memmove(data_ + idx, data_ + idx + n, (sz_ - idx) * sizeof(T));
    }
    void undo_shift_right_impl(size_type idx, size_type n, std::false_type) {
        for (size_type j = 0; j < sz_ - idx; ++j) {
            alloc_traits::construct(alloc(), &data_[idx + j], std::move(data_[idx + n + j]));
            destroy_at(&data_[idx + n + j]);
        }
    }

    /* ---- Relocate ---- */
    void relocate(pointer new_data, size_type new_cap, std::true_type) {
        auto old_sz = sz_;
        relocate_trivial(new_data, typename std::is_trivially_copyable<T>::type());
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ = old_sz;
        LIST_ASSERT_INV();
    }
    void relocate(pointer new_data, size_type new_cap, std::false_type) {
        auto old_sz = sz_;
        size_type i = 0;
        try {
            for (; i < old_sz; ++i)
                alloc_traits::construct(alloc(), &new_data[i], pick_move(data_[i]));
        } catch (...) {
            for (size_type j = 0; j < i; ++j) destroy_at(&new_data[j]);
            raw_dealloc(new_data, new_cap);
            throw;
        }
        for (size_type j = 0; j < old_sz; ++j) destroy_at(&data_[j]);
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ = old_sz;
        LIST_ASSERT_INV();
    }

    void relocate_trivial(pointer new_data, std::true_type) {
        std::memcpy(new_data, data_, sz_ * sizeof(T));
    }
    void relocate_trivial(pointer new_data, std::false_type) {
        std::uninitialized_copy(std::make_move_iterator(data_),
                                std::make_move_iterator(data_ + sz_),
                                new_data);
        for (size_type i = 0; i < sz_; ++i) destroy_at(&data_[i]);
    }

    /* ---- Optimized insert with reallocation ---- */
    void insert_realloc(pointer new_data, size_type new_cap, size_type idx, size_type n, const T& val) {
        insert_realloc_dispatch(new_data, new_cap, idx, n, val,
            typename std::is_trivially_copyable<T>::type());
    }

    void insert_realloc_dispatch(pointer new_data, size_type new_cap, size_type idx,
                                  size_type n, const T& val, std::true_type) {
        size_type old_sz = sz_;
        std::memcpy(new_data, data_, idx * sizeof(T));
        std::uninitialized_fill_n(new_data + idx, n, val);
        std::memcpy(new_data + idx + n, data_ + idx, (old_sz - idx) * sizeof(T));
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ = old_sz + n;
        LIST_ASSERT_INV();
    }

    void insert_realloc_dispatch(pointer new_data, size_type new_cap, size_type idx,
                                  size_type n, const T& val, std::false_type) {
        size_type old_sz = sz_;
        size_type constructed = 0;
        try {
            for (; constructed < idx; ++constructed)
                alloc_traits::construct(alloc(), &new_data[constructed],
                                        pick_move(data_[constructed]));
            for (size_type i = 0; i < n; ++i, ++constructed)
                alloc_traits::construct(alloc(), &new_data[idx + i], val);
            for (size_type j = 0; j < old_sz - idx; ++j, ++constructed)
                alloc_traits::construct(alloc(), &new_data[idx + n + j],
                                        pick_move(data_[idx + j]));
        } catch (...) {
            for (size_type j = 0; j < constructed; ++j) destroy_at(&new_data[j]);
            raw_dealloc(new_data, new_cap);
            throw;
        }
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ = old_sz + n;
        LIST_ASSERT_INV();
    }

    template <typename... Args>
    void emplace_realloc(pointer new_data, size_type new_cap, size_type idx, Args&&... args) {
        size_type old_sz = sz_;
        size_type constructed = 0;
        try {
            for (; constructed < idx; ++constructed)
                alloc_traits::construct(alloc(), &new_data[constructed],
                                        pick_move(data_[constructed]));
            alloc_traits::construct(alloc(), &new_data[idx], std::forward<Args>(args)...);
            ++constructed;
            for (size_type j = 0; j < old_sz - idx; ++j, ++constructed)
                alloc_traits::construct(alloc(), &new_data[idx + 1 + j],
                                        pick_move(data_[idx + j]));
        } catch (...) {
            for (size_type j = 0; j < constructed; ++j) destroy_at(&new_data[j]);
            raw_dealloc(new_data, new_cap);
            throw;
        }
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ = old_sz + 1;
        LIST_ASSERT_INV();
    }

    /* ---- Range insert helpers ---- */
    template <typename InputIt>
    iterator insert_range(const_iterator pos, InputIt first, InputIt last,
                          std::input_iterator_tag) {
        List tmp(first, last, alloc());
        return insert(pos, tmp.begin(), tmp.end());
    }

    template <typename FwdIt>
    iterator insert_range(const_iterator pos, FwdIt first, FwdIt last,
                          std::forward_iterator_tag) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        auto n = static_cast<size_type>(std::distance(first, last));
        if (n == 0) return iterator(data_ + idx);
        if (idx > sz_) throw std::out_of_range("List::insert");

        auto needed = sz_ + n;
        if (needed > cap_) {
            auto new_cap = std::max(next_cap(), needed);
            auto new_data = raw_alloc(new_cap);
            insert_range_realloc(new_data, new_cap, idx, first, last, n);
            return iterator(data_ + idx);
        }

        shift_right(idx, n);
        size_type i = 0;
        FwdIt it = first;
        try {
            for (; it != last; ++it, ++i)
                alloc_traits::construct(alloc(), &data_[idx + i], *it);
        } catch (...) {
            for (size_type j = 0; j < i; ++j) destroy_at(&data_[idx + j]);
            undo_shift_right(idx, n);
            throw;
        }
        sz_ += n;
        LIST_ASSERT_INV();
        return iterator(data_ + idx);
    }

    template <typename FwdIt>
    void insert_range_realloc(pointer new_data, size_type new_cap, size_type idx,
                              FwdIt first, FwdIt last, size_type /*n*/) {
        size_type constructed = 0;
        try {
            for (; constructed < idx; ++constructed)
                alloc_traits::construct(alloc(), &new_data[constructed],
                                        pick_move(data_[constructed]));
            for (auto it = first; it != last; ++it, ++constructed)
                alloc_traits::construct(alloc(), &new_data[constructed], *it);
            for (size_type j = 0; j < sz_ - idx; ++j, ++constructed)
                alloc_traits::construct(alloc(), &new_data[constructed],
                                        pick_move(data_[idx + j]));
        } catch (...) {
            for (size_type j = 0; j < constructed; ++j) destroy_at(&new_data[j]);
            raw_dealloc(new_data, new_cap);
            throw;
        }
        destroy_all();
        raw_dealloc(data_, cap_);
        data_ = new_data; cap_ = new_cap; sz_ += static_cast<size_type>(std::distance(first, last));
        LIST_ASSERT_INV();
    }

    /* ---- Range init / assign helpers ---- */
    template <typename InputIt>
    void init_from_range(InputIt first, InputIt last, std::input_iterator_tag) {
        for (; first != last; ++first)
            push_back(*first);
        LIST_ASSERT_INV();
    }

    template <typename FwdIt>
    void init_from_range(FwdIt first, FwdIt last, std::forward_iterator_tag) {
        auto n = static_cast<size_type>(std::distance(first, last));
        reserve(n);
        for (; first != last; ++first, ++sz_)
            alloc_traits::construct(alloc(), &data_[sz_], *first);
        LIST_ASSERT_INV();
    }

    template <typename InputIt>
    void assign_range(InputIt first, InputIt last, std::input_iterator_tag) {
        clear();
        for (; first != last; ++first)
            emplace_back(*first);
    }

    template <typename FwdIt>
    void assign_range(FwdIt first, FwdIt last, std::forward_iterator_tag) {
        auto n = static_cast<size_type>(std::distance(first, last));
        if (n > cap_) {
            List tmp(first, last, alloc());
            tmp.swap(*this);
        } else {
            FwdIt it = first;
            size_type i = 0;
            for (; i < sz_ && i < n; ++i, ++it) data_[i] = *it;
            for (; i < n; ++i, ++it) alloc_traits::construct(alloc(), &data_[i], *it);
            for (size_type j = n; j < sz_; ++j) destroy_at(&data_[j]);
            sz_ = n;
        }
        LIST_ASSERT_INV();
    }

    /* ---- Growth ---- */
    void grow() {
        size_type new_cap = next_cap();
        auto new_data = raw_alloc(new_cap);
        relocate(new_data, new_cap, typename std::is_nothrow_move_constructible<T>::type());
    }

    size_type next_cap() const noexcept {
        size_type base = (cap_ == 0) ? min_cap : cap_;
        return base + (base >> 1);
    }
};

/* ========== Non-member swap & erase ========== */
template <typename T, typename A>
void swap(List<T, A>& a, List<T, A>& b)
    noexcept(noexcept(a.swap(b))) { a.swap(b); }

template <typename T, typename A, typename U>
typename List<T, A>::size_type erase(List<T, A>& c, const U& val) {
    auto it = std::remove(c.begin(), c.end(), val);
    auto n = static_cast<typename List<T, A>::size_type>(c.end() - it);
    c.erase(it, c.end());
    return n;
}

template <typename T, typename A, typename Pred>
typename List<T, A>::size_type erase_if(List<T, A>& c, Pred pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto n = static_cast<typename List<T, A>::size_type>(c.end() - it);
    c.erase(it, c.end());
    return n;
}

#endif // LIST_COMPATIBLE_HPP
