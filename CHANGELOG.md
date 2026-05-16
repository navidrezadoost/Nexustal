# Changelog

All notable changes to this project will be documented in this file.

## 2026-05-16

### Added
- Project-scoped RBAC service and endpoint CRUD application services for REST, GraphQL, SOAP, and WebSocket endpoints.
- Endpoint domain model, repository contracts, controller surface, PostgreSQL endpoint persistence, and related unit tests.
- Durable SSE connection manager with Redis-backed fan-out and multi-connection lifecycle handling.
- Project and team repository adapters to support invitation acceptance and membership persistence.
- QuickJS sandbox scaffold with timeout enforcement, native function registration, `nt.*` API bindings, and sandbox tests.
- Environment domain models, repository contracts, Redis-backed environment cache, dynamic variable engine, request bundle types, request executor abstractions, and initial execution tests.
- Migration for project-level global environment variables.

### Changed
- Invitation acceptance now persists team or project membership and notifies the inviter.
- SSE controller now routes connections through the durable connection manager instead of direct Redis subscription wiring.
- Backend bootstrap now wires RBAC, endpoint services, SSE management, and new infrastructure adapters.
- Backend build configuration now supports optional QuickJS resolution through Conan, submodule, system library, or stub mode.
- Root build commands now include Conan-based backend configuration, full-tree backend builds, ctest-driven test execution, and a clean target.

### Notes
- QuickJS remains optional in this environment because local `cmake` and `conan` are unavailable for a full configure/build validation pass.
- Editor diagnostics reported no errors on the touched integration and test files after the changes were applied.
