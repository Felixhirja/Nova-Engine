# Integrating Documentation TODOs into Active Workstreams

This guide explains how to fold the backlog under `docs/todo/` into day-to-day
feature work. Treating the documentation TODOs as first-class tasks keeps them
visible and prevents context from drifting out of date as systems evolve.

## 1. Establish a regular review cadence
- Add the relevant subsystem TODO files to the agenda of weekly discipline syncs.
- Skim every unchecked entry and highlight items that align with the current
  milestone or bug-fix window.
- Capture follow-up questions directly in the backlog entry rather than in
  transient chat threads so the context stays centralized.

## 2. Convert backlog notes into actionable tickets
- For each selected TODO, open a tracking ticket in the primary issue tracker.
- Copy the backlog text verbatim, then expand it with acceptance criteria,
  dependencies, and QA expectations.
- Link the ticket back to the backlog entry (for example, `[#1234]`) so future
  reviewers can trace the history.
- Update the backlog entry with the ticket identifier and the date it was
  promoted, e.g. `[ ] 2024-04-18 – Promote to #1234: Rebuild ECS job graph docs`.

## 3. Integrate into sprint or milestone planning
- During sprint planning, include the new tickets in the backlog refinement
  session so the responsible team can estimate the effort.
- Ensure documentation work is represented alongside feature work to avoid
  documentation debt accumulating during crunch periods.
- Track dependencies between documentation tasks and engine changes (e.g.
  renderer refactors) so they are scheduled together.

## 4. Keep the backlog synchronized
- When a ticket is completed, return to the corresponding backlog entry and
  replace `[ ]` with `[x]`, appending the completion date.
- If the scope changes, update the backlog entry with the new context so future
  audits know why the original note diverged.
- Remove obsolete entries altogether once the knowledge is captured in the main
  documentation set.

## 5. Communicate updates
- Summarize newly integrated documentation tasks in the release notes or weekly
  status emails.
- Share links to the updated docs in stand-ups to encourage engineers to consult
  them while implementing related features.
- Celebrate the closure of long-standing backlog items to reinforce the value of
  maintaining accurate documentation.

Following this workflow keeps the TODO backlog actionable and prevents
regressions in institutional knowledge as the Nova Engine continues to evolve.
