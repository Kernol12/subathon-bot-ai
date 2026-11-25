Task ID: STORAGE-030
Work type: cleanup
Issue: https://github.com/Kernol12/subathon-bot-ai/issues/852

Advisor decision 834.11 places the existing read-only catalog checks in a
private Qt SQL target shared by the writer and migrations. Manifest, failure
and SQL row types can be used without either target. The existing callers
retain query execution, transaction ownership and clock-preflight metering.
Writer-open integration, mode transitions and verifier injection are outside
this cleanup.

The migration implementation and server CMake link lines are shared with
#607 and #834. #607 owns its catalog changes. #834 owns writer-open
integration and writer-mode transitions.
