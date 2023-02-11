const CODE_REGEX = /```([^\n]*)\n((.|\n)*)```/;

function processCode(code, comment_prefix, fragment) {
	var lines = code.split("\n");

	const START = `${comment_prefix} fragment ${fragment}`;
	const END = `${comment_prefix} endfragment ${fragment}`;
	const [fullCode, fragmentCode, _] = lines.reduce(
		([fullCode, fragmentCode, slicing], line) => {
			if (line == END) slicing = false;
			else if (line == START) slicing = true;
			else {
				if (slicing) fragmentCode += "\n" + line;
				fullCode += "\n" + line;
			}
			return [fullCode, fragmentCode, slicing];
		},
		["", "", !fragment]
	);
	return [fullCode.trim(), fragmentCode.trim()];
}

function godboltImpl(content, options) {
	const DEFAUT_COMPILERS = {
		"c++": "g121",
		rust: "r1620",
		clang: "clang1400",
		gcc: "g121",
		msvc: "vcpp_v19_latest_x64",
	};
	const COMMENT_PREFIX = {
		python: "#",
	};
	content = options.code || content;
	var code_match = content.match(CODE_REGEX);
	var language = options.language || code_match[1];
	var code = code_match[2];

	language = language.replace("cpp", "c++");
	var prism_language = language.replace("c++", "cpp");
	var [fullCode, fragmentCode] = processCode(
		code,
		COMMENT_PREFIX[language] || "//",
		options.fragment
	);

	var compiler =
		options.compiler || DEFAUT_COMPILERS[options.compiler_type || language];
	var session = {
		id: 1,
		language: language,
		compilers: [
			{
				id: compiler,
				options: options.compiler_args,
			},
		],
		executors: [
			{
				arguments: options.execution_args,
				compiler: {
					id: compiler,
					options: options.compiler_args,
				},
				compilerOutputVisible: "true",
			},
		],
		source: fullCode,
	};
	const clientstate_b64 = btoa(JSON.stringify({ sessions: [session] }));
	const gb_url = `https://godbolt.org/clientstate/${encodeURIComponent(
		clientstate_b64
	)}`;

	if (options.url_only) return gb_url;

	return `
<div class="code-play-button">
    <a href="${gb_url}" target="_blank">&#x25B6;</a>
</div>

\`\`\`${prism_language}
${fragmentCode}
\`\`\`
    `;
}

module.exports = function register(eleventyConfig) {
	eleventyConfig.addPairedNunjucksShortcode(
		"godbolt",
		function (content, options) {
			return godboltImpl(content, options || {});
		}
	);
	eleventyConfig.addNunjucksShortcode("godbolt_inline", function (options) {
		return godboltImpl(undefined, options);
	});
	eleventyConfig.addPairedShortcode("setPageVar", function (content, name) {
		this.page[name] = content;
		return "";
	});
};
