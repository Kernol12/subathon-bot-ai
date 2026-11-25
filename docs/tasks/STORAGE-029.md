Task ID: STORAGE-029
Work type: implementation
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/850

Decisions 850.1–850.3 seal the v22 catalog and projection identities and
digests in a private migration-owned leaf. The existing manifest remains
the sole executable recipe. The drift test compares the compiled manifest
with the seal under the existing SQLite migration CI selection when
contribution review is enabled. No CI workflow change is required.

The migrations module owns the leaf and shares migration test registration
with #607. The server CMake link lines are shared with the writer owner.
Live catalog inspection at writer open and writer-mode transitions belong
to #834.
