add_executable(astro_contract_tests astro_contract_tests.cpp)
subathon_target_options(astro_contract_tests)
target_link_libraries(astro_contract_tests PRIVATE subathon_streamelements_astro)

foreach(row IN ITEMS completed-tip unknown-accept unknown-review unknown-record-only
        same-tip-redelivery cross-tip-payment-hold exact-decimal-spelling-conflict
        missing-tip-id empty-tip-id missing-transaction missing-donation missing-amount
        amount-is-string amount-is-zero currency-case currency-absent positive-overflow
        invalid-currency-before-overflow mismatched-payload-channel moderation-topic-ignored
        welcome-requests-tips canonical-ack-binds nonce-less-ack-refused wrong-nonce-ack-refused
        canonical-ack-change-refused rate-limit-retryable authorization-deadline-retryable
        subscribe-refused acknowledgement-timeout connection-rate-limit draining-reconnect
        graceful-reconnect-keeps-old replacement-welcome-handoff
        reconnect-rejection-fallback binary-transport-ignored
        missing-optional-source-fields submillisecond-source-time
        mismatched-envelope-room retired-ingest-gap wrong-topic-ack-refused empty-room-ack-refused
        rejected-moderation unusual-currency exact-millisecond-trailing-zero
        acknowledged-disconnect-reconnect unclassifiable-text-gap old-disconnect-during-handoff)
    add_test(NAME astro.${row} COMMAND astro_contract_tests ${row})
    set_tests_properties(astro.${row} PROPERTIES LABELS "streamelements-001;acceptance")
endforeach()
