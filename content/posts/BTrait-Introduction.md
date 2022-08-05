---
title: "BTrait Introduction"
date: 2022-07-25T10:15:46+01:00
tags: [ C++, Rust, series-btrait-intro ]
categories: C++
draft: true
enableTOC: true
---

**THIS IS A DRAFT POST**

I love Rust. Rust's traits are powerful yet easy-to-use abstractions. C++ abstract classes and concepts are quite similar. C++ also has type traits and other traits like iterator traits. In this post I will introduce a simple Rust trait and implement this in C++. I will refer to this design pattern / header-library as BTrait.

*Have I BTrayed Rust by implementing BTrait? Read on to find out more... drumrolls...* (Okay no more puns now)
<!--more-->

## What are traits

Traits can be understood as a collection of methods, constants and associated types implemented for a class.

Lets define a trait called `Counter` in Rust

```rust
trait Counter {
    fn get_count(&self) -> i64; // type will have a get_count method
    fn set_count(&mut self, new_count: i64); // type will have a set_count
    fn increment(&mut self) { // increment method which has a default implementation
        self.set_count(self.get_count() + 1);
    }

    fn max_count() -> i64; // maxCount free function returns the maximum possible count
}
```

Now lets implement that trait for `MyCounter` and use that trait

{{< godbolt language="rust" >}}
```rust
trait Counter {
    fn get_count(&self) -> i64; // type will have a get_count method
    fn set_count(&mut self, new_count: i64); // type will have a set_count
    fn increment(&mut self) { // increment method which has a default implementation
        self.set_count(self.get_count() + 1);
    }

    fn max_count() -> i64; // maxCount free function returns the maximum possible count
}

//{
struct MyCounter {
    count: i64
}

impl Counter for MyCounter {
    fn get_count(&self) -> i64 { return self.count; }
    fn set_count(&mut self, new_count: i64) { self.count = new_count; }

    fn max_count() -> i64 { return 100; }
}

fn main() {
    let mut counter = MyCounter { count: 25 };
    println!("{}", counter.get_count()); // 25
    counter.set_count(10);
    // counter.get_count is syntactic sugar for
    println!("{}", Counter::get_count(&counter)); // 10
    counter.increment(); // will use default implementation of increment
    println!("{}", counter.get_count()); // 11
    println!("{}", MyCounter::max_count()); // 100
}
//}
```
{{< /godbolt >}}

Now for example the trait could have had an *associated constant* called `STEP` and `increment` could have used that

{{< godbolt language="rust" >}}
```rust
//{
trait Counter {
//}
    fn get_count(&self) -> i64; // type will have a get_count method
    fn set_count(&mut self, new_count: i64); // type will have a set_count

//{
    const STEP: i64; // For a default value, can write: const STEP: i64 = 1
    fn increment(&mut self) { // increment method should increment by STEP
        self.set_count(self.get_count() + Self::STEP);
    }
//}

    fn max_count() -> i64; // maxCount free function returns the maximum possible count
//{
}
//}

struct MyCounter {
    count: i64
}

//{
impl Counter for MyCounter {
    const STEP: i64 = 2; // increment by 2
//}

    fn get_count(&self) -> i64 { return self.count; }
    fn set_count(&mut self, new_count: i64) { self.count = new_count; }

    fn max_count() -> i64 { return 100; }
//{
}

fn main() {
    let mut counter = MyCounter { count: 25 };
    counter.increment(); // 27 ( = count 25 + STEP 2 )
    println!("{}", counter.get_count()); // 27
}
//}
```
{{< /godbolt >}}

It could also have had an *associated type* which represented the type of the count, for example `u32` or `i64`

```rust
trait Counter {
    type CountType;
    fn get_count(&self) -> CountType { return self.count; }
}
struct MyCounter {
    count: i64;
}
impl Counter for MyCounter {
    type CountType = i64;
}
```

## Traits use-cases and super-powers


### Code reuse

Traits thus allow us to specify default methods/values encouraging code-reuse and not having to write everything for every type.

### Method dispatching

Method dispatching can also be controlled via traits. C++ developers will immediately recognize this as SFINAE.

```rust
fn print_counter(count: impl Counter) {
    println!("{}", count.getCount());
}
fn print_counter(count: i64) {
    println!("i64 count {}", count);
}
print_counter(mycounter); // works
print_counter(32); // calls i64 count
print_counter(some_other_type); // compile error
```

### Trait objects

You can also store "trait objects" or pointers to them. This is extremely useful in say a GUI library that stores widgets of different types unkown at compile time. Also, in the previous example a copy of the `print_counter` function would be generated for every type that implements `Counter`. This gives great performance but increases compilation times.

{{< godbolt language="rust" >}}
```rust
trait Counter {
    fn get_count(&self) -> i64; // type will have a get_count method
}

struct MyCounter {
    count: i64
}
impl Counter for MyCounter {
    fn get_count(&self) -> i64 { return self.count; }
}

struct AnotherCounter {
    cute_count: i64
}
impl Counter for AnotherCounter {
    fn get_count(&self) -> i64 { return self.cute_count; }
}

//{
// Box<dyn Counter> can be any type and is resolved at runtime
// The function is not "copy-pasted" for all types that implement Counter
fn print_counter(count: Box<dyn Counter>) {
    println!("{}", count.get_count());
}

fn main() {
    let counter1 = MyCounter { count: 25 };
    let counter2 = AnotherCounter { cute_count: 32 };
    // "Boxes" of different types that implement Counter can also be stored
    // in the same container. Normally you cant store different types in a vector.
    // (A Box is just a smart pointer - unique_pointer in C++ terminology)
    let mut all_counters: Vec<Box<dyn Counter>> = vec![];
    all_counters.push(Box::from(counter1));
    all_counters.push(Box::from(counter2));
    for counter in all_counters {
        print_counter(Box::from(counter));
    }
}
//}
```
{{< /godbolt >}}

### Composability

Both dispatching and trait objects can specify that multiple traits must be satisfied, for example `impl Counter + Serializable` or `dyn Couter + Serializable` can be used for a method like `snapshot_counter`

Super traits can also be defined, for example `SnapshottableCounter` which requires that type implement both `Counter` and `Serializable` and provides a `snapshot method.

### Orphan rule

Rust allows defining a trait if we own the class or the trait. Note we can implement a trait *even if we do NOT own the class*. This comes in very handy.

For example, we want to write a recursive `toJson` method in a `JsonSerializable` trait. We realize that deep within our code we are storing a `vector<int>` directly and thus need to implement `JsonSerializable` for `vector<int>` which is a type we do not own. Such issues keep coming up when we want to define a "custom trait" but the class authors never anticipated it.

### Zero-cost abstraction

Traits do not add anything to the struct. `dyn` requires a vtable, but it is only added when a `dyn` is created.

## Comparisons

### C++ inheritance and abstract classes

C++ allows inheritance, providing us something like

```cpp
struct Counter {
    virtual int get_count() = 0;
    virtual void set_count(int new_count) = 0;
    virtual void increment() { set_count(get_count() + 1); }
    static int max_count() = delete;
};

struct MyCounter: Counter {
    int count;
    virtual int get_count() override { return count; }
    virtual void set_count(int new_count) override { count = new_count; }
    static int max_count() { return 100; }
};
```

`= 0` and `= delete` ensure that we must override the method in any inherited class.

The problem with this is inheritance adds a `vtable` to the struct. `sizeof(MyCounter) != sizeof(int)` anymore, even though `MyCounter` only stores an integer. Even more problems happen when we do multiple inheritance.

A vtable is essentially a table containing a method name to a pointer. For example in the vtable of `mycounter`, `set_count` will point to `MyCounter::set_count`, but `increment` will point to `Counter::increment`.

{{< godbolt >}}
```cpp
#include <iostream>
#include <type_traits>

struct Counter {
    virtual int get_count() = 0;
    virtual void set_count(int new_count) = 0;
    virtual void increment() { set_count(get_count() + 1); }
    static int max_count() = delete;
};

struct MyCounter: Counter {
    int count;
    virtual int get_count() override { return count; }
    virtual void set_count(int new_count) override { count = new_count; }
    static int max_count() { return 100; }
};

//{
// compile-time
template <typename T>
std::enable_if_t< std::is_base_of_v<Counter, T> , void>
print_counter(T counter) {
    std::cout << counter.get_count() << std::endl
}

// runtime
void print_counter(Counter& counter) {
    // get_count will be looked up in vtable
    std::cout << counter.get_count() << std::endl
}

// can also store Vector<Counter*>
int main() {
    MyCounter mycounter { 25 };
    print_counter(mycounter);
    std::cout << MyCounter::max_count() << std::endl;
}
//}
```
{{< /godbolt >}}

Thus inheritance provides us with
- [x] Code reuse
- [x] Method dispatch
- [x] Trait objects
- [x] Composabiltiy - multiple inheritance exists
- [ ] Orphan rule - can only define parent classes for classes we own
- [ ] Zero cost abstraction - adds vtables

### C++20 Concepts

Concepts are primarily syntactic sugar that allow "better SFINAE".

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
    { counter.increment() } -> std::same_as<void>;
    { T::max_count() } -> std::same_as<int>;
};

// Sample default method
template <typename T>
requires Counter<T>
void counter_default_increment(T& counter) {
    counter.set_count(counter.get_count() + 1);
}

// Our struct
struct MyCounter {
    int count;

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    void increment() { return counter_default_increment(*this); }
    static int max_count() { return 100; }
};

// Example usage
template <typename T>
requires Counter<T>
void print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}

int main() {
    MyCounter c { 25 };
    print_counter(c);
    c.increment();
    print_counter(c);
    std::cout << "Max count of MyCounter " << MyCounter::max_count() << std::endl;
}
//}
```
{{< /godbolt >}}

There is no concept of "trait objects" or "concept objects", that is there is no way to store heterogeneous references to something that "implements Counter".



Thus concepts naively provides us with
- [x] Code reuse
- [x] Method dispatch
- [ ] Trait objects
- [x] Composabiltiy - can "implement multiple concepts"
- [ ] Orphan rule - can only implement methods for classes we own
- [x] Zero cost abstraction

<details>

<summary>NOTE: The above solution can be amended to fix for the orphan rule.</summary>

{{< godbolt options="-std=c++20" >}}
```cpp
#include <iostream>
#include <concepts>

//{
// Define the "trait" with "default implementations"
struct Counter {
    template <typename T> static int get_count(T& counter) = delete;
    template <typename T> static void set_count(T& counter, int new_count) = delete;

    template <typename T>
    static void increment(T& counter) { // has default implementation
        set_count(counter, get_count(counter) + 1);
    }

    template <typename T>
    static int max_count() = delete;
};

// Define the concept check
template <class T>
concept CounterType = requires(T counter, int new_count) {
    { Counter::get_count(counter) } -> std::same_as<int>;
    { Counter::set_count(counter, new_count) } -> std::same_as<void>;
    { Counter::increment(counter) } -> std::same_as<void>;
    { Counter::max_count<T>() } -> std::same_as<int>;
};

// Our struct
struct MyCounter {
    int count;
};

// The "implementation of the trait" - does not modify "MyCounter" or "Counter"
template<> int Counter::get_count(MyCounter& counter) { return counter.count; }
template<> void Counter::set_count(MyCounter& counter, int new_count) { counter.count = new_count; }
template<> int Counter::max_count<MyCounter>() { return 100; }
static_assert(CounterType<MyCounter>); // optional check that we have fully implemented it

// Example usage
template <typename T>
requires CounterType<T>
void print_counter(T& counter) {
    std::cout << "Counter with count " << Counter::get_count(counter) << std::endl;
}

int main() {
    MyCounter c { 25 };
    print_counter(c);
    Counter::increment(c);
    print_counter(c);
    std::cout << "Max count of MyCounter " << Counter::max_count<MyCounter>() << std::endl;
}
//}
```
{{</ godbolt >}}

</details>

## Implementing "Explicit Concepts" in C++11

Lets say a class must explicitly specify that it implements a concept

```cpp
struct Counter {
    template <typename T>
    struct implemented_by: std::false_type {};

    template <typename Self>
    struct default_impl {
        void increment() {
            Self& self = static_cast<Self&>(*this);
            self.set_count(self.get_count() + 1);
        }
    };

    template <typename T>
    struct check: std::true_type {
        // Different valid syntaxes for checking
        T* counter;
        static_assert(std::is_same_v<int, decltype(counter->get_count())>);
        static_assert(std::is_invocable_r_v<void, decltype(&T::set_count), T*, int>);
        static_assert(static_cast< void (T::*)() >(&T::increment));
        // Static method checking
        static_assert(std::is_invocable_r_v<void, decltype(&T::max_count)>);
    };
};

struct MyCounter: Counter::default_impl<MyCounter> {
    int get_count() { return count; }
    ...
};
// Explicitly specify and check whether trait is implemented
template<>
struct Counter::implemented_by<MyCounter>: Counter::check<MyCounter> {};
```

Now we can use this like

{{< godbolt >}}
```cpp
#include <iostream>
#include <type_traits>

struct Counter {
    template <typename T>
    struct implemented_by: std::false_type {};
    template <typename T>
    static constexpr bool implemented_by_v = implemented_by<T>::value;

    template <typename Self>
    struct default_impl {
        void increment() {
            Self& self = static_cast<Self&>(*this);
            self.set_count(self.get_count() + 1);
        }
    };

    template <typename T>
    struct check: std::true_type {
        // Different valid syntaxes for checking
        T* counter;
        static_assert(std::is_same_v<int, decltype(counter->get_count())>);
        // Note this allows conversion of return type, so probably makes less sense here
        static_assert(std::is_invocable_r_v<void, decltype(&T::set_count), T*, int>);
        static_assert(static_cast< void (T::*)() >(&T::increment));
        // Static method checking
        static_assert(static_cast< int (*)() >(&T::max_count));
    };
};

struct MyCounter: Counter::default_impl<MyCounter> {
    int count;
    MyCounter(int c): count(c) {};

    int get_count() { return count; }
    void set_count(int new_count) { count = new_count; }
    static int max_count() { return 100; }
};
template<>
struct Counter::implemented_by<MyCounter>: Counter::check<MyCounter> {};

//{
// Example usage
template <typename T>
std::enable_if_t<
    Counter::implemented_by_v<T>,
void >
print_counter(T& counter) {
    std::cout << "Counter with count " << counter.get_count() << std::endl;
}
//}

int main() {
    MyCounter counter { 3 };
    print_counter(counter);
}
```
{{< /godbolt >}}

## BTrait

We now introduce btrait which solves all of the above. The code might feel slightly verbose

First lets see the usage of our trait `Counter<>`

```cpp
template<typename T>
std::enable_if_t< btrait_require_v< T, Counter<> >, void >
print_counter(T& counter) {
    std::cout << "Counter with count " << Counter<>::interpret(counter).get_count() << std::endl;
}

void print_heterogeneous_counter(std::vector<btrait_dyn<Counter<>>& counters) {
    for (auto counter: counters) {
        std::cout << "Counter with count " << counter->get_count() << std::endl;
    }
}
```

Now the implementation for `MyCounter`

```cpp
// declare and define our struct
struct MyCounter {
    int count;
};

// implement trait for our struct
template <>
struct Counter<MyCounter>: Counter<>::__btrait_implementation<MyCounter> {
    static void max_count() const { return 100; }
private:
    static int _impl_get_count(Self& self) BTRAIT_METHOD_IMPL
    { return self_.count; };
    static void _impl_set_count(Self& self, int new_count) BTRAIT_METHOD_IMPL
    { self_.count = new_count; };
    Counter(MyCounter& self): self_(self) {}
    struct __btrait_check;
    friend class Counter<>;
};
Counter<MyCounter>::__btrait_check: Counter<>::__btrait_check {};
```

And now the trait definition:

```cpp
template <typename Self = void>
struct Counter {
private:
    struct __btrait_check: std::false_type;
    friend class Counter<>;
};

template <>
struct Counter<> {
    struct __btrait_impl {
        virtual int get_count() const = 0;
        virtual void set_count() const = 0;
        virtual void increment() const = 0;
    };

    template <typename Self>
    struct __btrait_impl {
        virtual int get_count() BTRAIT_METHOD_DECL { return Counter<Self>::_impl_get_count(self_); };
        virtual void set_count() BTRAIT_METHOD_DECL { return Counter<Self>::_impl_get_count(self_); };
        virtual void increment() BTRAIT_METHOD_DECL { return Counter<Self>::_impl_increment(self_); };

        typedef Self Self;
    private:
        static void _impl_increment(Self& self) BTRAIT_METHOD_IMPL
        { auto counter = Counter<>::interpret(self); counter.set_count(counter.get_count() + 1); };

        MyCounter& self_;
    };
};
```
