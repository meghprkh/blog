#include <iostream>

#include <boost/config.hpp>

#if BOOST_CXX_VERSION == 199711L
#include <boost/type_traits.hpp>
namespace std {
    struct true_type: boost::true_type {};
    struct false_type: boost::false_type {};
    template <bool cond, typename type>
    struct enable_if: boost::enable_if_<cond, type> {};
}
#else
#include <type_traits>
#endif

#define _CONCEPT_TOKENPASTE(x, y) x ## y
#define _CONCEPT_TOKENPASTE2(x, y) _CONCEPT_TOKENPASTE(x, y)

#if BOOST_CXX_VERSION == 199711L
#define CONCEPT_CHECK_START \
    template <typename Self> \
    struct check: std::true_type { \
        check() { \

#define CONCEPT_CHECK_END } };

#define CONCEPT_ASSERT

#define IMPL_CONCEPT(CONCEPT, CLS) \
    template<> \
    struct CONCEPT::implemented_by<CLS>: CONCEPT::check<CLS> {}; \
    static CONCEPT::check<CLS> _CONCEPT_TOKENPASTE2(_concept_check_, __LINE__); \

#else
#define CONCEPT_CHECK_START \
    template <typename Self> \
    struct check: std::true_type { \

#define CONCEPT_CHECK_END };

#define CONCEPT_ASSERT static_assert

#define IMPL_CONCEPT(CONCEPT, CLS) \
    template<> \
    struct CONCEPT::implemented_by<CLS>: CONCEPT::check<CLS> {}; \

#endif

// Define the concept
struct Counter {
    template <typename T>
    struct implemented_by: std::false_type {};

    // Define the check
    CONCEPT_CHECK_START
        CONCEPT_ASSERT(static_cast< int (Self::*)() >(&Self::get_count));
        CONCEPT_ASSERT(static_cast< void (Self::*)(int) >(&Self::set_count));
        CONCEPT_ASSERT(static_cast< int (*)() >(&Self::max_count));
    CONCEPT_CHECK_END
};

// Define our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
// Declare and check that we have implemented the trait
IMPL_CONCEPT(Counter, MyCounter);

// Example usage using classic enable_if SFINAE
template <typename T>
typename std::enable_if<
    Counter::implemented_by<T>::value,
void >::type
print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

int main() {
    MyCounter counter;
    counter.count = 3;
    Counter::check<MyCounter> x;
    print_counter(counter);
}
