FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends clang make && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . /app
# perform a clean build
RUN make clean >/dev/null 2>&1; make owl main

FROM ubuntu:24.04
WORKDIR /app
COPY --from=build /app/main /app/template.html ./
COPY --from=build /app/templates ./templates
COPY --from=build /app/static ./static
ENV PORT=8000
EXPOSE 8000
CMD ["./main"]
