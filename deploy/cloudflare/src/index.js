// Puts kraken (running on AWS Lambda) behind a custom domain.
// Lambda Function URLs reject requests whose Host header is not their own
// *.lambda-url.*.on.aws hostname, so a plain CNAME can't point at them.
// This worker re-sends each request to the function URL instead.
export default {
    async fetch(request, env) {
        const url = new URL(request.url);
        const upstream = new URL(url.pathname + url.search, env.KRAKEN_ORIGIN);

        return fetch(new Request(upstream, request), { redirect: "manual" });
    },
};
