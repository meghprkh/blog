// Add an event listener listening for scroll and on load
window.addEventListener("scroll", navHighlighter);
window.addEventListener("load", navHighlighter);

function navHighlighter() {
  if (typeof navHighlighter.headings == "undefined") {
    // Get all navHighlighter.headings using table of contents
    navHighlighter.headings = Array.from(
      document.querySelectorAll("#TableOfContents a")
    ).map((a) => document.getElementById(a.href.split("#")[1]));
  }

  // Now we loop through navHighlighter.headings to find first active header
  let activeIndex = navHighlighter.headings.findIndex(
    (current) => current.getBoundingClientRect().top >= 0
  );
  if (!navHighlighter.headings[activeIndex]) {
    activeIndex = navHighlighter.headings.length - 1;
    if (navHighlighter.headings[activeIndex].getBoundingClientRect().top >= 0)
      return;
  }
  if (navHighlighter.headings[activeIndex].getBoundingClientRect().top > 100)
    activeIndex = activeIndex - 1;
  if (window.innerHeight + window.scrollY >= document.body.offsetHeight)
    activeIndex = navHighlighter.headings.length - 1;

  // Add "active" and store as activeElement
  var element;
  if (navHighlighter.headings[activeIndex]) {
    const sectionId = navHighlighter.headings[activeIndex].getAttribute("id");
    element = document.querySelector(
      "#TableOfContents a[href*=" + sectionId + "]"
    );
  }
  if (navHighlighter.activeElement != element) {
    if (element) element.classList.add("active");
    if (navHighlighter.activeElement)
      navHighlighter.activeElement.classList.remove("active");
    navHighlighter.activeElement = element;
  }
}
