# Phase 0 Execution Notes

## Objective
Stand up the self-hosted Nexustal baseline: one Docker stack, one buildable engine, and versioned runtime migrations.

## Execution order
1. Configure and build the engine locally.
2. Verify migration discovery.
3. Start the Docker Compose stack.
4. Initialize the Nuxt application shell.

## Manual commands
- `make backend-build`
- `make backend-run`
- `make db-migrate`
- `make infra-up`
- `cd frontend && npx nuxi@latest init app --packageManager npm`
- `cd frontend/app && npm install`

## Exit criteria
- `backend/build/nexustal_engine` builds successfully.
- The engine discovers files under `backend/migrations/`.
- `docker compose up --build` starts `db`, `cache`, and `engine`.
- Nuxt is initialized under `frontend/app`.
