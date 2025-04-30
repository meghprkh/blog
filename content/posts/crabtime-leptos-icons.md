---
title: Creating Rust macros via crabtime for including icons in Leptos
date: 2025-04-30 00:15:07
tags: [Rust, Leptos, crabtime]
# Dont use nunjucks / substitute {{str}} for this file
templateEngineOverride: md
---

TLDR: Using [`crabtime`][crabtime] to easily embed icons in [Leptos][leptos] like how you would do it in ReactJS 🦀🦀

Web development in Rust is exciting. While exploring Leptos, I ran into issues adding icons — but managed to solve them using `crabtime`. This posts shows its Zig-like compile-time power.

<!-- more -->

Icon fonts like FontAwesome are outdated; embedding SVGs with includes directly is now preferred. Libraries like [Lucide][lucide] use Javascript for this, and Rust ports like [`leptos_lucide`][leptos_lucide] & [`lucide-leptos`][lucide-leptos] offer similar functionality. This gives you tree shaking, only including the icons you need. Other things like `currentColor`, `size`, etc work too. However generating a component per icon, for 2000+ icons, compile times skyrocket.

The idea is to define a macro for defining an "icon component" and then explictly list the icons we may want to use in the project and only pay compile time costs for them.

## The `raw_icon_content!` macro

First lets create a small macro to include and escape the content of a macro. This is very similar to [std::include_str!](https://doc.rust-lang.org/std/macro.include_str.html#examples) but shows basic crabtime usage and assumes paths for our icons.

```rust
#[crabtime::expression]
#[macro_export]
pub fn raw_icon_content(icon_name: String) -> String {
    use std::path::Path;

    // Assumes lucide-static via npm, but any source for the icon files works.
    let icon_path = Path::new(crabtime::WORKSPACE_PATH)
        .join("node_modules/lucide-static/icons")
        .join(format!("{icon_name}.svg"));
    let icon_content = std::fs::read_to_string(&icon_path).unwrap_or_else(|_| {
        panic!("Could not open file {}", icon_path.to_string_lossy());
    });

    // Note the output is interepreted as Rust code, so it needs to be escaped
    format!("r##\"{icon_content}\"##")
}
```

To use this in a leptos component, do:

```rust
use leptos::{html, prelude::*};

view! {
    <i {html::inner_html(raw_icon_content!("menu"))} />
}
```

If we were to `cargo expand` the line `raw_icon_content!("menu")`, it would read something like

```rust
r##"
<!-- license lucide-static v0.503.0 - ISC -->
<svg width="24" height="24" stroke="currentColor"> <line .. /> </svg>
"##
```

(The exact SVG content can be found [here](https://github.com/lucide-icons/lucide/blob/main/icons/menu.svg?short_path=c68f00d))

## The `define_icon_component!` macro

However if we were to include each icon in every component we will have duplicated icon strings (not sure if optimization helps there). It also doesnt give us a nice "component" API. So lets define a `define_icon_component` macro where we see some ``crabtime` power

```rust
// filename: icons.rs

#[crabtime::function]
#[macro_export]
pub fn define_icon_component(component_name: String, icon_name: String) {
    let quoted_icon_name = format!("\"{icon_name}\"");
    crabtime::output!(
        #[component]
        pub fn {{component_name}}() -> impl IntoView {
            let content = raw_icon_content!({{quoted_icon_name}});
            view! {
                <i {html::inner_html(content)} />
            }
        }
    )
}

define_icon_component!(Menu, "menu");
```

This allows us to use the component as `<icons::Menu />` in other components. This is where crabtime really shines, allowing us to write Rust code as string template-y code generation. There are some annoyances about having to re-escape strings, especially since it has to be done out of line, but otherwise it works like magic!

## Handling `size` and `stroke_width`

Enhancing the `define_icon_component!` macro with some CSS and props allows us to handle sizing and stroke-width.

```css
/* file: index.css - or use inline css, or a library like stylers */
.icon > svg {
	width: inherit;
	height: inherit;
	stroke-width: inherit;
}
```

```rust
pub fn {{component_name}}(
    #[prop(default = 24)] size: u32,
    #[prop(default = 2)] stroke_width: u32,
) -> impl IntoView {
      // ....
          <i
              style:height=format!("{}px", size)
              style:width=format!("{}px", size)
              style:stroke-width=format!("{}px", stroke_width)
              class="icon"
              {html::inner_html(content)} />
}
```

and now we use it as

```rust
// file: navbar.rs
use crate::icons;

fn Navbar() {
    view! {
        <icons::Menu size={48} />
    }
}
```

## Asides

### Why not `include_view!`?

Leptos comes with a macro to include an external view, but this will try to parse it as the content of a `view!` macro. We want it to be inner html. Specifically HTML comments are a problem - [Github issue](https://github.com/leptos-rs/leptos/issues/3887)

### Crabtime and errors

I am unsure if `panic`-ing or `crabtime::error` is the way to go. In stable, `error` only logs it in the console, but does not fail the compilation. [Behaviour Reference](https://docs.rs/crabtime/latest/crabtime/#-logging--debugging).

### Crabtime and string escaping

I am unsure if there is a better way to do this, but string escaping is suggested [here](https://docs.rs/crabtime/latest/crabtime/#-output) under the note "Interpolated variables are inserted as-is, without additional quotes or escape characters."

It would be nice if crabtime provides an `crabtime::escape_string!` macro that works and can be used inline inside a `crabtime::output!` - [Github Issue](https://github.com/wdanilo/crabtime/issues/37)

```rust
fn escape_string(in_str: String) -> String {
    // this implementation uses hacky hash counting but it can be smarter if it wants
    let num_hashes = (0..).find(|&n| !in_str.contains(&"#".repeat(n))).unwrap();
    let hashes = "#".repeat(num_hashes);
    format!("r{hashes}\"{in_str}\"{hashes}")
}

#[crabtime::expression]
#[macro_export]
pub fn identity_string(icon_name: String) -> String {
    crabtime::output! {
        crabtime::escape_string!(icon_name)
    }
}

let a = identity_string!("A#\"b");
// should be
let a = r##"A#"b"##;
```

## Summary

This was my first time using both Leptos and crabtime. While the initial pain was high, and the macro errors are still hard to debug sometimes, crabtime really makes solving these problems fun imo.

<details>

<summary>Full code (click to expand)</summary>

```rust
// SPDX-FileCopyrightText: 2025 Megh Parikh (https://meghprkh.mit-license.org/)
// SPDX-License-Identifier: MIT or MIT-0

use leptos::{html, prelude::*};

#[crabtime::expression]
#[macro_export]
pub fn raw_icon_content(icon_name: String) -> String {
    use std::path::Path;

    fn escape_string(in_str: String) -> String {
        let num_hashes = (0..).find(|&n| !in_str.contains(&"#".repeat(n))).unwrap();
        let hashes = "#".repeat(num_hashes);
        format!("r{hashes}\"{in_str}\"{hashes}")
    }

    let icon_path = Path::new(crabtime::WORKSPACE_PATH)
        .join("node_modules/lucide-static/icons")
        .join(format!("{icon_name}.svg"));
    let icon_content = std::fs::read_to_string(&icon_path).unwrap_or_else(|_| {
        crabtime::error!("Could not open file {}", icon_path.to_string_lossy());
        panic!("Could not open file {}", icon_path.to_string_lossy());
    });

    escape_string(icon_content)
}

#[crabtime::function]
#[macro_export]
pub fn define_icon_component(component_name: String, icon_name: String) {
    let quoted_icon_name = format!("\"{icon_name}\"");
    crabtime::output!(
        #[component]
        pub fn {{component_name}}(
            #[prop(default = 24)] size: u32,
            #[prop(default = 2)] stroke_width: u32,
        ) -> impl IntoView {
            let content = raw_icon_content!({{quoted_icon_name}});
            view! {
                <i
                    style:height=format!("{}px", size)
                    style:width=format!("{}px", size)
                    style:stroke-width=format!("{}px", stroke_width)
                    class="icon"
                    {html::inner_html(content)} />
            }
        }
    )
}

define_icon_component!(Menu, "menu");
```

</details>

[crabtime]: https://docs.rs/crabtime/latest/crabtime/
[leptos]: https://leptos.dev/
[lucide]: https://lucide.dev/
[leptos_lucide]: https://github.com/opensourcecheemsburgers/leptos_lucide/
[lucide-leptos]: https://lucide.rustforweb.org/frameworks/leptos.html
