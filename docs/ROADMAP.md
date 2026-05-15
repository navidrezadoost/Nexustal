# Nexustal Roadmap

## Engineering principles

- Preserve clean architecture inside the monolith.
- Prefer one implementation of critical logic reused natively and through Wasm.
- Keep setup self-hosted and operator-friendly.
- Centralize readonly enforcement for license failure scenarios.
- Measure performance before introducing low-level complexity.

## Phase 0 — Docker scaffold and runtime bootstrap

**Objective:** stand up the self-hosted baseline and runtime migration story.

### Deliverables
- root Docker Compose stack
- engine container image
- runtime migration directory under `backend/migrations`
- bootstrap binary that discovers migrations and install mode

### Tasks
1. **Root Docker stack**
   - Definition: run `db`, `cache`, and `engine` from one compose file.
   - Done when: `docker compose up --build` starts all three services.
2. **Backend containerization**
   - Definition: build `nexustal_engine` in a multi-stage Dockerfile.
   - Done when: the image starts and exposes ports `8080` and `9001`.
3. **Migration system bootstrap**
   - Definition: version SQL files and add a C++ migration discovery layer.
   - Done when: the engine can enumerate pending migrations.

### Commands
- `make infra-up`
- `make backend-configure`
- `make backend-build`
- `make backend-run`
- `make db-migrate`

## Phase 1 — Identity, teams, and notifications

**Objective:** implement local identity without SMTP or OAuth.

### Deliverables
- `users`, `teams`, `team_members`, `invitations`, `notifications` schema
- user and invitation interactors
- SSE notification stream contract

### Tasks
1. **User and role schema**
   - Definition: local credential storage with admin and active flags.
   - Done when: schema and ports support admin-created users.
2. **Invitation workflow**
   - Definition: persist invitations and acceptance state transitions.
   - Done when: acceptance updates membership deterministically.
3. **SSE delivery channel**
   - Definition: stream user-scoped notifications using Redis fan-out.
   - Done when: UI can receive live invitation events.

### Commands
- `make backend-build`
- `make backend-test`

## Phase 2 — Polymorphic endpoint model

**Objective:** support REST, GraphQL, SOAP, and WebSocket definitions under one project model.

### Deliverables
- canonical `endpoints` table
- type-specific detail tables
- endpoint repositories and service layer

### Tasks
1. **Endpoint schema**
   - Definition: normalize common endpoint metadata and split protocol-specific details.
   - Done when: migrations create the parent and child tables.
2. **Repository contracts**
   - Definition: isolate persistence for each endpoint type behind ports.
   - Done when: CRUD flows do not leak database detail into use cases.
3. **Endpoint service orchestration**
   - Definition: route commands to the correct detail repository.
   - Done when: one application service can create and read all endpoint types.

### Commands
- `make backend-build`
- `make backend-test`

## Phase 3 — Execution engine and scripting sandbox

**Objective:** deliver Postman-style request execution with embedded test scripts.

### Deliverables
- QuickJS integration boundary
- request lifecycle executor
- collection runner and persisted results

### Tasks
1. **Sandbox integration**
   - Definition: expose `nt.*` APIs inside a constrained JavaScript runtime.
   - Done when: scripts execute in isolated contexts with time limits.
2. **Request pipeline**
   - Definition: run variable substitution, pre-request script, HTTP execution, and test evaluation.
   - Done when: one endpoint request returns response plus test results.
3. **Collection runner**
   - Definition: execute chained endpoint collections with bounded concurrency.
   - Done when: runs persist into `test_runs` and `test_results`.

### Commands
- `make backend-build`
- `make backend-test`
- `make perf`

## Phase 4 — Frontend and Wasm experience

**Objective:** ship the Nuxt operator experience with large-document rendering.

### Deliverables
- Nuxt application shell
- Monaco-based script editor
- canvas-backed or virtualized large-document viewer
- SSE and WebSocket clients

### Tasks
1. **Nuxt shell**
   - Definition: implement wizard, auth, projects, endpoints, and runner UI routes.
   - Done when: core navigation works end to end.
2. **Document renderer**
   - Definition: use Wasm-backed line computation for very large API specs.
   - Done when: 100k-line documents remain responsive.
3. **Execution UX**
   - Definition: provide editor completions, snippets, response visualization, and test feedback.
   - Done when: a user can author and run scripts in one interface.

### Commands
- `make frontend-install`
- `make frontend-dev`
- `make wasm-build`

## Phase 5 — Setup wizard and licensing

**Objective:** make first-run provisioning fully self-serve.

### Deliverables
- wizard status and setup endpoints
- company registration flow
- license persistence and heartbeat worker
- readonly lock enforcement model

### Tasks
1. **Wizard workflow**
   - Definition: progress through database, admin, company, and completion steps.
   - Done when: a blank install can become operational through the UI.
2. **Heartbeat daemon**
   - Definition: report instance health to the licensing endpoint once per day.
   - Done when: successful heartbeats update local installation state.
3. **Readonly enforcement**
   - Definition: lock mutation endpoints after seven days without successful heartbeat.
   - Done when: write paths return a clear service-unavailable response.

### Commands
- `make backend-build`
- `make backend-test`

## Phase 6 — RBAC and collaboration

**Objective:** enforce permissions and enable shared editing.

### Deliverables
- capability model and middleware
- project-scoped permission checks
- collaboration room engine and frontend sync

### Tasks
1. **RBAC middleware**
   - Definition: map users, team roles, and project capabilities to route guards.
   - Done when: forbidden operations are denied consistently.
2. **Capabilities endpoint**
   - Definition: provide the frontend with capability flags for rendering decisions.
   - Done when: the UI can hide or disable unauthorized actions.
3. **Collaboration engine**
   - Definition: synchronize endpoint edits over WebSockets using deterministic merge semantics.
   - Done when: concurrent edits converge in automated tests.

### Commands
- `make backend-test`
- `make frontend-dev`

## Phase 7 — Final integration and release candidate

**Objective:** validate the full product flow and package the first releasable stack.

### Deliverables
- full-stack wizard tests
- execution-engine and lockout regression coverage
- production container build and compose baseline

### Tasks
1. **End-to-end verification**
   - Definition: test setup, login, project creation, execution, invitations, and lockout.
   - Done when: the compose stack passes an end-to-end suite.
2. **Performance tuning**
   - Definition: benchmark large documents, test execution throughput, and notification fan-out.
   - Done when: baseline thresholds are documented.
3. **Release packaging**
   - Definition: build the final engine image with runtime assets and migrations.
   - Done when: operators can start the product from documented commands.

### Commands
- `make ci`
- `make docker-build`
- `docker compose up --build`
