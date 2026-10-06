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
        "examples/reading-request-data",
        "examples/status-codes-and-headers",
        "examples/serving-static-files",
        "examples/server-side-rendering",
      ],
    },
    "how-it-works",
    "deploying",
  ],
  docs: [
    {
      type: "category",
      label: "API Reference",
      collapsed: false,
      items: ["api/http-server", "api/http-req", "api/http-res"],
    },
  ],
};

module.exports = sidebars;
