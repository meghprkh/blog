#include <iostream>
#include <vector>
#include <type_traits>
#include <memory>
#include <cstring>

#define BTRAIT_METHOD_DECL const override final [[gnu::always_inline]]
#define BTRAIT_METHOD_IMPL

// template<typename Trait>
// using btrait_dyn<Trait> = Trait::__btrait_dyn;

template <typename T, typename... Traits>
struct btrait_require: std::conjunction<typename Traits::template is_implemented_by<T>... > {};

template <typename T, typename... Traits>
constexpr bool btrait_require_v = btrait_require<T, Traits...>::value;

template <typename Self = void>
struct Counter {
private:
    struct __btrait_check: std::false_type {};
    friend class Counter<>;
};

template <typename Trait>
struct btrait_dyn {
    typename Trait::__btrait_abstract* operator->() const {
        return ptr_;
    }
    template <typename T>
    btrait_dyn(T& orig) {
        try {
            ptr_ = (typename Trait::__btrait_abstract*) (new char[sizeof(typename Trait::implementor_for<T>::type)]);
            typename Trait::implementor_for<T>::type tmp = Trait::interpret(orig);
            std::memcpy((void*) ptr_, (const void*) &tmp, sizeof(typename Trait::implementor_for<T>::type));
        } catch (std::bad_alloc& all) {
            delete ptr_;
            throw all;
        }
    }
    constexpr btrait_dyn() noexcept {};
    ~btrait_dyn() { delete ptr_; }
private:
    typename Trait::__btrait_abstract* ptr_;
    friend Trait;
};

template <>
struct Counter<> {
    struct __btrait_abstract;

    template <typename Self>
    struct __btrait_check;

    template <typename Self>
    struct __btrait_impl;

    template<typename T>
    struct is_implemented_by: Counter<T>::__btrait_check {};

    template<typename T>
    struct implementor_for {
        typedef Counter<T> type;
    };

    template <typename T>
    static constexpr Counter<T> interpret(T& t) {
        return Counter<T>(t);
    }

    struct __btrait_abstract {
        virtual int get_count() const = 0;
        virtual void set_count(int new_count) const = 0;
        virtual void increment() const = 0;
    };

    template <typename Self_>
    struct __btrait_impl: __btrait_abstract {
    public:
        // Public methods
        virtual int get_count() BTRAIT_METHOD_DECL
        { return Counter<Self>::_impl_get_count(self_); }
        virtual void set_count(int new_count) BTRAIT_METHOD_DECL
        { return Counter<Self>::_impl_set_count(self_, new_count); }
        virtual void increment() BTRAIT_METHOD_DECL { return Counter<Self>::_impl_increment(self_); };

        // Static methods
        static int max_count() = delete;

        typedef Self_ Self;
    private:
        static void _impl_increment(Self& self) BTRAIT_METHOD_IMPL
        { auto counter = Counter<>::interpret(self); counter.set_count(counter.get_count() + 1); };

    protected:
        Self& self_;
        __btrait_impl(Self& self): self_(self) {}
        struct __btrait_check: Counter<>::__btrait_check<Self> {};
    };

    template <typename Self>
    struct __btrait_check: std::true_type {
        // Check whether static methods implemented
        static_assert(std::is_invocable_r_v<int, decltype(Counter<Self>::max_count)>);
    };
};

// declare and define our struct
struct MyCounter {
    int count;
};

// implement trait for our struct
template <>
struct Counter<MyCounter>: Counter<>::__btrait_impl<MyCounter> {
public:
    // PUBLIC static methods
    static int max_count() { return 100; }
protected:
    // Protected _impl methods
    static int _impl_get_count(Self& self) BTRAIT_METHOD_IMPL
    { return self.count; };
    static void _impl_set_count(Self& self, int new_count) BTRAIT_METHOD_IMPL
    { self.count = new_count; };
private:
    // Private constructor
    Counter(MyCounter& self): Counter<>::__btrait_impl<Self>(self) {}
    friend class Counter<>;
};

template<typename T>
std::enable_if_t< btrait_require_v< T, Counter<> >, void >
print_counter(T& counter) {
    std::cout << "Counter with count " << Counter<>::interpret(counter).get_count() << std::endl;
}

int main() {
    MyCounter counter { 25 };
    print_counter(counter);
    auto counter_dyn = btrait_dyn<Counter<>>(counter);
    std::cout << typeid(counter_dyn).name() << " "
              << counter_dyn->get_count() << std::endl;
}
