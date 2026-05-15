# Nexustal Architecture

## System context

Nexustal is a self-hosted platform for API-centric teams. A local administrator provisions users, teams, projects, and licensing without external identity or email services. The product must feel enterprise-grade while remaining deployable as one Docker Compose stack.

## Deployment shape

### Runtime services
1. **`nexustal-engine`**
  - C++23 modular monolith
  - serves HTTP API, SSE, WebSocket collaboration, setup wizard, and heartbeat logic
2. **`nexustal-db`**
  - PostgreSQL 16
  - authoritative store for identity, projects, endpoints, runs, and licensing state
3. **`nexustal-cache`**
  - Redis 7
  - SSE fan-out, presence, short-lived environment state, and cached capabilities

## Architectural style

The engine remains internally layered:

- **Domain** — entities, value objects, policies, invariants
- **Application** — interactors, ports, capability evaluation, wizard workflow
- **Infrastructure** — PostgreSQL, Redis, QuickJS, HTTP client, bcrypt, migration execution
- **Interfaces** — Drogon controllers, SSE streams, WebSocket gateways, setup endpoints

This is a modular monolith, not an undisciplined monolith.

## Core subsystems

### 1. Identity and team management
- local `users` directory
- team ownership and role membership
- invitation workflow without email
- capability projection for UI and middleware

### 2. Endpoint catalog
- one canonical `endpoints` aggregate root
- detail storage split by protocol type: REST, GraphQL, SOAP, WebSocket
- bilingual names and descriptions stored as `jsonb`

### 3. Execution engine
- request bundle orchestration
- dynamic variable substitution
- pre-request and test script execution inside QuickJS
- persisted `test_runs` and `test_results`

### 4. Collaboration and notifications
- SSE notifications for invitations and system alerts
- WebSocket collaboration rooms for concurrent editing
- Redis-assisted pub/sub and presence tracking

### 5. Setup, license, and readonly enforcement
- first-run wizard
- installation and company records
- license key persistence
- heartbeat worker and seven-day readonly lock

## Canonical data model

### Identity and governance
- `installations`
- `companies`
- `licenses`
- `users`
- `teams`
- `team_members`
- `invitations`
- `notifications`

### Project and API model
- `projects`
- `endpoints`
- `rest_endpoints`
- `graphql_endpoints`
- `soap_endpoints`
- `websocket_endpoints`
- `environments`

### Runtime history and collaboration
- `test_runs`
- `test_results`
- `collaboration_sessions`
- `collaboration_events`
- `schema_version`

## Request and event surfaces

### HTTP
- setup wizard endpoints
- auth and capability endpoints
- project, endpoint, environment, and runner endpoints

### SSE
- user-scoped notification stream
- system warning events, including readonly warnings

### WebSocket
- collaboration rooms
- cursor, patch, and presence events

## Non-functional requirements

### Performance
- keep large-document rendering off naive DOM paths
- bound test execution concurrency
- minimize per-request contention in the engine

### Reliability
- deterministic migration ordering
- explicit readonly behavior when license checks fail
- durable audit history for invitations and test execution

### Maintainability
- separate use-case policies from frameworks
- track irreversible choices in ADRs
- evolve by vertical slices instead of broad speculative subsystems

## Major engineering decisions

1. **Self-hosted modular monolith first**
  - operational simplicity is favored over distributed deployment complexity.
2. **QuickJS for embedded scripting**
  - small footprint and straightforward C embedding make it suitable for safe request scripting.
3. **Wasm-backed large-document rendering**
  - line computation and tokenization stay reusable between native and browser paths.
4. **Central readonly policy**
  - license lockout will be enforced in one policy boundary, not scattered across controllers.

## Initial repository boundaries

- `backend/include/nexustal/` — public engine headers
- `backend/src/` — engine implementation
- `backend/migrations/` — canonical runtime SQL migrations
- `frontend/app/` — Nuxt application
- `parsers/` — native/Wasm shared libraries
- `docs/adr/` — architecture decisions

## Principal risks

1. QuickJS integration and sandbox safety
2. collaboration convergence semantics
3. licensing edge cases under intermittent connectivity
4. performance regressions from oversized API documents

## Control strategy

- keep the first slices thin and executable
- benchmark parser, renderer, and runner workloads
- prefer testable ports over framework-driven logic
- enforce every major product pivot through an ADR
