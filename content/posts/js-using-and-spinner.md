---
title: Javascript's using statement & a spinner using aria-busy
date: 2026-03-01 17:28:52
tags: [JavaScript]
---

JS's new `using` statement works very well as a loading indicator with minimal
CSS frameworks like [PicoCSS](https://picocss.com/) &
[Oat.Ink](https://oat.ink/) which use `aria-busy` to show a loading indicator.

<p class="codepen" data-height="300" data-pen-title="Untitled" data-slug-hash="LERpGmp" data-user="meghprkh" style="height: 300px; box-sizing: border-box; display: flex; align-items: center; justify-content: center; border: 2px solid; margin: 1em 0; padding: 1em;">
  <span>See the Pen <a href="https://codepen.io/meghprkh/pen/LERpGmp">
  Untitled</a> by Megh Parikh (<a href="https://codepen.io/meghprkh">@meghprkh</a>)
  on <a href="https://codepen.io">CodePen</a>.</span>
</p>
<script async src="https://public.codepenassets.com/embed/index.js"></script>

This trick lets you add a spinner to the clicked button or result container, and
optionally disable the button while it runs. It also destroys the loading
indicator if there was an error when using with `try/catch`.

The `using` statement is not baseline yet.
[caniuse using](https://caniuse.com/mdn-javascript_statements_using)

Note: HTMX's has this utility as `data-loading-aria-busy` in its
[`loading-states` extension](https://v1.htmx.org/extensions/loading-states/)

## Full Code

```javascript
/// Usage: `using _loading = new LoadingIndicator(element, disable?)`
/// Where `element` is the element to add the LoadingIndicator to and disable is
/// whether to disable the element
class LoadingIndicator {
	#element;
	#disable;
	constructor(element, disable = false) {
		this.#element = element;
		this.#disable = disable;
		this.#element.setAttribute("aria-busy", "true");
		if (this.#disable) this.#element.setAttribute("disabled", "true");
	}
	[Symbol.dispose]() {
		this.#element.removeAttribute("aria-busy");
		if (this.#disable) this.#element.removeAttribute("disabled");
	}
}
```

Example Usage

```javascript
document.querySelector("myBtn").addEventListener("click", async (e) => {
	const response = document.querySelector("#response");
	try {
		using _loading = new LoadingIndicator(e.target, true);
		using _loading2 = new LoadingIndicator(response, true);
		const res = await fetch("https://httpbin.org/delay/2");
		response.innerHTML += "<p>Hello world!</p>";
	} catch {
		response.innerHTML += "<p>Failed fetching!</p>";
	}
});
```

```html
<link
	rel="stylesheet"
	href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css"
/>

<main class="container">
	<button id="myBtn">Click me!</button>
	<article id="response">
		<h2>Response</h2>
	</article>
</main>
```

## Other minimal JS tricks I found useful

While building a small web frontend for a WASM playground around a CLI tool, I
explored this and a few related stuff. It was a great experience working within
the no-build web ecosystem.

- [`iconify-icon` web component](https://iconify.design/docs/iconify-icon/)
- Creating small no-framework UI web components, for example:
  - TabBar
  - A Splitter / Split Panel with [`split.js`](https://split.js.org/)
  - Sometimes I prefer having minimal web components, for example I have a
    `TabBar` component not a `Tabs` component uses PicoCSS's `role="group"` and
    a `target` attribute instead of having the CSS of the tab content also
    integrated. I found that this plays well for simple use cases especially
    when the flex layout of the tab bar could have led to conflicts.
