create table if not exists environments (
    id uuid primary key default gen_random_uuid(),
    project_id uuid not null references projects(id) on delete cascade,
    name varchar(120) not null,
    variables jsonb not null default '{}'::jsonb,
    created_by uuid not null references users(id) on delete restrict,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists test_runs (
    id uuid primary key default gen_random_uuid(),
    project_id uuid not null references projects(id) on delete cascade,
    endpoint_id uuid references endpoints(id) on delete set null,
    triggered_by uuid not null references users(id) on delete restrict,
    status varchar(32) not null default 'queued',
    request_bundle jsonb not null default '{}'::jsonb,
    response_snapshot jsonb,
    started_at timestamptz,
    finished_at timestamptz,
    created_at timestamptz not null default now()
);

create table if not exists test_results (
    id uuid primary key default gen_random_uuid(),
    run_id uuid not null references test_runs(id) on delete cascade,
    test_name varchar(255) not null,
    status varchar(16) not null check (status in ('passed', 'failed', 'skipped')),
    message text,
    execution_time_ms integer,
    created_at timestamptz not null default now()
);

create table if not exists collaboration_sessions (
    id uuid primary key default gen_random_uuid(),
    project_id uuid not null references projects(id) on delete cascade,
    document_type varchar(64) not null,
    document_id uuid not null,
    started_by uuid not null references users(id) on delete restrict,
    snapshot jsonb not null default '{}'::jsonb,
    created_at timestamptz not null default now(),
    ended_at timestamptz
);

create table if not exists collaboration_events (
    id uuid primary key default gen_random_uuid(),
    session_id uuid not null references collaboration_sessions(id) on delete cascade,
    actor_id uuid not null references users(id) on delete restrict,
    operation_type varchar(32) not null,
    payload jsonb not null,
    created_at timestamptz not null default now()
);

create index if not exists idx_test_runs_project_created_at on test_runs (project_id, created_at desc);
create index if not exists idx_collaboration_events_session_created_at on collaboration_events (session_id, created_at);
