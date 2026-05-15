# Nexustal

Nexustal is a self-hosted collaboration platform where projects and experts meet. The baseline product is a Docker-based modular monolith: one C++ engine, one PostgreSQL instance, one Redis instance, and a Nuxt frontend experience.

## Product baseline

- **Edition:** self-hosted, operator-friendly, no external SMTP or OAuth requirements
- **Engine:** C++23 modular monolith with clean architecture boundaries
- **Frontend:** Nuxt 3 application, ultimately served as part of the product experience
- **Identity:** local user directory administered inside the product
- **Execution:** embedded JavaScript sandbox for pre-request and test scripts
- **Data:** PostgreSQL for durable state, Redis for cache, presence, and SSE fan-out
- **Licensing:** setup wizard plus heartbeat-driven read-only enforcement

## Repository layout

- `backend/` — C++ engine, Dockerfile, and runtime migrations
- `frontend/` — Nuxt 3 application shell
- `wasm-libs/` — Wasm build outputs and packaging boundary
- `parsers/` — shared native/Wasm parsing and interpreter libraries
- `database/` — design-time schema notes and compatibility artifacts
- `docs/` — roadmap, architecture, ADRs, backlog
- `infra/` — supporting infrastructure assets
- `scripts/` — execution notes and bootstrap helpers

## Delivery model

Execution now follows these stages:

1. Docker scaffold and migration system
2. Local identity, teams, invitations, SSE
3. Polymorphic endpoint data model
4. Postman-style execution engine with embedded scripting
5. Nuxt and Wasm frontend experience
6. Setup wizard, licensing, heartbeat lock
7. RBAC, collaboration, integration hardening

Primary planning documents:

- [docs/ROADMAP.md](docs/ROADMAP.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/BACKLOG.md](docs/BACKLOG.md)
- [docs/adr/0001-system-vision.md](docs/adr/0001-system-vision.md)
- [docs/adr/0002-self-hosted-monolith.md](docs/adr/0002-self-hosted-monolith.md)

## Command index

Primary operator and development commands:

- `make infra-up` — start the self-hosted stack with Docker Compose
- `make infra-down` — stop the stack
- `make backend-configure` — configure the engine build tree
- `make backend-build` — build `nexustal_engine`
- `make backend-run` — run the local bootstrap binary
- `make db-migrate` — inspect runtime migration files
- `make docker-build` — build the engine container image
- `docker compose up --build` — launch the full local product stack

## Current status

The repository now reflects the self-hosted product direction. Phase 0 is scaffolded with a root Docker stack, runtime migration files, a container build, and a backend bootstrap that discovers migrations.
