# VectorForge architecture

VectorForge keeps its search-quality claims anchored to an exact baseline. The approximate graph exists to trade work for latency; the brute-force index remains the correctness reference used to measure that trade.

## System view

```mermaid
flowchart LR
    D[Vector dataset] --> E[Exact index]
    D --> G[Navigable graph ANN]
    Q[Query] --> E
    Q --> G
    E --> ER[Exact top-k]
    G --> AR[Approximate top-k]
    ER --> R[Recall@k comparison]
    AR --> R
    G <--> F[(Versioned .kvf file)]
    B[Benchmark CLI] --> E
    B --> G
```

The graph index supports squared L2 and cosine distance. Search readers share access; graph mutation and persistence changes take exclusive ownership through `std::shared_mutex`.

## Engineering invariants

| Concern | VectorForge behavior |
| --- | --- |
| Correctness baseline | Exact brute-force search is retained as the oracle for deterministic ANN recall tests. |
| ANN scope | v0.1 is a single-layer navigable proximity graph, not a claimed full hierarchical HNSW implementation. |
| Metric behavior | Squared L2 and cosine are explicit supported distance modes. |
| Quality measurement | Recall@k is measured against exact results and benchmark output pairs recall with latency. |
| Reader concurrency | Searches can execute concurrently under shared ownership. |
| Mutation safety | Insertions and persistence mutations require exclusive ownership. |
| Persistence versioning | Serialized graph data carries an explicit format version and magic validation. |
| Loader validation | Metadata bounds, duplicate IDs, degree/neighbor references, truncation and trailing bytes are rejected. |
| Native verification | GCC and Clang builds plus ASan/UBSan run in CI. |
| Static security analysis | CodeQL C/C++ uses `security-extended` queries. |
| Supply chain | Third-party CI/release actions are pinned to reviewed immutable commit SHAs. |
| Distribution | Tagged releases provide native binaries and a container image with checksums, SBOM and provenance. |

## Exact versus approximate path

The exact path examines the dataset without an ANN shortcut and establishes the expected nearest neighbors. The graph path explores a bounded neighborhood controlled by construction and search parameters. Tests and benchmarks compare the graph result to the exact result rather than treating speed alone as success.

This is why the benchmark reports recall and latency together: lower latency is only useful when the retained search quality is visible.

## Persistence boundary

The `.kvf` format is versioned and treated as untrusted input when loaded. Validation covers the properties documented by the implementation and test suite:

- file magic and version
- dimensions and node-count bounds
- configured metric
- duplicate vector IDs
- graph degree bounds
- neighbor references
- truncated input
- unexpected trailing bytes

The format is a graph persistence mechanism, not a compatibility promise across arbitrary future versions. Version checks make incompatibility explicit instead of silently interpreting unknown layouts.

## Concurrency boundary

v0.1 intentionally chooses a simple synchronization contract:

- searches acquire shared access and may run concurrently
- insertions require exclusive access
- persistence mutations require exclusive access

The project does not claim lock-free mutation. The current design prioritizes understandable correctness before more aggressive concurrency techniques.

## Verification and release boundary

CI verifies both GCC and Clang builds, tests the implementation, runs a smoke benchmark, and executes sanitizer coverage separately. CodeQL provides C/C++ static security analysis.

Release validation builds the native CLI and production container. Tagged releases publish native binaries for Linux, Windows and macOS, generate `SHA256SUMS.txt`, and publish a GHCR image with SBOM and provenance metadata. Third-party workflow dependencies are pinned to immutable reviewed commits.

## Explicit non-claims

VectorForge v0.1 does not claim:

- full hierarchical HNSW
- lock-free graph mutation
- that latency alone establishes ANN quality
- compatibility with unknown persistence-format versions
- production-scale benchmark results beyond the measurements actually run

The roadmap can add hierarchical HNSW and SIMD distance kernels once those behaviors are implemented and measured.