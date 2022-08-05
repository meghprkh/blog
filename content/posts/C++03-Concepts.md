---
title: "C++20 Concepts in C++03"
date: 2022-08-04T01:23:11+01:00
tags: [ C++, Rust, series-btrait-intro ]
crossposts:
    hackernews: https://news.ycombinator.com/item?id=32356522
    reddit: https://www.reddit.com/r/cpp/comments/wgxjzy/c20_concepts_in_c03/
categories: C++
enableTOC: true
---

C++20 Concepts are a new language feature that ease generic programming, but are primarily syntactic sugar.

We will try to implement them in C++03, with one caveat - we must *explicitly specify that a class implements an concept*.

**NOTE**: We will use template specialization and do not need to be able to modify the class or our concept for this.

**NOTE**: If it seems like the caveat ignores the entire point of concepts, call these "pseudo-minimal-rust-traits" and read on. By the end of the article, as the Zen of Python mentions, I promise you will agree that explicit is better than implicit :P

## What are C++ Concepts

C++ Concepts allow us to do compile-time dispatch of methods.

This compile-time dispatch is thus kind of like Rust traits. (Rust traits provide other features too.)

{{< godbolt options="-std=c++20" >}}
```cpp
#include <iostream>
#include <concepts>

//{
// Define the concept check
template <typename Self>
concept Counter = requires(Self counter, int new_count) {
    { counter.get_count() } -> std::same_as<int>;
    { counter.set_count(new_count) } -> std::same_as<void>;
    { Self::max_count() } -> std::same_as<int>;
};

// Our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
static_assert(Counter<MyCounter>); // optionally check implementation
                                   // if we forgot any methods, etc...

// Example usage
template <typename T>
requires Counter<T>
void print_counter(T& counter) {
    // Can also use shorthand `template <Counter T>` instead of `requires` clause
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

// compile-time dispatch another method
void print_counter(int counter) {
    std::cout << "Integer counter with count " << counter << std::endl;
}

// Shorthand syntax
void print_counter_shorthand(Counter auto counter) { print_counter(counter); }

int main() {
    MyCounter c { 25 };
    print_counter(c);
    print_counter(10); // Prints Integer counter
    print_counter_shorthand(c);
}
//}
```
{{< /godbolt >}}

Notice that we never needed to specify that `MyCounter` implements `Counter`. This can easily be fixed by requiring some constant to be defined in `MyCounter` or otherwise.

We call the above **"implicit concepts"**. We will try to implement **"explicit concepts"** - where something must specify that the concept has been implemented for a class.

## C++03 Concepts

We will use C++11 initially. Then will also modify this using some macros for C++03.

We use a templated struct and observe that `static_asserts` inside it are executed when the template is specialized.

{{< godbolt options="-std=c++11" >}}
```cpp
#include <iostream>
#include <type_traits>

//{
// Define the concept
struct Counter {
    // This template is specialized to true_type by any class that
    // implements this. Specialization requires ownership of neither
    // Counter concept or Self (Self being the class in question)
    template <typename Self>
    struct is_implemented_by: std::false_type {};

    // Define the check
    template <typename Self>
    struct check: std::true_type {
        static_assert(static_cast< int (Self::*)() >(&Self::get_count));
        static_assert(static_cast< void (Self::*)(int) >(&Self::set_count));
        static_assert(static_cast< int (*)() >(&Self::max_count));
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
struct Counter::is_implemented_by<MyCounter>: Counter::check<MyCounter> {};

// Example usage using classic enable_if SFINAE. Verbose yet conventional
template <typename T>
typename std::enable_if<
    Counter::is_implemented_by<T>::value,
void >::type
print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

// compile-time dispatch another method
void print_counter(int counter) {
    std::cout << "Integer counter with count " << counter << std::endl;
}

int main() {
    MyCounter counter { 25 };
    print_counter(counter);
    print_counter(10); // Prints Integer counter
}
//}
```
{{< /godbolt >}}

To do this in C++03, and make it work with C++11 too, lets sprinkle some macros.

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

#define CONCEPT_CHECK_BEGIN \
    template <typename Self> \
    struct check: std::true_type { \

#define CONCEPT_CHECK_END };

#define CONCEPT_ASSERT static_assert

#define IMPL_CONCEPT(CONCEPT, CLS) \
    template<> \
    struct CONCEPT::is_implemented_by<CLS>: CONCEPT::check<CLS> {}; \

#else

// In C++03 we dont have static_assert and we cannot use "&T::method"
// inside say a BOOST_STATIC_ASSERT.
// We just define all of the checks inside the constructor of `check`.
// Then in `IMPL_CONCEPT` we instantiate a static object of the same.

#define CONCEPT_CHECK_BEGIN \
    template <typename Self> \
    struct check: std::true_type { \
        check() { \

#define CONCEPT_CHECK_END } };

void _concept_assert(bool) {}
#define CONCEPT_ASSERT _concept_assert

#define IMPL_CONCEPT(CONCEPT, CLS) \
    template<> \
    struct CONCEPT::is_implemented_by<CLS>: CONCEPT::check<CLS> {}; \
    static CONCEPT::check<CLS> _CONCEPT_TOKENPASTE2(_concept_check_, __LINE__); \

#endif

//{
// Define the concept
struct Counter {
    template <typename T>
    struct is_implemented_by: std::false_type {};

    // Define the check
    CONCEPT_CHECK_BEGIN
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
    Counter::is_implemented_by<T>::value,
void >::type
print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

int main() {
    MyCounter counter;
    counter.count = 25;
    Counter::check<MyCounter> x;
    print_counter(counter);
}
```
{{< /godbolt >}}

We can now use this for defining `print_count` using the same `enable_if` way we used previously. Also most of our macros are simple ones that dont require any parenthesis-escaping except `IMPL_CONCEPT`. Note these macros are completely optional in C++11.

<details>

<summary>C++03 details and the macros</summary>


`BOOST_STATIC_ASSERT` can not take reference to a function (and `static_assert` is C++11)
```
<source>:67:67: error: '&' cannot appear in a constant-expression
   67 |         BOOST_STATIC_ASSERT(static_cast< int (Self::*)() >(&Self::get_count));
```

We get around this by defining the `CONCEPT_ASSERT` macro expands an empty function in C++03, and the `CONCEPT_CHECK_BEGIN` defines the constructor of a `check<Self>` struct. This object is then internal-linkage-constructed by `IMPL_CONCEPT`. This ensures that the compiler tries to specialize the constructor with `Self` and detects that the `static_cast`s failed.

Note the `CONCEPT_ASSERT` macro should not be used for "normal"/non-method check asserts as it simply does nothing. Use say `BOOST_STATIC_ASSERT` otherwise.

See example preprocessor output [here](https://godbolt.org/z/vn9eKr6G1)

We can check the compile time error because `get_count` is commented out

- [Gcc 4.9](https://godbolt.org/z/TY5ce3G9b) ([GCC 4.1.2](https://godbolt.org/z/oca3efas4))
- [Clang 3.4](https://godbolt.org/z/fnndG38cc)
- [MSVC 19.14 (2017 - new but oldest on godbolt)](https://godbolt.org/z/5xvnErG1v)
- [ICC 13.0.1 (2012)](https://godbolt.org/z/M1oT6na4v)

</details>

## Aside: Explicit concepts in C++20

Explicit concepts can be implemented in pretty much the same way in C++20, using a templated `is_counter` conditional struct

{{< godbolt options="-std=c++20" >}}
```cpp
#include <iostream>
#include <concepts>
#include <type_traits>

//{
// Explicit check
template <typename Self>
struct is_counter: std::false_type {};

// Define the concept check
template <typename Self>
concept Counter =
    is_counter<Self>::value &&               // ** NEW **
    requires(Self counter, int new_count) {  // ** SAME STUFF **
        { counter.get_count() } -> std::same_as<int>;
        { counter.set_count(new_count) } -> std::same_as<void>;
        { Self::max_count() } -> std::same_as<int>;
    };

//}
// Our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
//{
// ... skipping definition of struct MyCounter ...
// Declare and check that we have implemented the trait
template <> struct is_counter<MyCounter> : std::true_type {};
static_assert(Counter<MyCounter>); // optionally check implementation
                                   // if we forgot any methods, etc...
//}

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

## Rant on C++20 Concepts

C++20 Concepts thus allow for powerful implicit matching. But, let us take the following example:

> Lets say we are building some kind of social media stats app and we have `youtube_api::VideoViewCounter` and `instagram_api::LikeCounter`. Both of them have the `get_count` method.
>
> We want to define a `print_count(counter)` method which takes either of these two classes and does `std::cout << counter.get_count()`.

We do not have control over either APIs, but would like a common abstraction. We can:

1. Declare an "implicit concept" called `Counter` which requires a `get_count` method. Define templated `print_count` for concept
2. Declare an "explicit concept" with the same. Specify that the above two classes implement this concept without modying the classes. Define templated `print_count` for concept.
3. Use an unchecked templated `print_count`

Now consider the following modification to the codebase:

> We add class `my_shared_ptr` which has `get_count` method which returns the reference count of the pointer.
>
> Lets say another engineer started refactoring to store the objects in `shared_ptr` but `print_counter` has not been modified for an explicit overload for `shared_ptr`
>
> What happens when we call `print_count(counter_ptr)` with `counter_ptr = my_shared_ptr<youtube_api::VideoViewCounter>()`?

Note that:
1. In the case of "implicit concepts", we would see the reference count being printed, without any compile or run-time error.
2. In the case of "explicit concepts", we would get a compile time error since no method matches this.
3. In the case of an unchecked template too, we would see the reference count being printed too.

Thus, *implicit concepts are almost as bad as not having any check at all*. Except maybe they can produce neater compiler errors (ignoring the case of overloading  based on concepts).

Even if you had a 1000 different classes, writing 1000 more lines saying that a concept is implemented by them is better than implicit behaviour in my opinion. In most cases you will either have 1000 template specializations or some script generated code, and in both cases you only need to add one line.

> What regex is to parsing, implicit concepts are to C++.

And if explicit concepts are better and already implementable in C++03, why provide an abstraction where most developers will write error-prone code instead of providing syntax sugar for explicit concepts?

## Extra syntactic sugar stuff

- Using C++20 Concepts with C++03 concepts
  ```cpp
  template <typename Self>
  concept Counter_ = Counter::is_implemented_by<Self>::value;

  void print_counter(Counter_ auto counter) { ... }
  ```
- Requiring multiple C++03 Concepts to be satisfied
  ```cpp
  template <typename Self, typename... Trait>
  struct require_concepts: std::conjunction< Trait::is_implemented_by<Self>... > {};

  USE AS std::enable_if<require_concepts<Cls, Concept1, Concept2>> ...
  ```
- Composing concepts - Using other concept checks in a check (slightly leaky abstraction for C++03)
  ```cpp
  CONCEPT_CHECK_BEGIN
    // Require other_concept to be implemented
    BOOST_STATIC_ASSERT(other_concept::is_implemented_by<Self>::value)
    // Require either_concept1 or or_concept to be implemented
    BOOST_STATIC_ASSERT(either_concept1::is_implemented_by<Self>::value || or_concept2::is_implemented_by<Self>::value)
  CONCEPT_CHECK_END
  ```

## Summary

We saw what C++ concepts were and how to write "explicit concepts" in C++03. We also noted that C++20 implicit concepts are error-prone.

In the next post I will describe how this compares to Rust traits and how to implement "trait objects" or "concept maps" using the same code.

**NOTE**: For any of the "predefined concepts" like say `copy_constructible`, `type_traits` or similar Boost/utility library can be used.
