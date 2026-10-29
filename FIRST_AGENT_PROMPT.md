# Copy this prompt into your CLion agent

Read AGENTS.md, docs/CLION_SETUP.md, docs/AGENT_WORKFLOW.md,
docs/TEST_STRATEGY.md, docs/contracts/time-credit-v0.md, and progress/state.json.
Then read the relevant sections and decision IDs in docs/specification.md.

Your first role is environment validation and test preparation, not feature implementation.

1. Claim BOOT-002. Configure/build/test scaffold-debug on this machine. Resolve routine
   toolchain issues and record actual versions/commands/results in a new evidence note.
2. Work on CONTRACT-001 and TEST-001. Independently review the draft time-credit
   contract and six GoogleTest cases against the specification. Expand meaningful
   boundary/failure coverage; document contract questions, including invalid inputs,
   overflow and interaction with lifecycle/debt before assuming those semantics.
3. Arrange a separate reviewer pass for REVIEW-001. If your environment offers only
   one agent, leave the review ready for a new reviewer session; do not self-certify it.
4. After review, complete TEST-002: build the acceptance preset and record the actual
   intended failing tests caused by the explicit FeatureNotImplemented stub. Missing
   dependencies/compilation failures do not establish a red baseline.
5. Do not implement calculate_time_credit or other production behavior in this session.
   Hand the reviewed baseline and evidence to the later implementation agent.

Update task statuses/notes, add a durable handoff, run the tooling checks and regenerate
PROGRESS.html. Do not mark live integrations, Windows/Wayland/OBS or unrun checks verified.
