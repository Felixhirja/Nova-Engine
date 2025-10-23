# Migrating TODOs into the Documentation Backlog

This guide explains how to migrate inline `TODO` comments or ad-hoc task lists into
our structured documentation backlog under `docs/todo/`. Following this workflow keeps
implementation notes discoverable and prevents regressions when refactoring large
systems such as the renderer or ECS scheduler.

## 1. Discover existing TODOs
1. Use ripgrep to collect every TODO across the repository:
   ```bash
   rg --no-heading --line-number "TODO" src include tests
   ```
2. Review the context for each result and note the owning subsystem (renderer, ECS,
   audio, menus, etc.).

## 2. Decide on the destination backlog
Map each TODO into one of the existing backlog files:

- `docs/todo/TODO_LIST.txt` for cross-team milestones or tasks that affect multiple systems.
- `docs/todo/todo_*.txt` (for example `todo_ecs.txt`, `todo_spaceship.txt`) when the work
  belongs to a specific subsystem.
- Create a new subsystem-specific TODO file if the work does not fit an existing document.

When in doubt, prefer the more focused subsystem backlog so the owning feature team can
triage the work without sifting through unrelated notes.

## 3. Capture the task details
For each migrated TODO:

1. Copy the actionable description into the appropriate backlog file.
2. Add surrounding context such as affected components, APIs, or reproduction steps.
3. Reference the original source location (file path and line number) so future
   migrations remain traceable.
4. Mark the entry with `[ ]` to indicate that the task still needs to be completed.

## 4. Clean up the original code comment
After documenting the TODO:

1. Replace the inline comment with a concise note referencing the backlog entry, e.g.
   `// See docs/todo/todo_ecs.txt: "Refactor system scheduler"`.
2. If the TODO was tracking temporary debug code or safety checks, evaluate whether the
   comment is still necessary once the backlog entry exists.

## 5. Verify and communicate
1. Run the relevant tests to ensure that removing or rewriting comments did not hide
   intentional behaviour.
2. Share the backlog updates in the next stand-up or PR description so the broader team
   knows where to find the migrated tasks.

Following this guide keeps the TODO backlog consolidated and ensures we capture enough
context to finish the work without trawling through historical commits.
