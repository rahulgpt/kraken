FROM arm64v8/ubuntu
RUN apt-get update && apt-get install -y build-essential clang
WORKDIR /app
COPY . /app
RUN chmod +x scripts/build-lib.sh && scripts/build-lib.sh
# perform a clean build
RUN make clean && make
CMD ["./main"]