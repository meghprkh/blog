---
title: "Concepts in C++03"
date: 2022-08-04T01:23:11+01:00
tags: [ C++, Rust, series-btrait-intro ]
categories: C++
enableTOC: true
---

C++20 Concepts are a new language feature that ease generic programming, but are primarily syntactic sugar.

We will try to implement them in C++03, with one caveat - we must explicitly specify that it implements the concept.

NOTE: We do not need to be able to modify the class for this.

## What are C++ Concepts

C++ Concepts allow us to do compile-time dispatch of methods. Kind of like Rust traits.

{{< godbolt options="-std=c++20" >}}
```cpp
#include <iostream>
#include <concepts>

//{
// Define the concept check
template <class T>
concept Counter = requires(T counter, int new_count) {
    { counter.get_count() } -> std::same_as<int>;
    { counter.set_count(new_count) } -> std::same_as<void>;
    { T::max_count() } -> std::same_as<int>;
};

// Our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};

// Example usage
template <typename T>
requires Counter<T>
void print_counter(T& counter) {
    // Can also use shorthand `template <Counter T>` instead of `requires` clause
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

int main() {
    MyCounter c { 25 };
    print_counter(c);
}
//}
```
{{< /godbolt >}}

Notice that we never needed to specify that `MyCounter` implements `Counter`. This can easily be fixed by requiring some constant to be defined in `MyCounter` or otherwise.

We call the above "implicit concepts". We will try to implement "explicit concepts" - where something must specify that the concept has been implemented.

## C++03 Concepts

We will use C++11 for a better STL, but we can easily swap it Boost

{{< godbolt options="-std=c++11" >}}
```cpp
#include <iostream>
#include <type_traits>

//{
// Define the concept
struct Counter {
    template <typename T>
    struct implemented_by: std::false_type {};

    // Define the check
    template <typename T>
    struct check: std::true_type {
        static_assert(static_cast< int (T::*)() >(&T::get_count));
        static_assert(static_cast< void (T::*)(int) >(&T::set_count));
        static_assert(static_cast< int (*)() >(&T::max_count));
    };
};

// Define our struct normally, without any modifications
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
// Declare and check that we have implemented the trait
template<>
struct Counter::implemented_by<MyCounter>: Counter::check<MyCounter> {};

// Example usage using classic enable_if SFINAE. Verbose yet conventional
template <typename T>
typename std::enable_if<
    Counter::implemented_by<T>::value,
void >::type
print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

int main() {
    MyCounter counter { 3 };
    print_counter(counter);
}
//}
```
{{< /godbolt >}}

To do this in C++03, lets sprinkle some macros. We will also *beautify* the C++11 code using macros.

{{< godbolt options="-std=c++03" >}}
```cpp
#include <iostream>

#if __cplusplus != 199711L
#include <type_traits>
#else
// Monkey patch std for the sake of the example
// Can use your favourite utility library instead
namespace std {
    struct true_type { static const bool value = true; };
    struct false_type { static const bool value = true; };
    template <bool Cond, typename Type = void>
    struct enable_if { typedef Type type; };
    template <typename Type>
    struct enable_if<false, Type> { };
}
#endif

#define _CONCEPT_TOKENPASTE(x, y) x ## y
#define _CONCEPT_TOKENPASTE2(x, y) _CONCEPT_TOKENPASTE(x, y)

#if __cplusplus != 199711L

#define CONCEPT_CHECK_START \
    template <typename Self> \
    struct check: std::true_type { \

#define CONCEPT_CHECK_END };

#define CONCEPT_ASSERT static_assert

#define IMPL_CONCEPT(CONCEPT, CLS) \
    template<> \
    struct CONCEPT::implemented_by<CLS>: CONCEPT::check<CLS> {}; \

#else

// In C++03 we dont have static_assert and we cannot use "&T::method"
// inside say a BOOST_STATIC_ASSERT.
// We just define all of the checks inside the constructor of `check`.
// Then in `IMPL_CONCEPT` we instantiate a static object of the same.

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

#endif

//{
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
//}

// Define our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
//{
// ... skipping definition of struct MyCounter ...
// Declare and check that we have implemented the trait
IMPL_CONCEPT(Counter, MyCounter);
//}

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
```
{{< /godbolt >}}

We can now use this for defining `print_count` using the same `enable_if` way we used this previously. Also most of our macros are simple ones that dont require any parenthesis-escaping except `IMPL_CONCEPT`.

## Rant on C++20 Concepts

C++20 Concepts thus allow for powerful implicit matching. But, let us take the following example:

> Lets say we are building some kind of social media stats app and we have `youtube_api::VideoViewCounter` and `instagram_api::LikeCounter`. Both of them have the `get_count` method.
>
> We want to define a `print_count(counter)` method which takes either of these two classes and does `std::cout << counter.get_count()`.

We do not have control over either APIs, but would like a common abstraction. We have two choices:

1. Declare an "implicit concept" called `Counter` which requires a `get_count` method. Define templated `print_count` for concept
2. Declare an "explicit concept" with the same. Specify that the above two classes implement this concept without modying the classes. Define templated `print_count` for concept.
3. Use an unchecked templated `print_count`

Now consider the following modification to the codebase:

> We add class `my_shared_ptr` which has `get_count` method which returns the reference count of the pointer.
>
> Lets say another engineer started refactoring to store the objects in `shared_ptr` but `print_counter` has not been modified for an explicit overload for `shared_ptr`
>
> What happens when we call `print_count(counter_ptr)` with `counter_ptr = my_shared_ptr<youtube_api::VideoViewCounter>()`?

In the case of "implicit concepts", we would see the reference count being printed, without any compile or run-time error.

In the case of "explicit concepts", we would get a compile time error since no method matches this.

In the case of an unchecked template too, we would see the reference count being printed..

Thus, *implicit concepts are almost as bad as not having any check at all*. Except maybe they can produce neater compiler errors (ignoring the case of overloading  based on concepts).

And if explicit concepts are better and already implementable in C++03, why provide an abstraction where most developers will write error-prone code instead of providing syntax sugar for explicit concepts?

## Summary

We saw what C++ concepts were and how to write "explicit concepts" in C++03. This was followed by a small rant on C++20 implicit concepts.

In the next post I will describe how this compares to Rust traits and how to implement "trait objects" or "concept maps" using the same code.
