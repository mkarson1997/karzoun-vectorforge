# Karzoun VectorForge

[![CI](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/ci.yml/badge.svg)](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/ci.yml)
[![CodeQL](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/codeql.yml/badge.svg)](https://github.com/mkarson1997/karzoun-vectorforge/actions/workflows/codeql.yml)
[![Release](https://img.shields.io/github/v/release/mkarson1997/karzoun-vectorforge)](https://github.com/mkarson1997/karzoun-vectorforge/releases)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

VectorForge is a C++20 approximate nearest-neighbor search engine focused on measurable behavior, persistence safety, and systems-level engineering.

## Architecture at a glance

```mermaid
flowchart LR
    V[Input vectors] --> E[Exact baseline index]
    V --> G[Graph ANN index]
    Q[Query vector] --> E
    Q --> G
    E --> X[Exact top-k]
    G --> A[Approximate top-k]
    X --> M[Recall@k measurement]
    A --> M
    G <--> P[(Versioned .kvf persistence)]
    G --> C[Shared-reader / serialized-writer concurrency]
```

The exact index is the correctness oracle. ANN quality is evaluated against it using deterministic recall@k, while latency is reported alongside recall so performance claims stay measurable rather than decorative.

## Engineering proof points

| Area | What the repository demonstrates |
| --- | --- |
| Search systems | Exact nearest-neighbor search plus a navigable graph ANN implementation. |
| Algorithmic honesty | The current graph is explicitly not advertised as full hierarchical HNSW. |
| Metrics | Squared L2 and cosine distance with deterministic recall@k validation. |
| Persistence safety | Versioned binary format with magic/version, bounds, duplicate-ID, neighbor, truncation and trailing-byte validation. |
| Concurrency | Concurrent readers with serialized writers through `std::shared_mutex`. |
| Performance evidence | Reproducible benchmark reports recall and latency together. |
| Native quality | GCC + Clang CI plus ASan + UBSan validation. |
| Security analysis | CodeQL C/C++ runs with `security-extended` queries. |
| Supply-chain security | Third-party GitHub Actions are pinned to reviewed immutable commit SHAs. |
| Distribution | Native Linux/Windows/macOS binaries, SHA-256 manifest, GHCR image, SBOM and provenance. |

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

## Security and delivery controls

- GCC and Clang builds execute independently in CI
- ASan + UBSan validate memory and undefined-behavior paths
- CodeQL analyzes C/C++ with `security-extended`
- third-party GitHub Actions are pinned to immutable reviewed commits
- release validation builds both the native CLI and production container
- tagged releases publish checksummed native artifacts and a GHCR image with SBOM and provenance

## Architecture

See [`docs/architecture.md`](docs/architecture.md) for the system view, invariants and failure boundaries.

## Roadmap

The next meaningful step is hierarchical HNSW with reproducible recall/latency sweeps, followed by SIMD-dispatched distance kernels with scalar-equivalence tests.

## License

Apache-2.0.
