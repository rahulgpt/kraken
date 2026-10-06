#!/bin/bash
# Builds kraken for Lambda (Amazon Linux 2023, arm64) inside Docker and
# packages it as dist/kraken-lambda.zip.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
DIST="$HERE/dist"

rm -rf "$DIST" && mkdir -p "$DIST/pkg"

# compile in a throwaway copy so the local mac build artifacts are untouched
docker run --rm --platform linux/arm64 \
    -v "$ROOT":/src:ro -v "$DIST":/dist \
    public.ecr.aws/amazonlinux/amazonlinux:2023 bash -euc '
        dnf install -y -q clang make findutils >/dev/null
        cp -r /src /build && cd /build
        rm -rf out lib external/lib main
        find . -name "*.o" -delete
        make CC=clang owl main
        cp main /dist/pkg/
    '

cp "$HERE/bootstrap" "$ROOT/template.html" "$DIST/pkg/"
cp -r "$ROOT/templates" "$ROOT/static" "$DIST/pkg/"

(cd "$DIST/pkg" && zip -qr "$DIST/kraken-lambda.zip" .)
echo "Built $DIST/kraken-lambda.zip ($(du -h "$DIST/kraken-lambda.zip" | cut -f1))"
