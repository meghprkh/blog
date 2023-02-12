---
title: Force Inline in C++
date: 2022-09-04T00:28:10+01:00
tags: [C++]
categories: C++
crossposts:
  reddit: https://www.reddit.com/r/cpp/comments/x5b6qk/force_inline_in_c/
---

{% setPageVar "code" %}

```cpp
#if defined(__clang__)
#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline
#elif defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define FORCE_INLINE __forceinline
#else
#error Unsupported compiler
#endif

FORCE_INLINE int decrement(int x) { return x - 1; }
int factorial(int x) { return (x == 0) ? 1 : x * factorial(x - 1); }

int main(int argc, char ** argv) {
    // Executed with args "a b c d e", should return 120 normally
    return factorial(decrement(argc));
}
```

{% endsetPageVar %}

{% setPageVar "code_error" %}

```cpp
#if defined(__clang__)
#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline
#elif defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define FORCE_INLINE __forceinline
#else
#error Unsupported compiler
#endif

FORCE_INLINE int decrement(int x) { return x - 1; }
int factorial(int x) { return (x == 0) ? 1 : x * factorial(x - 1); }

int main(int argc, char ** argv) {
    // Executed with args "a b c d e", should return 120 normally
    return factorial(decrement(argc));
}
```

{% endsetPageVar %}

Function calls are expensive. They require allocating a new stack frame, pushing params calling, return values. And lets not get started on calling conventions. `inline`, `always_inline` and `forceinline` are just hints. They dont always inline [^1] [^2].

Trust the compiler some say. Profile your code say the others. Use macros say the old and wise.

But what if you are developing a library and need to ensure that your method gets inlined? You cant say trust the compiler, because the users want to trust your library. You cant profile the code, because the application isn't your code.

We want a `FORCE_INLINE` keyword that _just works_, at least on the three major compilers - `g++`, `clang` and `msvc`. If it cant inline, it should error in a meaningful way.

The example:

```cpp
int decrement(int x) { return x - 1; }
int factorial(int x) { return (x == 0) ? 1 : x * factorial(x - 1); }
```

Putting our `FORCE_INLINE` keyword in front of `decrement` should inline.

It should either compile-time error or link-time error if put it in front of `factorial` (as the recursion depth / input argument is not known at compile-time).

```text
#if defined(__clang__)
#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline

#elif defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]] inline

#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define FORCE_INLINE __forceinline

#else
#error Unsupported compiler
#endif
```

Now lets check it in action:

| Compiler | Working case    | Error case    | Error type                                 |
| :------- | :-------------- | :------------ | :----------------------------------------- |
| Clang    | [clang_working] | [clang_error] | Linker error                               |
| GCC      | [gcc_working]   | [gcc_error]   | Compile error                              |
| MSVC     | [msvc_working]  | [msvc_error]  | Compile error (requires optimization flag) |

How it works:

- GCC would generate an error if it cant `always_inline` [^3]
- Clang:
  - Does not generate an error for non-inlinable `always_inline` functions. [^1]
  - Instead `gnu_inline` and `extern inline` forces it to not generate any code for the function [^4] [^5]
  - Thus give a linker error if it is not inlined
- MSVC:
  - Generates a warning for for non-inlinable `__forceinline` functions
  - But only if compiled with any "inline expansion" optimization (`/Ob<n>`) [^2]
  - This is present with `/O1` or `/O2`
  - We promote this warning to an error

**Note**: do not use this with virtual functions. You can't "force inline" them as they need to be pointed to at runtime.

GCC summarizes this as "An Inline Function is As Fast As a Macro" [^5]. Zig provides something similar as its [`callconv(.Inline)`](https://ziglang.org/documentation/0.9.1/#Functions)

Thus we can and should build syntactic sugar as functions instead of weird macros. Without any worries of performance.

[^1]: https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline
[^2]: https://docs.microsoft.com/en-us/cpp/cpp/inline-functions-cpp
[^3]: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#always_inline
[^4]: https://clang.llvm.org/docs/AttributeReference.html#gnu-inline
[^5]: https://gcc.gnu.org/onlinedocs/gcc/Inline.html

[clang_working]: {% godbolt_inline compiler_type="clang", language="c++", execution_args="a b c d e", code=page.code, url_only=true %}
[clang_error]: {% godbolt_inline compiler_type="clang", language="c++", execution_args="a b c d e", code=page.code_error, url_only=true %}
[gcc_working]: {% godbolt_inline compiler_type="gcc", language="c++", execution_args="a b c d e", code=page.code, url_only=true %}
[gcc_error]: {% godbolt_inline compiler_type="gcc", language="c++", execution_args="a b c d e", code=page.code_error, url_only=true %}
[msvc_working]: {% godbolt_inline compiler_type="msvc", language="c++", compiler_args="/Ob1", execution_args="a b c d e", code=page.code, url_only=true %}
[msvc_error]: {% godbolt_inline compiler_type="msvc", language="c++", compiler_args="/Ob1", execution_args="a b c d e", code=page.code_error, url_only=true %}
