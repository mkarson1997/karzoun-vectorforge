# Contributing

Contributions should preserve VectorForge's evidence-first engineering approach.

## Development flow

1. Create a focused issue for non-trivial changes.
2. Work on a branch.
3. Build with CMake and run the test suite.
4. For memory-sensitive changes, run ASan + UBSan.
5. Include benchmark evidence for performance claims.
6. Open a pull request and wait for CI/security checks.

## Performance claims

Do not describe a change as faster without a reproducible benchmark, dataset shape, compiler/build mode, and relevant recall/accuracy result. ANN changes must report quality and latency together.

## Safety

Never add real customer data, credentials, private embeddings, API tokens, or proprietary datasets to tests or benchmarks. Persistence fixtures must be synthetic.
