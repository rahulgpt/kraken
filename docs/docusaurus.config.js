/** @type {import('@docusaurus/types').Config} */
const config = {
  title: "Kraken",
  tagline: "A simple http server written in c",
  favicon: "img/favicon.ico",

  // Set the production url of your site here
  url: process.env.url || "https://krakendocs.netlify.app",
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: "/",
  organizationName: "rahulgpt", // Usually your GitHub org/user name.
  projectName: "kraken/docs", // Usually your repo name.

  onBrokenLinks: "throw",
  onBrokenMarkdownLinks: "warn",

  // Even if you don't use internalization, you can use this field to set useful
  // metadata like html lang. For example, if your site is Chinese, you may want
  // to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: "en",
    locales: ["en"],
  },

  presets: [
    [
      "classic",
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: require.resolve("./sidebars.js"),
          routeBasePath: "/",
          // Please change this to your repo.
          // Remove this to remove the "edit this page" links.
          editUrl: "https://github.com/rahulgtp/kraken/edit/main/docs/",
        },
        theme: {
          customCss: require.resolve("./src/css/custom.css"),
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      // Replace with your project's social card
      image: "img/docusaurus-social-card.jpg",
      navbar: {
        title: "🐙 Kraken",
        hideOnScroll: true,
        items: [
          {
            to: "/",
            activeBasePath: "docs",
            label: "Docs",
            position: "left",
          },
          {
            to: "api/http-server",
            activeBasePath: "api",
            label: "API",
            position: "left",
          },
          {
            href: "https://github.com/rahulgpt/kraken/issues/new/choose",
            label: "Feedback",
            position: "right",
          },
          {
            href: "https://github.com/rahulgpt/kraken",
            label: "GitHub",
            position: "right",
          },
        ],
      },
      footer: {
        style: "dark",
        links: [
          {
            title: "Docs",
            items: [
              {
                label: "Tutorial",
                to: "/docs/intro",
              },
            ],
          },
          {
            title: "Community",
            items: [
              {
                label: "Stack Overflow",
                href: "https://stackoverflow.com/questions/tagged/kraken",
              },
              {
                label: "Twitter",
                href: "https://twitter.com/RHGPT",
              },
            ],
          },
          {
            title: "More",
            items: [
              {
                label: "Blog",
                to: "/blog",
              },
              {
                label: "GitHub",
                href: "https://github.com/rahulgpt/kraken",
              },
            ],
          },
        ],
        copyright: `Copyright © ${new Date().getFullYear()} Rahul Gupta. Built with Docusaurus.`,
      },
      prism: {
        theme: require("prism-react-renderer/themes/github"),
        darkTheme: require("prism-react-renderer/themes/dracula"),
      },
      announcementBar: {
        id: "notice",
        backgroundColor: "#2250F4",
        isCloseable: false,
        textColor: "white",
        content: `
          <div style="font-weight: 700; margin: 2px;">
            <span >
            ❕Kraken is built as my final year project. It's not even close to a complete production ready http server.
            </span>
          </div>
          `,
      },
    }),
};

module.exports = config;
