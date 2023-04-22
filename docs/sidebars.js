/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  home: ["intro", "getting-started"],
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
