function asArray(stringOrArray) {
	if (stringOrArray === undefined) return [];
	return Array.isArray(stringOrArray) ? stringOrArray : [stringOrArray];
}

eleventyConfig.addCollection("categories", function (collectionApi) {
	let categorySet = new Set();
	collectionApi.getAll().forEach((item) => {
		asArray(item.data.categories).forEach((tag) => categorySet.add(tag));
	});
	return Array.from(categorySet);
});

eleventyConfig.addFilter(
	"filterCategory",
	function filterCategory(posts, category) {
		return posts.filter(
			(p) => asArray(p.data.categories).indexOf(category) != -1,
		);
	},
);
