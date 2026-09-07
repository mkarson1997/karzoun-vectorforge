# VectorForge Roadmap

## v0.1 foundation

- [x] Exact-search truth baseline
- [x] Single-layer proximity graph
- [x] L2 and cosine distance
- [x] Concurrent readers
- [x] Versioned validated persistence
- [x] Recall@k benchmark path
- [x] Sanitizer and CodeQL gates

## v0.2 graph quality

- [ ] Hierarchical HNSW layers with randomized level assignment
- [ ] Neighbor-selection heuristic that preserves graph diversity
- [ ] Entry-point/max-level persistence
- [ ] Recall/latency sweeps across `M`, `efConstruction`, and `efSearch`
- [ ] Deletion tombstones and rebuild/compaction strategy

## v0.3 performance

- [ ] Runtime-dispatched AVX2/AVX-512/NEON distance kernels where supported
- [ ] Scalar/SIMD numerical equivalence tests
- [ ] Cache-aware vector storage layout
- [ ] Memory-mapped read path
- [ ] Multi-threaded query benchmark with p50/p95/p99 latency

## v0.4 integration

- [ ] Stable C API boundary
- [ ] Streaming bulk loader
- [ ] Optional HTTP service only after the core index has reproducible performance data
