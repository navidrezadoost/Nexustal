# Nexustal Managed Backlog

## Active program view

### Epic A — Self-hosted foundation
- [x] Add root Docker Compose stack
- [x] Add backend container build
- [x] Add runtime migration directory
- [ ] Initialize Nuxt 3 application shell

### Epic B — Local identity and team model
- [ ] Define `User`, `Team`, `Invitation`, and `Capability` domain types
- [ ] Add `users`, `teams`, `team_members`, `invitations`, `notifications` repositories
- [ ] Add `CreateUserInteractor`
- [ ] Add `SearchUsersInteractor`
- [ ] Add `SendInvitationInteractor`
- [ ] Add `AcceptInvitationInteractor`

### Epic C — Notification and wizard flows
- [ ] Add SSE notification stream contract
- [ ] Add wizard status endpoint contract
- [ ] Add wizard database/admin/company interactors
- [ ] Add installation and license persistence ports

### Epic D — Endpoint catalog
- [x] Add polymorphic endpoint schema migrations
- [ ] Add endpoint aggregate and protocol-specific detail models
- [ ] Add endpoint repositories
- [ ] Add unified `EndpointService`

### Epic E — Execution engine
- [ ] Add QuickJS third-party integration strategy
- [ ] Add `nt.*` scripting host contract
- [ ] Add request bundle executor
- [ ] Add collection runner orchestration
- [ ] Add `test_runs` and `test_results` persistence adapter

### Epic F — Frontend and Wasm
- [ ] Add Nuxt route shell
- [ ] Add Monaco-based script editor integration
- [ ] Add Wasm document renderer packaging
- [ ] Add canvas-based API spec viewer
- [ ] Add SSE and WebSocket client stores

### Epic G — Licensing, RBAC, and collaboration
- [ ] Add heartbeat worker and readonly policy
- [ ] Add RBAC middleware and capability endpoint
- [ ] Decide collaboration merge model through ADR
- [ ] Add room engine and frontend synchronization

### Epic H — Quality and release
- [ ] Add wizard end-to-end tests
- [ ] Add execution-engine regression tests
- [ ] Add heartbeat lockout tests
- [ ] Add compose-based release verification

## Task discipline

Each task must carry:
- objective
- bounded scope
- dependencies
- done criteria
- measurable artifact

## Current priority order

1. initialize Nuxt application shell
2. add domain model for identity and teams
3. implement migration execution against PostgreSQL
4. define wizard contracts
5. integrate SSE notifications
6. begin endpoint service layer
