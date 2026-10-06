#!/bin/bash
# Deploys dist/kraken-lambda.zip to AWS Lambda behind a public Function URL.
# Safe to re-run: creates everything on the first run, updates the code after.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
FUNCTION="${FUNCTION:-kraken}"
ROLE="${FUNCTION}-lambda-role"
REGION="${AWS_REGION:-$(aws configure get region)}"
ZIP="$HERE/dist/kraken-lambda.zip"
# Lambda Web Adapter, published by AWS (github.com/awslabs/aws-lambda-web-adapter)
ADAPTER_LAYER="arn:aws:lambda:${REGION}:753240598075:layer:LambdaAdapterLayerArm64:30"
# the adapter polls this until the server is up, then starts sending traffic
ENVIRONMENT='Variables={PORT=8000,AWS_LWA_READINESS_CHECK_PATH=/healthz}'
# hard cap on parallel instances so a traffic spike can't run up a bill
MAX_CONCURRENCY="${MAX_CONCURRENCY:-5}"

export AWS_REGION="$REGION" AWS_PAGER=""

[ -f "$ZIP" ] || "$HERE/build.sh"

# execution role: only allowed to write its own logs
if ! ROLE_ARN=$(aws iam get-role --role-name "$ROLE" --query Role.Arn --output text 2>/dev/null); then
    echo "Creating role $ROLE"
    ROLE_ARN=$(aws iam create-role --role-name "$ROLE" --query Role.Arn --output text \
        --assume-role-policy-document '{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Principal":{"Service":"lambda.amazonaws.com"},"Action":"sts:AssumeRole"}]}')
    aws iam attach-role-policy --role-name "$ROLE" \
        --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
    sleep 10 # new roles take a moment before lambda can assume them
fi

if aws lambda get-function --function-name "$FUNCTION" >/dev/null 2>&1; then
    echo "Updating $FUNCTION"
    aws lambda update-function-code --function-name "$FUNCTION" \
        --zip-file "fileb://$ZIP" --architectures arm64 >/dev/null
    aws lambda wait function-updated-v2 --function-name "$FUNCTION"
    aws lambda update-function-configuration --function-name "$FUNCTION" \
        --environment "$ENVIRONMENT" >/dev/null
else
    echo "Creating $FUNCTION"
    aws lambda create-function --function-name "$FUNCTION" \
        --runtime provided.al2023 --architectures arm64 --handler bootstrap \
        --role "$ROLE_ARN" --zip-file "fileb://$ZIP" \
        --memory-size 128 --timeout 10 \
        --layers "$ADAPTER_LAYER" \
        --environment "$ENVIRONMENT" >/dev/null
fi
aws lambda wait function-updated-v2 --function-name "$FUNCTION"

aws lambda put-function-concurrency --function-name "$FUNCTION" \
    --reserved-concurrent-executions "$MAX_CONCURRENCY" >/dev/null \
    || echo "warning: could not reserve concurrency (account limit too low?)"

if ! URL=$(aws lambda get-function-url-config --function-name "$FUNCTION" --query FunctionUrl --output text 2>/dev/null); then
    echo "Creating public Function URL"
    URL=$(aws lambda create-function-url-config --function-name "$FUNCTION" \
        --auth-type NONE --query FunctionUrl --output text)
    aws lambda add-permission --function-name "$FUNCTION" --statement-id public-url \
        --action lambda:InvokeFunctionUrl --principal '*' --function-url-auth-type NONE >/dev/null
    aws lambda add-permission --function-name "$FUNCTION" --statement-id public-url-invoke \
        --action lambda:InvokeFunction --principal '*' --invoked-via-function-url >/dev/null
fi

echo "Live at $URL"
