---
title: Rust Traits and Associated Types
date: 2025-04-05 13:40:20
tags: [Rust, Traits, Typeclasses]
enableTOC: true
---

Rust's traits have a nifty feature called [Associated Types](https://doc.rust-lang.org/stable/book/ch20-02-advanced-traits.html#specifying-placeholder-types-in-trait-definitions-with-associated-types). Rust's traits can also have [Generic Type Parameters](https://doc.rust-lang.org/stable/book/ch20-02-advanced-traits.html#default-generic-type-parameters-and-operator-overloading).

This post delves into some differences between the two, whether both are needed and my mental model of these concepts.

I am not a Rust pro, and came across this while relearning traits. Please let me know your comments!

<!-- more -->

## Are both features needed?

The Rust book itself asks this question:

> Associated types might seem like a similar concept to generics, in that the latter allow us to define a function without specifying what types it can
> handle. To examine the difference between the two concepts, we’ll look at an implementation of the Iterator trait on a type named Counter that
> specifies the Item type is u32:
>
> ```rust
> impl Iterator for Counter {
>     type Item = u32;
>     fn next(&mut self) -> Option<Self::Item> {}
> }
> ```
>
> This syntax seems comparable to that of generics. So why not just define the Iterator trait with generics, as shown in Listing 20-14?
>
> ```rust
> pub trait Iterator<T> {
>    fn next(&mut self) -> Option<T>;
> }
> ```
>
> <cite>[(From the Rust Book)](https://doc.rust-lang.org/stable/book/ch20-02-advanced-traits.html#specifying-placeholder-types-in-trait-definitions-with-associated-types)</cite>

It further states that

> The difference is that when using generics, as in Listing 20-14, we must annotate the types in each implementation; because we can also implement Iterator<String> for Counter or any other type, we could have multiple implementations of Iterator for Counter. In other words, when a trait has a generic parameter, it can be implemented for a type multiple times, changing the concrete types of the generic type parameters each time. When we use the next method on Counter, we would have to provide type annotations to indicate which implementation of Iterator we want to use.
>
> <cite>(From the Rust Book)</cite>

## Trait inferring - works even for generic traits

Part of the above statement is is not true. The following works without explicit qualification of the trait:

{% godbolt %}

```rust
trait MyTrait<T> {
    fn first_element(&self) -> Option<T>;
}
impl MyTrait<i64> for Vec<i64> {
    fn first_element(&self) -> Option<i64> { None }
}
fn main() {
    let v = vec![1, 2, 3];
    println!("{:?}", v.first_element());
}
```

{% endgodbolt %}

Thus if you have a generic type, and only one implementation of it, Rust is able to do a name-based lookup for it. This is similar to how it does name-based lookup for traits in the first place, i.e. on noticing `v.first_element()`, it:

1. Analyzes the type of the object the method is being called on, i.e. `v` -> `Vec<i64>`
2. List traits implemented for it, i.e. `Index, MyTrait` and so on
3. See if any of them have the matching method call, i.e. `first_element()`
4. If there are multiple matches, error out with an ambiguous match error

## Uniqueness constraint

However, with associated types, the useful contract that a trait can only be implemented once for a type remains. I.e, the following is possible with generic traits but not with associated types.

{% godbolt fragment="double_impl" %}

```rust
trait MyTrait<T> {
    fn first_element(&self) -> Option<T>;
}
// fragment double_impl
impl MyTrait<i64> for Vec<i64> {
    fn first_element(&self) -> Option<i64> { None }
}
impl MyTrait<u64> for Vec<i64> {
    fn first_element(&self) -> Option<u64> { None }
}
// endfragment double_impl
fn main() {
    let v = vec![1, 2, 3];
// fragment double_impl
// Need to disambiguate the trait to be called
println!("{:?}", MyTrait::<i64>::first_element(&v));
// endfragment double_impl
}
```

{% endgodbolt %}

So, are there other differences?

## More differences - returning trait objects

Let's assume we want to define `vec_iter` which wraps `vec.iter()` such that we can

```rust
fn main() {
    let v = vec![1, 2, 3];
    for x in vec_iter(&v) {
         println!("{}", x);
    }
}
```

In the case of associated types, we can specify the trait without specifying the associated type.

{% godbolt fragment="return_iterator" %}

```rust
// fragment return_iterator
// Valid to only specify Iterator without its associated type Item
fn vec_iter(x: &Vec<i64>) -> impl Iterator + '_ { x.iter() }
// endfragment return_iterator
fn main() {
    let v = vec![1, 2, 3];
// fragment return_iterator
// Valid to call this
let _ = vec_iter(v);
// endfragment return_iterator
}
```

{% endgodbolt %}

This itself is not very useful in most cases, as using it would have caused an error

{% godbolt fragment="return_iterator" %}

```rust
// Valid to only specify Iterator without its associated type Item
// fragment return_iterator
fn vec_iter(x: &Vec<i64>) -> impl Iterator + '_ { x.iter() }
// endfragment return_iterator
fn main() {
    let v = vec![1, 2, 3];
// fragment return_iterator

// Invalid as type of x is "opaque"
for x in vec_iter(&v) {
    println!("{}", x);
    // ^ error[E0277]: `<impl Iterator as Iterator>::Item` doesn't implement `std::fmt::Display`
}
// endfragment return_iterator
}
```

{% endgodbolt %}

Returning a dyn of a trait is invalid when associated types are not specified

{% godbolt fragment="return_dyn" %}

```rust
// Valid to only specify Iterator without its associated type Item
// fragment return_dyn
fn vec_iter(x: &Vec<i64>) -> dyn Iterator { x.iter() }
// error[E0191]: the value of the associated type `Item` (from trait `Iterator`) must be specified
// endfragment return_dyn
fn main() {
    let v = vec![1, 2, 3];
// fragment return_iterator
// Invalid as type of x is "opaque"
for x in vec_iter(&v) {
    println!("{}", x);
    // ^ error[E0277]: `<impl Iterator as Iterator>::Item` doesn't implement `std::fmt::Display`
}
// endfragment return_iterator
}
```

{% endgodbolt %}

Returning a generic trait is also (understandably) invalid

{% godbolt fragment="return_my_iterator"  %}

```rust
trait MyTrait<T> {
    fn first_element(&self) -> Option<T>;
}
impl MyTrait<i64> for Vec<i64> {
    fn first_element(&self) -> Option<i64> { None }
}
// fragment return_my_iterator
fn vec_iter(x: &Vec<i64>) -> impl MyTrait { x.first_element() }
    // ^ error[E0107]: missing generics for trait `MyTrait`
// endfragment return_my_iterator
fn main() {}
```

{% endgodbolt %}

Thus even in the above case, specifying the associated type is necessary in most cases.

## More differences - multiple associated types

In the case of atrait having multiple associated types, it is valid to associate only one of them and use it.

{% godbolt fragment="return_half_impl"  %}

```rust
// fragment return_half_impl
trait MyTrait {
    type A;
    type B;

    fn double_a(&self, a: Self::A) -> Self::A;
    fn double_b(&self, b: Self::B) -> Self::B;
}
// endfragment return_half_impl

struct ExampleStruct;
impl MyTrait for ExampleStruct {
    type A = u32;
    type B = f32;

    fn double_a(&self, a: Self::A) -> Self::A { a * 2 }
    fn double_b(&self, b: Self::B) -> Self::B { b * 2.0 }
}

// fragment return_half_impl
fn example_fn() -> impl MyTrait<A = u32> { ExampleStruct{} }

fn main() {
    let x = example_fn();
    println!("{}", x.double_a(2));
    println!("{}", x.double_b(2.0));
    // error[E0277]: `<impl MyTrait<A = u32> as MyTrait>::B` doesn't implement `std::fmt::Display`
}
// endfragment return_half_impl
```

{% endgodbolt %}

This does not seem as useful as traits are intended to be "small / composable", so if you have multiple associated types, you are likely using all of them or none of them.

## My mental model

- I believe that there is only one real difference between them is the single implementation of a trait constraint.
- For usage, you should treat them as generic traits everywhere. Always specify the associated types. Thus this difference only affects trait implementors.
- According to me, a slightly more intuitive syntax would be something that consistently enforces associated type specification in usage.

<details>

<summary>My unpolished syntax alternative</summary>

What about something like?

```rust
trait Iterator<Item> {
	// no type statement, it becomes a generic argument consistenly
	require unique implementation for <Item>
}

impl<T> Iterator<Item=T> for Vector<T> { ... }

trait ToPairs<Item1, Item2> {
	require unique implementation for <Item1, Item2>
}

trait NDShape<BaseUnit, PerimeterUnit, AreaUnit> {
	// PerimeterUnit & AreaUnit are associated types, but BaseUnit is not
	require unique implementation for <BaseUnit>
}
```

(I admit its not great though)

Aside: In nightly, there is `#![feature(associated_type_defaults)]` which affects trait implementors, not users, by providing defaults for associated types.

</details>
