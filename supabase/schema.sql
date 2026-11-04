create table if not exists public.persons (
  id uuid primary key default gen_random_uuid(),
  owner_id uuid not null references auth.users(id) on delete cascade,
  kind smallint not null check (kind in (0, 1)),
  code text not null check (char_length(code) between 1 and 16),
  name text not null,
  fields jsonb not null default '[]'::jsonb,
  created_at timestamptz not null default now(),
  unique (owner_id, kind, code)
);

create table if not exists public.marks (
  person_id uuid not null references public.persons(id) on delete cascade,
  day date not null,
  status char(1) not null check (status in ('P', 'A')),
  primary key (person_id, day)
);

create table if not exists public.user_settings (
  owner_id uuid primary key references auth.users(id) on delete cascade,
  rest_mask integer not null default 1
);

alter table public.persons enable row level security;
alter table public.marks enable row level security;
alter table public.user_settings enable row level security;

create policy "persons_owner_all" on public.persons
  for all using (owner_id = auth.uid()) with check (owner_id = auth.uid());

create policy "marks_owner_all" on public.marks
  for all
  using (exists (select 1 from public.persons p where p.id = marks.person_id and p.owner_id = auth.uid()))
  with check (exists (select 1 from public.persons p where p.id = marks.person_id and p.owner_id = auth.uid()));

create policy "settings_owner_all" on public.user_settings
  for all using (owner_id = auth.uid()) with check (owner_id = auth.uid());

revoke all on public.persons, public.marks, public.user_settings from anon;
