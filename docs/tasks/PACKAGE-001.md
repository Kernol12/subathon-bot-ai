# PACKAGE-001

Task ID: PACKAGE-001
Work type: implementation
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/525

The [native build evidence producer](../../deploy/native/native_build_evidence.py)
checks selected inputs against an observed build and a clean repeat. It emits
technical rows for [RELEASE-001](RELEASE-001.md). Actual release pins require
evidence from the selected target builds.
