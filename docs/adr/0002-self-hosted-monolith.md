# ADR 0002 — Self-Hosted Monolith as Product Baseline

## Status
Accepted

## Context
The free edition of Nexustal must be self-hosted, Docker-based, and free of external identity or email dependencies. Setup must work inside one local stack while still preserving clean internal boundaries.

## Decision
Nexustal will ship first as a modular monolith:
- one C++ engine process for HTTP API, SSE, WebSocket, migrations, heartbeat, and business logic
- one PostgreSQL instance for durable state
- one Redis instance for cache, presence, and notification fan-out
- one Nuxt frontend served as part of the product experience

Identity remains local to the installation. The setup wizard provisions the first administrator, company profile, and license material.

## Consequences
### Positive
- simpler operator experience
- fewer external dependencies
- easier local development and on-premise evaluation

### Negative
- the engine process carries more responsibilities
- careful module boundaries are required to avoid a big-ball-of-mud outcome

## Guardrails
- preserve clean architecture inside the monolith
- isolate infrastructure adapters from domain logic
- keep migrations explicit and versioned
- centralize readonly enforcement for license failures
