/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  home: [
    "intro",
    "getting-started",
    {
      type: "category",
      label: "Examples",
      collapsed: false,
      items: [
        "examples/creating-a-endpoint",
        "examples/serving-static-files",
        "examples/server-side-rendering",
      ],
    },
  ],
  docs: [
    {
      type: "category",
      label: "API Reference",
      collapsed: false,
      items: ["api/http-server"],
    },
  ],
};

module.exports = sidebars;
