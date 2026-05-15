alter table users add column if not exists token_version uuid;
update users set token_version = gen_random_uuid() where token_version is null;
alter table users alter column token_version set default gen_random_uuid();
alter table users alter column token_version set not null;
