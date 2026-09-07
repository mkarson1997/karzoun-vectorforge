# Karzoun VectorForge

[![CI](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/ci.yml/badge.svg)](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/ci.yml)
[![CodeQL](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/codeql.yml/badge.svg)](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/codeql.yml)
[![Release](https://img.shields.io/github/v/release/mkarson1997/karzoun-vectorforge)](https://github.com/mkarson1997/karzoun-vectorforge/releases)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

VectorForge is a C++20 approximate nearest-neighbor search engine focused on measurable behavior, persistence safety, and systems-level engineering.

## v0.1 scope

VectorForge v0.1 contains a **single-layer navigable proximity graph**. It is a graph ANN foundation inspired by the search mechanics used in modern graph indexes, but it is **not yet a full hierarchical HNSW implementation**. That distinction is intentional: the project only claims algorithms that are actually implemented and measured.

Implemented now:

- exact brute-force index used as the correctness baseline
- navigable proximity-graph ANN index
- configurable graph degree, construction breadth, and search breadth
- squared L2 and cosine distance
- shared-reader / serialized-writer concurrency using `std::shared_mutex`
- versioned binary persistence with magic/version checks, bounded metadata, duplicate-ID rejection, neighbor validation, truncation detection, and trailing-byte rejection
- deterministic recall@k tests against exact search
- concurrent-reader stress test
- standalone benchmark executable and CLI benchmark command
- GCC and Clang CI
- ASan + UBSan validation
- CodeQL C/C++ security analysis
- dependency automation for GitHub Actions

## Build

Requirements: CMake 3.24+ and a C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Run a benchmark

```bash
./build/vectorforge benchmark \
  --vectors 5000 \
  --dimensions 64 \
  --queries 200 \
  --k 10 \
  --ef-search 64
```

The benchmark reports recall and latency together because ANN speed without recall is not a meaningful quality claim.

## Distribution

Tagged releases publish native CLI binaries for Linux, Windows, and macOS plus `SHA256SUMS.txt`.

Container image:

```bash
docker run --rm ghcr.io/mkarson1997/karzoun-vectorforge:latest --help
```

Benchmark from the container:

```bash
docker run --rm ghcr.io/mkarson1997/karzoun-vectorforge:0.1.0 \
  benchmark --vectors 5000 --dimensions 64 --queries 200 --k 10 --ef-search 64
```

Release containers are built with SBOM and provenance attestations. Verify downloaded native artifacts against `SHA256SUMS.txt` before execution.

## Sanitizers

```bash
CC=clang CXX=clang++ cmake -S . -B build-san \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVECTORFORGE_ENABLE_SANITIZERS=ON
cmake --build build-san --parallel
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-san --output-on-failure
```

## Persistence format

The `.kvf` graph format is versioned. The loader validates file magic/version, dimensions, node counts, metrics, duplicate IDs, degree bounds, neighbor references, truncation, and unexpected trailing bytes.

## Concurrency model

Searches take a shared lock and may execute concurrently. Insertions and persistence mutations take exclusive ownership. v0.1 optimizes for correctness and a clear synchronization model before pursuing lock-free mutation.

## Roadmap

The next meaningful step is hierarchical HNSW with reproducible recall/latency sweeps, followed by SIMD-dispatched distance kernels with scalar-equivalence tests.

## License

Apache-2.0.
