---
title: "BTrait - Combining Traits"
date: 2022-07-29T14:03:51+01:00
tags: [ C++, Rust, series-btrait-intro ]
categories: C++
draft: true
enableTOC: true
---

In the [previous post]({{< ref "/posts/BTrait-CPP-Rust-like-traits" >}}), we looked at how to define and implement simple traits

In this post we will look at how to combine them and pass them around, as that is what makes them really powerful. Otherwise each of them is quite basic on their own.

<!--more-->

*Second post of a [three-post series]({{< ref "/tags/series-btrait-intro" >}})*

## Cleanup

First lets rearchitect our code and clean it up a bit

Lets redine the trait to have everything related to it:

```cpp
template<>
struct Counter<void> {
    template<typename T>
    static constexpr Counter<T> interpret(T& t) {
        return Counter<T> { t };
    };

    template<typename T>
    struct is_implemented_by: Counter<T>::__btrait_implemented {};

    struct __btrait_abstract {
        virtual int getCount() = 0;
    };

    template <typename Self>
    struct __btrait_inherit: btrait_dyn<Counter> {
        constexpr __btrait_inherit (Self& self_): self(self_) {};

        inline virtual int getCount() { return Counter<Self>::__btrait_impl_getCount(self); }
    private:
        Self& self;
    };
};
```

We define `btrait_dyn` which can be used as a dynamic trait object

```cpp
template<typename Trait>
struct btrait_dyn: Trait::__btrait_abstract {};
```

Then to implement the trait

```cpp
template<>
struct Counter<MyCounter>: public Counter<void>::__btrait_inherit<MyCounter> {
    constexpr Counter (MyCounter& self_): __btrait_inherit<MyCounter>(self_) {};
    struct __btrait_implemented: std::true_type {};

private:
    __attribute__((noinline)) static int __btrait_impl_getCount(MyCounter& self) { return self.count; }
    friend class Counter<void>::__btrait_inherit<MyCounter>;
};
```

With this cleanup the following is achieved:
- Everything starts with `__btrait` prefix. This prefix should only be used in trait definitions and implementation and not anywhere else.
- Everything related to the trait definition is in one class
- You cant do something like `Counter<MyCounter>::interpret(other_type_of_counter)`
- It is explicit when you are taking a `dynamic reference` or a trait object, instead of compile time generic, as you have to state
  ```cpp
  void traitObjectExample(btrait_dyn<Counter<>> &counter);
  INSTEAD OF
  void traitObjectExample(Counter<> &counter);
  ```

We have also added another trait `Serde` (serialize + deserialize). The full code can be found [here](https://godbolt.org/z/6f6d545E8)

<details>

<summary>Serde trait</summary>

```cpp
template<>
struct Serde<void> {
    template<typename T>
    static constexpr Serde<T> interpret(T& t) {
        return Serde<T> { t };
    };

    template<typename T>
    struct is_implemented_by: Serde<T>::__btrait_implemented {};

    struct __btrait_abstract {
        virtual std::string serialize() = 0;
        virtual void deserialize(const std::string&) = 0;
    };

    template <typename Self>
    struct __btrait_inherit: btrait_dyn<Serde> {
        constexpr __btrait_inherit (Self& self_): self(self_) {};

        inline virtual std::string serialize() { return Serde<Self>::__btrait_impl_serialize(self); }
        inline virtual void deserialize(const std::string& serialized) { return Serde<Self>::__btrait_impl_deserialize(self, serialized); }
    private:
        Self& self;
    };
};

// implementation
template<>
struct Serde<MyCounter>: public Serde<void>::__btrait_inherit<MyCounter> {
    constexpr Serde (MyCounter& self_): __btrait_inherit<MyCounter>(self_) {};
    struct __btrait_implemented: std::true_type {};

private:
    __attribute__((noinline)) static std::string __btrait_impl_serialize(MyCounter& self) { return std::to_string(self.count); }
    __attribute__((noinline)) static void __btrait_impl_deserialize(MyCounter& self, const std::string serialized) { self.count = std::stoi(serialized); }
    friend class Serde<void>::__btrait_inherit<MyCounter>;
};
```

</details>

## Compile-time dispatch based on multiple traits

Lets assume we want to snapshot our counter every hour. We want to write a method `logAndSerialize` which would print the current value to the log and also serialize to a string that can be saved.

The parameter needs to have implemented both `Counter` and `Serialize` traits.

```cpp
template <typename T, typename... Traits>
struct btrait_require: std::conjunction<typename Traits::template is_implemented_by<T>... > {};

template <typename T, typename... Traits>
constexpr bool btrait_require_v = btrait_require<T, Traits...>::value;

template <typename CounterType>
std::enable_if_t<btrait_require_v<CounterType, Counter<>, Serde<>>, std::string>
__attribute_noinline__ logAndSerialize(CounterType& counter) {
    std::cout << "Serializing counter with value " << Counter<>::interpret(counter).getCount() << std::endl;
    return Serde<>::interpret(counter).serialize();
}

int main() {
    MyCounter counter { 10 };
    std::cout << "Counter value " << Counter<>::interpret(counter).getCount() << std::endl;
    std::string serialized = logAndSerialize(counter);
    counter.count = 25;
    std::cout << "Before deserialization: Counter value " << Counter<>::interpret(counter).getCount() << std::endl;
    Serde<>::interpret(counter).deserialize(serialized);
    std::cout << "After deserialization: Counter value " << Counter<>::interpret(counter).getCount() << std::endl;
    return 0;
}
```

Voila! This just works! [Godbolt](https://godbolt.org/z/rYf74GbjY)

It is also possible to write complicated requires clauses using `conjunction` and `disjunction`, however it complicates things and I dont see a use-case.

## Run-time trait object based on multiple traits

We want to get an trait object which implements multiple traits, eg `btrait_multi_dyn<Counter<>, Serde<> >`

<details>

<summary>Ugly C++ metaprogramming code</summary>

```cpp
template<typename Self, typename... Traits>
struct btrait_multi_dyn_specific;

template<typename... Traits>
struct btrait_multi_dyn {
private:
    template<typename Trait>
    struct is_in_pack : std::disjunction<std::is_same<Trait, Traits>...> {};
public:
    template<typename Trait>
    inline std::enable_if_t< is_in_pack<Trait>::value, btrait_dyn<Trait>& > as() {
        return std::get<btrait_dyn<Trait>&>(dyns_);
    }

    template<typename Self>
    static std::enable_if_t<btrait_require_v<Self, Traits...>, btrait_multi_dyn_specific<Self, Traits...>>
    inline  interpret(Self& self) {
        return btrait_multi_dyn_specific<Self, Traits...>(self);
    }

    template<typename Self>
    static std::enable_if_t<btrait_require_v<Self, Traits...>, btrait_multi_dyn>
    inline __btrait_interpret_multi_dyn_specific(btrait_multi_dyn_specific<Self, Traits...>& specific) {
        return btrait_multi_dyn(std::tie(static_cast< btrait_dyn<Traits>& >(
            std::get<decltype(Traits::interpret(specific.self_))>(specific.specializations_)
        )...));
    }
private:
    std::tuple<btrait_dyn<Traits>&...> dyns_;
    btrait_multi_dyn(std::tuple<btrait_dyn<Traits>&...> dyns): dyns_(dyns) {};
};


template<typename Self, typename... Traits>
struct btrait_multi_dyn_specific {
    inline operator btrait_multi_dyn<Traits...>()
    {
        return btrait_multi_dyn<Traits...>::template __btrait_interpret_multi_dyn_specific<Self>(*this);
    }
private:
    Self& self_;
    std::tuple<decltype(Traits::interpret(self_))...> specializations_;

    constexpr btrait_multi_dyn_specific(Self& self):
        self_(self),
        specializations_(std::make_tuple(Traits::interpret(self)...))
    {}
    friend class btrait_multi_dyn<Traits...>;
};
```
</details>

Used as:
```cpp
__attribute_noinline__
std::string logAndSerializeDyn(
    btrait_multi_dyn<Counter<>, Serde<> > serializableCounter
) {
    std::cout << "Serializing counter with value " << serializableCounter.as<Counter<>>().getCount() << std::endl;
    return serializableCounter.as<Serde<>>().serialize();
}
```

We end up with the following [full code](https://godbolt.org/z/vqj5nx1r8)

- It is probably not as efficient as native multiple inheritance as each trait has a "self" reference to be stored.
- This has `number of traits * 2` extra instructions when creating this dynamic trait object - one for storing `self` and another for each `vtable`
- Can probably be optimized somehow maybe, at least the `self` part

## Supertraits

It makes sense to define "supertraits" sometimes, example `SnapshottableCounter = Counter + Serde`.

This can define the additional `snapshot` method, which uses methods from the other two traits and also exposes them.

This is as easy as defining our trait's `__btrait_abstract` to inherit those of the other traits.

```cpp
    struct __btrait_abstract: Counter<>::__btrait_abstract, Serde<>::__btrait_abstract {
        virtual void snapshot() = 0;
    };

    template <typename Self>
    struct __btrait_inherit: btrait_dyn<SnapshottableCounter> {
        // Need to copy paste every method from Counter and Serde, but with changed body
        inline virtual int getCount() { return Counter<>::interpret(self).getCount(); }
    }
    static_assert(!std::is_abstract_v<__btrait_inherit<int>>);
```

Then it can be used as a native trait was used and `SnapshottableCounter::interpret(counter).getCount()` can be called too or passed as dyn

```cpp
__attribute_noinline__
void logAndSerializeDyn(
    btrait_dyn<SnapshottableCounter<>>&& counter
) {
    std::cout << "I can call counter methods " << counter.getCount() << std::endl;
    counter.snapshot();
}

logAndSerializeDyn(SnapshottableCounter<>::interpret(counter));
```

This does put some burden on trait define-r. However the `static_assert` helps catch any missing methods and trait implementors can go about their day normally.

[Full code on Godbolt](https://godbolt.org/z/eqE5Wzq5T)

## Summary

- We saw that compile time dispatching with multiple traits is very easy
- Runtime multi-trait does have additional cost
- Suertraits are easy to use/implement but a little cumbersome to define

In the next part we will see how to define static methods and constants
