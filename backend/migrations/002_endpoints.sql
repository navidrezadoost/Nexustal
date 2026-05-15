create table if not exists endpoints (
    id uuid primary key default gen_random_uuid(),
    project_id uuid not null references projects(id) on delete cascade,
    name jsonb not null,
    description jsonb,
    type varchar(20) not null check (type in ('REST', 'GRAPHQL', 'SOAP', 'WEBSOCKET')),
    status varchar(20) not null default 'draft',
    created_by uuid not null references users(id) on delete restrict,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists rest_endpoints (
    endpoint_id uuid primary key references endpoints(id) on delete cascade,
    method varchar(16) not null,
    url_path text not null,
    query_params jsonb not null default '[]'::jsonb,
    headers jsonb not null default '[]'::jsonb,
    body jsonb not null default '{}'::jsonb
);

create table if not exists graphql_endpoints (
    endpoint_id uuid primary key references endpoints(id) on delete cascade,
    query text not null,
    variables jsonb not null default '{}'::jsonb,
    schema_url text
);

create table if not exists soap_endpoints (
    endpoint_id uuid primary key references endpoints(id) on delete cascade,
    wsdl_url text not null,
    operation varchar(255) not null,
    request_xml text not null
);

create table if not exists websocket_endpoints (
    endpoint_id uuid primary key references endpoints(id) on delete cascade,
    connection_url text not null,
    protocols jsonb not null default '[]'::jsonb,
    sample_messages jsonb not null default '[]'::jsonb
);

create index if not exists idx_endpoints_project_type on endpoints (project_id, type);
