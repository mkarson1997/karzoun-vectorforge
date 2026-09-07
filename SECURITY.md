# Security Policy

## Supported versions

The repository is currently pre-1.0. Security fixes target the latest code on `main` and the most recent release once releases begin.

## Reporting

Do not open a public issue for a vulnerability that could expose users to unsafe index parsing, memory corruption, denial of service, or supply-chain compromise. Use GitHub's private vulnerability reporting when available, or contact the maintainer privately through the repository owner's published contact channel.

Please include the affected commit/version, a minimal reproducer, expected impact, and any sanitizer or CodeQL evidence.

## Security boundaries

VectorForge treats persisted index files as untrusted input. The loader validates format magic/version, dimensions, node counts, metric values, duplicate IDs, neighbor counts, graph references, truncation, and unexpected trailing bytes before returning a usable index.

The project does not claim that v0.1 is hardened for hostile multi-tenant service exposure. Network-service functionality is outside the current milestone.
