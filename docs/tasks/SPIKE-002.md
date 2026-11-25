# SPIKE-002 — Validate independent Twitch authorization

Module: Integrations. Kind: feasibility.

Specification references: AUTH-15, INTEGRATION-01.

## Goal

Check installation credentials, public device flow suitability, token types and optional sign-in without a project service.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Refine this task's bounded acceptance criteria before editing dependent production code.
- For a feasibility task, identify protocol/platform questions, reproduce the experiment and record limitations.
- For a contract task, define versioned examples and error cases before downstream implementation.
- For scaffolding/environment, distinguish successful checks from feature correctness.
- Use the applicable template, record exact evidence and leave a durable handoff.

Run relevant checks and record actual results in the PR and canonical proof.

## Documentation feasibility findings

The documented reference route uses an installation-owned confidential Twitch
application. The server keeps the client secret and obtains user grants through
the authorization-code flow for the broadcaster and bot identities. Where an
operation requires an app access token, the installation obtains one separately
through client credentials. Registration and user consent are distinct steps.
This design can operate directly between the installation and Twitch.
[Twitch app registration](https://dev.twitch.tv/docs/authentication/register-app/) ·
[Twitch token flows](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/)

A registered public client can obtain a user token through Device Code Grant
without holding a secret. It cannot obtain an app token through client
credentials. A public refresh token is single-use and expires 30 days after
issuance. Public Device Code Grant is therefore a candidate for user-token
operations only. Registration-free onboarding, refresh recovery after a lost
response, and operation-by-operation compatibility remain open.
[Twitch token flows](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#device-code-grant-flow) ·
[Twitch refresh tokens](https://dev.twitch.tv/docs/authentication/refresh-tokens/)

EventSub WebSocket subscriptions use a user token. Webhook subscriptions use
an app token. Chat operations depend on the token type, actor, and scopes for
the specific operation. Optional Twitch sign-in is separate from connector
consent. An OIDC ID token can convey identity and does not authorize a Helix
call or establish either connector grant.
[EventSub](https://dev.twitch.tv/docs/eventsub/) ·
[Chat authorization](https://dev.twitch.tv/docs/chat/authenticating/) ·
[Twitch OIDC](https://dev.twitch.tv/docs/authentication/getting-tokens-oidc/)

## Proposed offline qualification cases

These fake-adapter cases describe expected behavior for follow-on qualification.
They have not been implemented or run.

| Simulated provider result | Expected installation result |
| --- | --- |
| Device consent pending, then approved | Respect the polling interval and install the grant after validating the client, actor and required scopes |
| Consent denied or expired | Stop polling, preserve any existing connection and report the outcome |
| Validation returns an unexpected client, actor or scope set | Refuse the requested binding |
| Concurrent refresh requests for one public grant | Serialize ownership of the current single-use token |
| Refresh returns a replacement token | Durably retain the new generation before publishing the refreshed grant |
| Refresh response is lost after possible token consumption | Treat the grant as uncertain and require reconnection if recovery cannot be proved |
| Optional sign-in returns only an ID token | Use it for the sign-in purpose and install no connector grant |

[Twitch token validation](https://dev.twitch.tv/docs/authentication/validate-tokens/)

No app was registered, no device flow or token exchange ran, and no Twitch
API operation was attempted. Live feasibility and the public route's full MVP
operation coverage remain unverified. Any live experiment requires separate
owner approval.
