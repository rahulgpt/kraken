# Deploying

Kraken is a normal long running server: it opens a port and waits for connections. Anywhere you can run a Linux binary that listens on a port, you can run Kraken. This page shows how the [live demo](https://kraken.rahulgpt.com) is hosted for free on AWS Lambda, and a few other options.

All hosting platforms tell your server which port to listen on with the `PORT` environment variable, so read it in `main`:

```c
char *port_env = getenv("PORT");
int port = port_env ? atoi(port_env) : 8000;

http_server_t *server = http_server_init(port, BACKLOG);
```

:::caution

Kraken doesn't speak HTTPS. On every option below, TLS is handled by the platform or a proxy in front of Kraken, which then talks plain HTTP to it.

:::

## Serverless on AWS Lambda (what the demo uses)

A small demo gets very little traffic, so paying for a server that runs all day is wasteful. Lambda only runs your code while a request is being handled and costs nothing while idle. The free tier (1 million requests a month) covers a personal project.

Lambda normally calls a function with an event object, not with an HTTP request on a socket. The [Lambda Web Adapter](https://github.com/awslabs/aws-lambda-web-adapter), published by AWS, bridges the two. It runs next to your server, receives each Lambda event, sends it to your server as a plain HTTP request on `127.0.0.1:$PORT`, and returns your response. Kraken runs without any changes.

```text
browser → Lambda Function URL (HTTPS) → Web Adapter → 127.0.0.1:8000 → kraken accept()
```

The repository has scripts for this in `deploy/lambda/`:

```bash
deploy/lambda/build.sh   # builds kraken for Amazon Linux (arm64) inside Docker
deploy/lambda/deploy.sh  # creates or updates the function and prints its public URL
```

`build.sh` compiles Kraken inside an Amazon Linux 2023 container, so the binary matches the OS Lambda runs on, and zips it with the templates, static files and this `bootstrap` script:

```sh
#!/bin/sh
cd "$LAMBDA_TASK_ROOT" || exit 1
exec ./main
```

`deploy.sh` needs the AWS CLI with credentials, then:

1. Creates an IAM role that only lets the function write logs.
2. Creates the function on the `provided.al2023` runtime (arm64, 128 MB) with the Web Adapter layer and `PORT=8000`.
3. Sets `AWS_LWA_READINESS_CHECK_PATH=/healthz`. The adapter calls this path until the server answers, then starts sending traffic.
4. Creates a public Function URL.

Running it again after a change rebuilds and updates the code.

### Cold starts

When nobody has visited for a few minutes, Lambda stops the process. The next request starts it again (a "cold start"). Kraken is a 200 KB binary with no runtime to load, so this takes well under a second. The live demo shows whether your request was a cold start, and how many requests that process has served since it started.

## Custom domain with Cloudflare

Function URLs look like `https://<id>.lambda-url.<region>.on.aws`. To serve the demo on `kraken.rahulgpt.com`, a small [Cloudflare Worker](https://developers.cloudflare.com/workers/) forwards requests to the Function URL. A plain `CNAME` doesn't work, because Lambda rejects requests whose `Host` header isn't its own URL.

```js
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const upstream = new URL(url.pathname + url.search, env.KRAKEN_ORIGIN);
    return fetch(new Request(upstream, request), { redirect: "manual" });
  },
};
```

The worker and its config are in `deploy/cloudflare/`. With your domain on Cloudflare, set your subdomain and Function URL in `wrangler.jsonc` and run:

```bash
cd deploy/cloudflare
npx wrangler deploy
```

Wrangler creates the DNS record and the HTTPS certificate for the custom domain. Workers are free for up to 100,000 requests a day.

## Other options

- **Container platforms** like Google Cloud Run, Fly.io or Render run a Docker image and pass `PORT`. Kraken's `Dockerfile` builds and runs the demo. Cloud Run and Fly.io can also scale to zero.
- **A VPS** (any Linux server): build with `make`, run `./main` under systemd, and put Caddy or nginx in front for HTTPS.
- **Your own machine**: run `./main` and expose it with a tunnel such as `cloudflared tunnel`.
