create table if not exists project_global_variables (
    project_id uuid primary key references projects(id) on delete cascade,
    variables jsonb not null default '[]'::jsonb,
    updated_at timestamptz not null default now()
);