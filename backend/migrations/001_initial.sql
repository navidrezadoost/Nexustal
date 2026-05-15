create extension if not exists pgcrypto;

create table if not exists schema_version (
    version varchar(64) primary key,
    applied_at timestamptz not null default now()
);

create table if not exists installations (
    id uuid primary key default gen_random_uuid(),
    instance_key varchar(128) not null unique,
    install_mode boolean not null default true,
    setup_step varchar(32) not null default 'db',
    readonly_mode boolean not null default false,
    last_heartbeat_success timestamptz,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists companies (
    id uuid primary key default gen_random_uuid(),
    legal_name varchar(255) not null,
    activity varchar(255),
    country_code varchar(8),
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists licenses (
    id uuid primary key default gen_random_uuid(),
    installation_id uuid not null references installations(id) on delete cascade,
    company_id uuid references companies(id) on delete set null,
    license_key varchar(255) not null unique,
    status varchar(32) not null default 'pending',
    issued_at timestamptz,
    expires_at timestamptz,
    payload jsonb not null default '{}'::jsonb,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists users (
    id uuid primary key default gen_random_uuid(),
    username varchar(120) not null unique,
    password_hash varchar(255) not null,
    display_name varchar(200),
    is_admin boolean not null default false,
    is_active boolean not null default true,
    token_version uuid not null default gen_random_uuid(),
    last_login_at timestamptz,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists teams (
    id uuid primary key default gen_random_uuid(),
    name varchar(200) not null,
    description text,
    owner_id uuid not null references users(id) on delete restrict,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists projects (
    id uuid primary key default gen_random_uuid(),
    team_id uuid not null references teams(id) on delete cascade,
    slug varchar(200) not null unique,
    name varchar(200) not null,
    description text,
    status varchar(32) not null default 'draft',
    created_by uuid not null references users(id) on delete restrict,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table if not exists team_members (
    team_id uuid not null references teams(id) on delete cascade,
    user_id uuid not null references users(id) on delete cascade,
    role varchar(32) not null check (role in ('owner', 'backend_dev', 'frontend_dev', 'viewer')),
    created_at timestamptz not null default now(),
    primary key (team_id, user_id)
);

create table if not exists invitations (
    id uuid primary key default gen_random_uuid(),
    from_user_id uuid not null references users(id) on delete cascade,
    to_user_id uuid not null references users(id) on delete cascade,
    team_id uuid references teams(id) on delete cascade,
    project_id uuid references projects(id) on delete cascade,
    role varchar(32) not null check (role in ('owner', 'backend_dev', 'frontend_dev', 'viewer')),
    status varchar(32) not null default 'pending' check (status in ('pending', 'accepted', 'rejected')),
    created_at timestamptz not null default now(),
    responded_at timestamptz
);

create table if not exists notifications (
    id uuid primary key default gen_random_uuid(),
    user_id uuid not null references users(id) on delete cascade,
    event_type varchar(80) not null,
    payload jsonb not null default '{}'::jsonb,
    is_read boolean not null default false,
    created_at timestamptz not null default now()
);

create index if not exists idx_users_username on users using btree (username);
create index if not exists idx_notifications_user_created_at on notifications (user_id, created_at desc);
create index if not exists idx_invitations_to_user_status on invitations (to_user_id, status);
