FROM debian:bookworm-slim AS build

ARG VERSION=dev

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DVECTORFORGE_BUILD_BENCHMARKS=OFF \
    && cmake --build build --parallel 2 \
    && ./build/vectorforge --help

FROM debian:bookworm-slim

ARG VERSION=dev
LABEL org.opencontainers.image.title="Karzoun VectorForge" \
      org.opencontainers.image.description="C++20 approximate nearest-neighbor search engine" \
      org.opencontainers.image.version="${VERSION}" \
      org.opencontainers.image.licenses="Apache-2.0"

RUN groupadd --system vectorforge \
    && useradd --system --gid vectorforge --no-create-home --shell /usr/sbin/nologin vectorforge

COPY --from=build /src/build/vectorforge /usr/local/bin/vectorforge

USER vectorforge
ENTRYPOINT ["/usr/local/bin/vectorforge"]
CMD ["--help"]
