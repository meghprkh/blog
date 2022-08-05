---
title: "BTrait - Rust Like Traits in C++"
date: 2022-07-28T10:15:46+01:00
tags: [ C++, Rust, series-btrait-intro ]
categories: C++
draft: true
enableTOC: true
---
I love Rust. Rust's traits are powerful yet easy-to-use abstractions. C++ interfaces are similar but not quite there. What if there was a way to implement Rust-like traits in C++? Enter **BTrait** - a design pattern / header-library.

*Have I BTrayed Rust by implementing BTrait? Read on to find out more... drumrolls...* (Okay no more puns now)
<!--more-->

*First post of a [three-post series]({{< ref "/tags/series-btrait-intro" >}})*

Skip directly to the [second post]({{< ref "/posts/BTrait-CPP-Combining-Traits" >}}) to see it in action and skip performance related stuff.

## Goals

- To allow:
    - Defining traits - small collections of methods, typedefs and constants
    - Implementing traits - even if we do not own the class.
        - For example, we want to implement `to_json` for `std::vector`
    - Compile-time Dispatching/overloading based on if Trait is implemented
    - Runtime Trait Objects - storing pointers or references of trait implementors
- To not add any runtime cost, with a basic `-O1` optimization level on gcc and clang
- While we use C++17, most of our code is compatible with C++03 (and some basic Boost)
- To allow defining the actual trait implementation is a separate compile unit
    - Defining a method with `__attribute_noinline__` is the same as putting it in a separate compile unit for our purposes

## Comparision with C++20 concepts

- We want to target old C++ versions
- C++ concepts are quite flexible features but lack some of the simplicity
- Cant create "trait objects" or "concept objects", i.e. how to store a vector of heteogeneous references?
- Combining concepts and "Super-concepts" as we see in part two is hard

## Primary Idea - `Trait<Type>`

If we implement our trait as a template taking the type:
- We can implement it without modifying the Type/Class
- We can implement it without adding a vtable to the class
- We can possibly combine multiple traits easily

## Initial pass - plain old inheritance

We will try to use plain old inheritance here:

```cpp
// declare trait
template <typename T = void>
struct Counter {
    struct implemented: std::false_type {};
};

// define trait
template <>
struct Counter<> {
    // Read as - allow this static method if T implements Counter
    // Will return a Counter<T>
    template<typename T>
    static std::enable_if_t<Counter<T>::implemented::value, Counter<T> >
    interpret(T& self) { return Counter<T>(self); }

    virtual int getCount() = 0;
};

// declare and define our struct
struct MyCounter {
    int count;
};

// implement trait for our struct
template <>
struct Counter<MyCounter>: Counter<> {
    Counter(MyCounter& self): self_(self) {}
    virtual __attribute_noinline__ int getCount() { return self_.count; };
    struct implemented: std::true_type {};
private:
    MyCounter& self_;
};

// struct without any trait stuff
struct NoTraitCounter {
    int count;
    int getCount() __attribute_noinline__ { return count; }
};

// Dispatched when trait is implemented
template <typename T>
std::enable_if_t<Counter<T>::implemented::value, void>
__attribute_noinline__
printCounter(T& counter) {
    std::cout << Counter<>::interpret(counter).getCount() << std::endl;
}

// Dispatched when trait not implemented
template <typename T>
std::enable_if_t<!Counter<T>::implemented::value, void>
__attribute_noinline__
printCounter(T& counter) {
    // Hope that method is implemented, else this will fail to compile
    std::cout << counter.getCount() << std::endl;
}

// check
int main() {
    MyCounter mycounter { 25 };
    NoTraitCounter ntcounter { 10 };
    printCounter(mycounter);
    printCounter(ntcounter);
}
```


If we put [this code on Godbolt](https://godbolt.org/z/87vMWcjrW), we see that it does work perfectly, but has a few regressions:

- `Counter<MyCounter>::getCount()` has an additional parameter (1 extra instruction)
    - Evident from the extra `mov     rax, QWORD PTR [rdi+8]`
    - This is because C++ passes `this` as the first parameter, and even if a struct has no data members, its address can be used to do stuff in methods.
- `printCounter<MyCounter>(MyCounter&)` has 3 more instructions
    - As compared to `printCounter<NoTraitCounter>(NoTraitCounter&)`
    - The following comes from having to construct `this` which has a `vtable` and a reference to `self_` / `mycounter`
      ```asm
      mov     QWORD PTR [rsp], OFFSET FLAT:vtable for Counter<MyCounter>+16
      mov     QWORD PTR [rsp+8], rdi
      ```
    - The following comes from having to load this newly constructed `this` as the first parameter to the call
      ```asm
      mov     rdi, rsp
      ```
    - Note that since `getCount` would be implemented in another CPP file / compile-unit, the compiler can do nothing smart about it and skip construction/loading of `this`

## Improvising - Using inline overloads, static methods and CRTP

- Static methods do not take a `this` pointer
- Inheritance does not really allow us to omit `this`

Lets add the following new class - this will always be inlined and live in a header file

```cpp
// Self would refer type being implemented for, "MyCounter" for example
template <typename Self>
struct CounterDefaultImpl: Counter<> {
    CounterDefaultImpl(Self& self): self_(self) {}

    virtual int getCount() override { return Counter<Self>::_impl_getCount(self_); };
private:
    Self& self_;
};
```

And change the implementation of `Counter<MyCounter>` to inherit it

```cpp
template <>
struct Counter<MyCounter>: CounterDefaultImpl<MyCounter> {
    Counter(MyCounter& self): CounterDefaultImpl<MyCounter>(self) {}
    struct implemented: std::true_type {};
private:
    static int _impl_getCount(MyCounter& self) __attribute_noinline__ { return self.count; };
    friend class CounterDefaultImpl<MyCounter>;
};
```

What we have effectively done is moved the construction and usage of `this` to a stub method and the core implementation to a static method that takes `self` as the parameter

Putting [this on godbolt](https://godbolt.org/z/dYcM8oEKq) we see that both implementations now have the **exact same code generated**! Hooray, we have implemented a basic version of traits!!

## Trait objects

We can still pass this like we used to do in inheritance to save some compilation overhead. We can also store this reference or raw pointer in an vector for example.

For example:

```cpp
void traitObjectExample(Counter<> &counter) {
    int count = counter.getCount();
    all_counters.push_back(counter);
}

auto mycounter_trait = Counter<>::interpret(mycounter); // type is Counter<MyCounter>
traitObjectExample(&mycounter_trait);
```

## Forgetting to implement a method / adding a new method in the trait

This will give us an easy to read error - even if we do not actually use the method - [Godbolt](https://godbolt.org/z/KoGjeYcac)

```cpp
<source>: In instantiation of 'int CounterDefaultImpl<Self>::getCount() [with Self = MyCounter]':
<source>:24:17:   required from here
<source>:24:75: error: '_impl_getCount' is not a member of 'Counter<MyCounter>'
   24 |     virtual int getCount() override { return Counter<Self>::_impl_getCount(self_); };
      |                                              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~
ASM generation compiler returned: 1
```

## Optimization and inline guarantees

- We compiled with `-O1` to ensure the compiler inlines our code
- But what if it didnt?
- `constexpr` those methods, helps optimization
- With required copy elision and optimization, it should still be fast. [Godbolt](https://godbolt.org/z/szTxqKn47) shows 3 extra instructions.
- `-Winline` can show warnings in gcc
- Compiler attributes that should be added - `__attribute__((__always_inline__))` or `__forceinline`
- These have not been written in above code for simplicity
- [Clang](https://godbolt.org/z/8fzjabGec) and [GCC](https://godbolt.org/z/8fzjabGec) have no issues keeping up
- [MSVC](https://en.wikipedia.org/wiki/Inline_function#Restrictions) has this problem especially because it cant inline a virtual function, generating 4 more instructions - [Godbolt](https://godbolt.org/z/KEb5r31nn)

We can get completely rid of this by making the static `impl` methods public. And directly requiring calls to call `Counter<MyCounter>::impl_getCount(mycounter)`

This can also be prettified using some more advanced metaprogramming I guess, maybe user-defined literals or macros for older C++? The extra cost would still be associated with dynamic passing.

## Summary

- We could implement a simple trait with no performance loss
- We could also implement a trait for an object we do not own
    - We can make the implementor a friend class to access private members if we own the class
- In the next post we will declare combinations of traits and discuss storing these as "trait objects"
- We will also discuss what methods are not "trait object safe" and how to handle them
- This includes discussion of traits with "other static methods" or constants
