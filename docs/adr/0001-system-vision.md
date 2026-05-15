# ADR 0001 — System Vision and Delivery Shape

## Status
Accepted

## Context
Nexustal needs a delivery model that balances performance, maintainability, and product iteration speed. The system must process large API documents, support real-time collaboration, and reuse critical logic across backend and browser boundaries.

## Decision
The system will be delivered as a monorepo with:
- a C++23 backend core
- shared parser/runtime libraries compiled to native and Wasm artifacts
- a Nuxt 3 frontend shell for SSR and collaboration UX
- PostgreSQL and Redis as the initial state infrastructure

The delivery will proceed in phases, each producing a testable vertical increment.

## Consequences
### Positive
- clear ownership boundaries
- strong reuse of performance-critical logic
- disciplined evolution through ADRs and phased work

### Negative
- more demanding toolchain
- added integration complexity between C++, Wasm, and Nuxt

## Follow-up decisions
- collaboration algorithm choice
- backend framework selection confirmation
- Wasm packaging strategy
