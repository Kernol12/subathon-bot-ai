# Pure delegation component

Task ID: ACCESS-009
Work type: implementation
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/645

The GitHub issue owns the current brief, acceptance, prerequisites and status.
This pointer supplies the canonical new-task binding required by the publication
checker; it does not duplicate or replace that brief.

Public contract: `core/include/subathon/core/delegated_permissions.hpp` v3.
Tier A applies to the tests and implementation stages. PR #648 holds review
receipts, including historical rejections and the additive regression reviews.

This component calculates permissions from supplied facts. Authoritative saves,
currentness, identity linking and real action holding remain open under ACCESS-003
(#249), ACCESS-010 (#646) and the linked owning issues.
