set(CMAKE_AUTOMOC ON)
if(SUBATHON_BUILD_APPLICATION_SERVICE_TESTS)
    add_executable(server_application_service_environment application_service_environment.cpp)
    # The independent foundation cannot link or invoke the new service stubs.
    target_link_libraries(server_application_service_environment PRIVATE subathon_sqlite_application_holds Qt6::Test)
    target_compile_definitions(server_application_service_environment PRIVATE
        SUBATHON_SERVICE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/application_service_vectors.json"
        SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_application_service_environment)
    add_test(NAME scaffold.application_service_environment COMMAND server_application_service_environment)
    set_tests_properties(scaffold.application_service_environment PROPERTIES
        LABELS "scaffold;qt;server;service-environment" TIMEOUT 30)
endif()
if(SUBATHON_BUILD_SQLITE_HOLD_TESTS)
    add_executable(server_sqlite_application_hold_environment sqlite_application_hold_environment.cpp)
    target_link_libraries(server_sqlite_application_hold_environment PRIVATE subathon_sqlite_application_bundle Qt6::Test)
    target_compile_definitions(server_sqlite_application_hold_environment PRIVATE
        SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_hold_environment)
    add_test(NAME scaffold.sqlite_application_hold_environment COMMAND server_sqlite_application_hold_environment)
    set_tests_properties(scaffold.sqlite_application_hold_environment PROPERTIES
        LABELS "scaffold;qt;hold-environment" TIMEOUT 30)

    add_executable(server_sqlite_application_hold_acceptance sqlite_application_hold_tests.cpp)
    target_link_libraries(server_sqlite_application_hold_acceptance PRIVATE subathon_sqlite_application_holds Qt6::Test)
    target_compile_definitions(server_sqlite_application_hold_acceptance PRIVATE
        SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_hold_acceptance)
    # Reviewed hold cases retain separate processes and the independent fixture foundation.
    foreach(case IN ITEMS
        manifestPreservesFivePrefixesAndExactFrozenStepSix
        explicitMigrationPreservesEachSupportedPrefix
        globalTaggedSubjectsReturnCompleteOwnedViews
        currentViewAndFiniteHistoricalResolutionAreDifferentValues
        historyChecksHeadBeforeCursorAndReturnsExactContinuation
        detectionStagingIsProvisionalAndNeverAppliesWholeOverflow
        exactMutationReplayReturnsHistoricalVersionsBeforeCurrentFreshness
        sharedMutationIdentityAndImmutablePayloadCannotBeReinterpreted
        detectionClosedCauseExpectedAndLifetimeVersionPriority
        appendedDetectionPreservesFirstCauseAndSeparatesCurrentFromReplay
        subsetResolutionLeavesIndependentBlockerAndHistoricalRetry
        resolutionChecksCompleteSetThenEntryAndReplacementIdentity
        atomicReplacementAndLaterReoverflowPreserveOldCauseWithoutGap
        timerApplicationLinksAreDataAndCannotReplayBesideOriginal
        incomingReferenceAbsenceAndMismatchActuallyRollback
        failedCaptureAbsenceAndHistoricalStateAreNotCurrentHeadPolicy
        selectedCorruptionCannotBecomeAbsenceConflictOrPartialSuccess
        activeAuditsAllAnchorsInactiveHeadsAndSixtyFifthMetadata
        narrowActiveProjectionDoesNotCertifyUnselectedDocumentsOrOtherSubjects
        activeAnchorLookaheadIsCompleteBeforeAnyPayloadOrLimit
        metadataHasNoLifetimeMaterializationQuotaAndFullPagesOwnOnlySelectedBodies
        allComposedOptionsPrecedeInputsAndStaleTickets
        pureInputShapeCountsMatrixExactAndOrderingPreserveCallerStaging
        everyPureEntryPreservesTheAlreadyExhaustedSqlBudget
        inputOwnedBytesUseFrozenCompleteRequestAccounting
        returnedOwnedBytesAndNestedItemsMatchIndependentFrozenBoundaries
        selectedCountsAndFieldOveragesAreDistinctFromCorruption
        mutationInputItemsCountEveryNestedReplacementBeforeBodies
        fixedQueryInputsHaveTheirOwnOwnedBytePreflight
        lateSqlFailureRollsBackResolutionReplacementAndEarlierCallerStaging
        callerAndAllBorrowedPortsShareActualCumulativeWriterBudgets
        foreignConstructionAndRevokedTicketsKeepCapabilityTruthAndPurePriority
        closedSessionMisuseStaysSynchronousAndSticky
        stagedDetectionCannotTurnLostCommitAcknowledgementIntoDurableHeld
        uncertainRollbackOutranksLocalSelectedCorruptionWithoutKnownReason
    )
        add_test(NAME sqlite-hold.${case} COMMAND server_sqlite_application_hold_acceptance ${case})
        set_tests_properties(sqlite-hold.${case} PROPERTIES
            LABELS "sqlite-hold;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_ORIGINAL_CLAIM_TESTS)
    add_executable(server_sqlite_original_claim_environment sqlite_original_claim_environment.cpp)
    target_link_libraries(server_sqlite_original_claim_environment PRIVATE subathon_sqlite_application_bundle Qt6::Test)
    target_compile_definitions(server_sqlite_original_claim_environment PRIVATE
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_original_claim_environment)
    add_test(NAME scaffold.sqlite_original_claim_environment COMMAND server_sqlite_original_claim_environment)
    set_tests_properties(scaffold.sqlite_original_claim_environment PROPERTIES
        LABELS "scaffold;qt;original-claim-environment" TIMEOUT 30)

    add_executable(server_sqlite_original_claim_acceptance sqlite_original_claim_tests.cpp)
    target_link_libraries(server_sqlite_original_claim_acceptance PRIVATE subathon_sqlite_application_bundle Qt6::Test)
    target_compile_definitions(server_sqlite_original_claim_acceptance PRIVATE
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_original_claim_acceptance)
    # Keep query acceptance separate from the old-primitive environment diagnostics.
    foreach(case IN ITEMS
        frozenSelectorsPreserveGlobalPriorityAndRoles
        originalFilterExcludesFinancialReversalRows
        absentDoesNotAdmitCandidateOrAuditUnselectedNamespace
        legacyAppliedWithoutBundleNamespaceRemainsAbsent
        historicalMatchIgnoresCurrentDimensionsAndLaterHealthyPlanHead
        selectedClosureFailureRollsBackBeforeCatch
        pureInputPriorityPreservesHealthyCallerWork
        fixedSelectorDoesNotAcquireAggregateInputQuota
        bodyPlusOneDiscriminatorHasExactOwnedBoundary
        fullTwentyItemResultUsesOutputQuotaWithInputOne
        nonemptyOpaqueResultOwnsBytesAfterFacadeWriterAndFixtureClose
        ownStagedClaimIsProvisionalUntilRollbackOrCommitAndReopen
        cumulativeWriterBudgetExhaustionPrecedesNewReader
        foreignAndExpiredTicketsKeepActualCapabilityTruth
        terminalSessionMisuseIsSynchronousAndSticky
        existingRecordCannotTurnCommitAckLossIntoSuccess
        rollbackAckLossOutranksSelectedCorruption
        fullSelectedExactClassSizeAndGrammarRemainStrict
    )
        add_test(NAME sqlite-original-claim.${case} COMMAND server_sqlite_original_claim_acceptance ${case})
        set_tests_properties(sqlite-original-claim.${case} PROPERTIES
            LABELS "sqlite-original-claim;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_BUNDLE_TESTS)
    add_executable(server_sqlite_application_bundle_environment sqlite_application_bundle_environment.cpp)
    target_link_libraries(server_sqlite_application_bundle_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    target_compile_definitions(server_sqlite_application_bundle_environment PRIVATE
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_bundle_environment)
    add_test(NAME scaffold.sqlite_application_bundle_environment COMMAND server_sqlite_application_bundle_environment)
    set_tests_properties(scaffold.sqlite_application_bundle_environment PROPERTIES
        LABELS "scaffold;qt;bundle-environment" TIMEOUT 30)

    add_executable(server_sqlite_application_bundle_acceptance sqlite_application_bundle_tests.cpp)
    target_link_libraries(server_sqlite_application_bundle_acceptance PRIVATE subathon_sqlite_application_bundle Qt6::Test)
    target_compile_definitions(server_sqlite_application_bundle_acceptance PRIVATE
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_bundle_acceptance)
    # Reviewed application-bundle cases retain their baseline names and separate processes.
    foreach(case IN ITEMS
        manifestMatchesFrozenStepFiveAndPrefixes
        initialStateAndEmptyAbsenceUseRealCallerUnit
        readInputsResolvesCurrentAndHistoricalProvenance
        stateAndRecordedValuesOwnBytesAfterReopen
        explicitReceiptSubjectAndContributionZeroStayDistinct
        originalStageCommitsCompleteBundleAndPatchesOnlyApplication
        capDebtAndDeficitOutcomesPreserveSuppliedConservation
        sameUnitReadDuplicateAndClaimRollbackOrCommit
        existingIntentComparisonIgnoresFreshComputedOutcome
        operationSourceAndReversalIdentityConflictsDoNotWrite
        staleStateContributionPlanAndTokenRebindRefuse
        clockAdvancedBeforeIsDistinctFromPersistedHead
        sourceAndSelectedClosureFailuresRollbackBeforeCatch
        fullLookupRejectsMissingCountsGroupsAndCorruptSelectedBodies
        typedReferenceClassesNullsAndSparseMaxAreStrict
        returnedAndInputBoundsHaveExactOneOverCategories
        supportAnchorConditionsUseExactReturnedByteBoundary
        invalidInputPriorityPreservesHealthyUnit
        signedExactResidualAndAggregateConservationAreExact
        multiBasisRefundsUseOneCurrentRequestAndIndependentProgress
        zeroIncrementFullRefundAndStaleProgressStayDistinct
        reversalBasisReadReturnsActualCreditAndTerminalKey
        selectedSourceMetadataDoesNotCertifyUnselectedBodies
        pagedMetadataBeyondSixtyFourHasNoMaterializationQuota
        progressHeadPathsAndSparseSequenceAreValidated
        directAmountMustMatchCopiedRequestEvenWhenDeltaBalances
        callerCommitAckLossKeepsUnknownWithPhysicalWholeBundle
        cleanupAckLossOutranksSelectedFailure
        postCommitBodyFailurePreservesAccountingWithoutPayload
        genuineMidstageJobFaultRollsBackWholeCallerUnit
        cumulativeWriterBudgetsAbortObservedStaging
        borrowedStoreLifetimeAndCapabilityErrorsStayTruthful
        presentEmptyAndOpaqueEnvelopeFieldsAreLossless
        lookupUsesReturnedQuotaIndependentlyOfInputItems
        metadataRejectsRowsOutsidePositiveKeysets
    )
        add_test(NAME sqlite-bundle.${case} COMMAND server_sqlite_application_bundle_acceptance ${case})
        set_tests_properties(sqlite-bundle.${case} PROPERTIES
            LABELS "sqlite-bundle;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_PLAN_TESTS)
    add_executable(server_sqlite_application_plans_environment sqlite_application_plans_environment.cpp)
    target_link_libraries(server_sqlite_application_plans_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    target_compile_definitions(server_sqlite_application_plans_environment PRIVATE
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_plans_environment)
    add_test(NAME scaffold.sqlite_application_plans_environment COMMAND server_sqlite_application_plans_environment)
    set_tests_properties(scaffold.sqlite_application_plans_environment PROPERTIES
        LABELS "scaffold;qt;plan-environment" TIMEOUT 30)

    add_executable(server_sqlite_application_plans_acceptance sqlite_application_plans_tests.cpp)
    target_link_libraries(server_sqlite_application_plans_acceptance PRIVATE subathon_sqlite_application_plans Qt6::Test)
    target_compile_definitions(server_sqlite_application_plans_acceptance PRIVATE
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_application_plans_acceptance)
    # Reviewed behavior stays separate from required old-writer fixture diagnostics.
    foreach(case IN ITEMS
        revisionCodecMatchesIndependentFullRange
        revisionCodecRejectsMalformedBuffers
        exactCodecMatchesCanonicalLiteralVectorsAndLargeValue
        exactCodecRejectsInvalidAndOversizedComponents
        manifestPreservesPrefixesAndMatchesLiteralPlanStep
        initialRoundTripPreservesCompleteOwnedSnapshot
        revisionsAppendContiguouslyWithoutRewritingInitial
        quotesAndIndependentStatesRetainAllProvenanceAfterReopen
        orderedDuplicatesAndOpaqueBytesRemainExact
        initialDuplicateAndConflictKeepEarlierCallerWrites
        appendRefusalsDoNotWriteOrResolveProposedTargets
        queuedExpectedHeadHasOneWinner
        fullFindCapsNeverBecomeAppendQuotas
        targetedMaxIsExactWhileAggregateSparseHistoryRefuses
        targetedScopesDoNotCertifyUnselectedPayloads
        selectedOriginalAndTypedTargetsAreRequired
        selectedContributionIdentityDoesNotRequireSupportReceiptOrCurrentState
        inputPreflightPriorityAndRepresentability
        byteAndCollectionBoundsHaveExactEdges
        selectedMalformedScalarAndChildDataRollback
        selectedExactSqlTypesGrammarAndBoundsStayDistinct
        selectedHistoryAndOrdinalCorruptionRefuses
        laterMetadataPagesCannotHideCorruptCapture
        stagedRowsAndHeadRollbackWithCallerUnit
        cumulativeResultsRollbackObservedWrites
        inertStoreAndStagedResultsNeedCallerCommit
        foreignStaleTerminalAndOwnedLifetimes
        callerCommitAckLossRemainsUnknownWithPhysicalPlan
        cleanupAckLossOutranksSelectedFailure
    )
        add_test(NAME sqlite-plan.${case} COMMAND server_sqlite_application_plans_acceptance ${case})
        set_tests_properties(sqlite-plan.${case} PROPERTIES
            LABELS "sqlite-plan;acceptance;qt;server" TIMEOUT 20)
    endforeach()
endif()

if(SUBATHON_BUILD_QT_PROBE)
    add_executable(qt_environment_probe qt_environment_probe.cpp)
    target_link_libraries(qt_environment_probe PRIVATE Qt6::Core Qt6::Test)
    subathon_target_options(qt_environment_probe)
    add_test(NAME scaffold.qt_event_loop COMMAND qt_environment_probe)
    set_tests_properties(scaffold.qt_event_loop PROPERTIES LABELS "scaffold;qt")
endif()

if(SUBATHON_BUILD_SQLITE_MIGRATION_TESTS)
    add_executable(server_sqlite_migration_environment sqlite_migration_environment.cpp)
    target_link_libraries(server_sqlite_migration_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    subathon_target_options(server_sqlite_migration_environment)
    add_test(NAME scaffold.sqlite_migration_environment COMMAND server_sqlite_migration_environment)
    set_tests_properties(scaffold.sqlite_migration_environment PROPERTIES
        LABELS "scaffold;qt;migration-environment" TIMEOUT 30)

    add_executable(server_sqlite_migration_acceptance sqlite_migration_tests.cpp)
    target_link_libraries(server_sqlite_migration_acceptance PRIVATE subathon_sqlite_migrations Qt6::Test)
    target_compile_definitions(server_sqlite_migration_acceptance PRIVATE
        SUBATHON_MIGRATION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/migration_catalog_vectors.json")
    subathon_target_options(server_sqlite_migration_acceptance)
    # Reviewed migration cases retain their baseline names and separate processes.
    foreach(case IN ITEMS
        canonicalPayloadAndDigestMatchLiteralVector
        canonicalNullUnicodeOrderingAndInvalidText
        canonicalCodecPreservesInitialBom
        storedSchemaPreservesInitialBom
        freshAdvanceCommitsWholePrefix
        supportedOldAdvancesOnceAndCurrentDoesNotReplay
        inspectionReportsFreshOldAndCurrentWithoutAdvancement
        foreignPartialAndMismatchingCatalogsAreRefused
        futureAndUnsupportedOldVersionsAreRefused
        invalidManifestsAreRejectedBeforeAdmission
        historyAndSchemaMismatchesAreRefused
        storedTypesRangesAndCardinalityAreValidated
        checkConstraintIntegrityFailurePreservesData
        foreignKeyFailureCannotPassIntegrity
        failedMigrationRollsBackDdlHistoryAndPriorWrites
        cancellationRollsBackWholeBootstrapAndPrefix
        busyMigrationDoesNotReplay
        lostCommitAckFailsClosedWithPhysicalTarget
        lostRollbackAckOverridesSqlFailure
        cumulativeBudgetsAndManifestLimitsHaveExactBoundaries
        diagnosticShadowsAreRefusedWithoutRepair
        utf16StoresAreRefusedWithoutConversion
        ownedRequestReportAndAdmissionRemainBounded
        shutdownAndCompletionLifetimePreserveWriterRules
    )
        add_test(NAME sqlite-migrations.${case} COMMAND server_sqlite_migration_acceptance ${case})
        set_tests_properties(sqlite-migrations.${case} PROPERTIES
            LABELS "sqlite-migrations;acceptance;qt;server" TIMEOUT 20)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_RETAINED_TESTS)
    add_executable(server_sqlite_retained_context_environment sqlite_retained_context_environment.cpp)
    target_link_libraries(server_sqlite_retained_context_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    target_compile_definitions(server_sqlite_retained_context_environment PRIVATE
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_retained_context_environment)
    add_test(NAME scaffold.sqlite_retained_context_environment COMMAND server_sqlite_retained_context_environment)
    set_tests_properties(scaffold.sqlite_retained_context_environment PROPERTIES
        LABELS "scaffold;qt;retained-environment" TIMEOUT 30)

    add_executable(server_sqlite_retained_context_acceptance sqlite_retained_context_tests.cpp)
    target_link_libraries(server_sqlite_retained_context_acceptance PRIVATE subathon_sqlite_retained_context Qt6::Test)
    target_compile_definitions(server_sqlite_retained_context_acceptance PRIVATE
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_retained_context_acceptance)
    # Reviewed retained cases preserve their baseline names and separate processes.
    foreach(case IN ITEMS
        manifestMatchesApprovedRetentionMigration
        retainsExactBytesAndResolvesAfterReopen
        presentEmptyRoundTripsAndRetainsAsDuplicate
        opaqueReferencesAndFullRangeBytesStayDistinct
        sameBytesRemainSeparateAcrossNamespaces
        exactDuplicateAndConflictDoNotMutate
        optionalConfigurationLinkPresenceIsImmutable
        missingAndWrongNamespaceLinksCannotCreateContext
        newVersionsPreserveHistoricalReferences
        sameSessionStagesConfigurationAndContext
        callerOwnsCommitRollbackAndConflictPolicy
        invalidInputPrecedencePreservesHealthySession
        configuredBoundsAndSelectedLimitsAreExact
        selectedInvalidRowsAndDanglingLinksAbort
        ownedResultsAndCapabilityFailuresRemainSafe
        sqlFailureAndUnknownOutcomeAreNotSuccess
    )
        add_test(NAME sqlite-retained.${case} COMMAND server_sqlite_retained_context_acceptance ${case})
        set_tests_properties(sqlite-retained.${case} PROPERTIES
            LABELS "sqlite-retained;acceptance;qt;server" TIMEOUT 20)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_WORK_TESTS)
    add_executable(server_sqlite_work_acceptance sqlite_writer_work_tests.cpp)
    target_link_libraries(server_sqlite_work_acceptance PRIVATE
        subathon_sqlite_writer subathon_receipt_intake Qt6::Test)
    subathon_target_options(server_sqlite_work_acceptance)
    # Reviewed work cases run in isolated processes, preserving the baseline names.
    foreach(case IN ITEMS
        mixedBatchAndWorkAreFifo
        dynamicReadDecideWriteIsAtomic
        serviceCommitIsVisibleBeforeBodyReturns
        returnWithoutCommitRollsBack
        exceptionBeforeCommitRollsBackAndQueueContinues
        caughtSqlErrorStaysStickyAndCannotCommit
        busyBeginDoesNotReplay
        failedCommitRollsBackBeforeThrowing
        exceptionAfterCommitPreservesCommittedOutcome
        lostCommitAcknowledgmentFailsClosed
        lostRollbackAcknowledgmentFailsClosed
        retainedCapabilitiesCannotReachLaterWork
        capabilitiesAfterWriterDestructionAreRevoked
        foreignCapabilitiesDoNotPoisonOwner
        secondBeginAndSqlControlAreRejected
        cumulativeBudgetsHaveExactBoundaries
        activeCapacityLastsUntilBodyReturns
        shutdownDrainsWorkAndBatches
        throwingOrDestroyingCompletionIsContained
        ownedRequestAndResultOutliveCallerAndSession
        unknownOutcomeIsNotKnownRepositoryFailure
        returnWithoutBeginHasNoSuccessPayload
        admissionErrorsDoNotInvokeWork
        cleanupIsNonthrowingAndDoesNotRepeatRollback
    )
        add_test(NAME sqlite-work.${case} COMMAND server_sqlite_work_acceptance ${case})
        set_tests_properties(sqlite-work.${case} PROPERTIES
            LABELS "sqlite-work;acceptance;qt;server"
            TIMEOUT 15)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_WRITER_TESTS)
    add_executable(server_sqlite_writer_acceptance sqlite_writer_tests.cpp)
    target_link_libraries(server_sqlite_writer_acceptance PRIVATE subathon_sqlite_writer Qt6::Test)
    subathon_target_options(server_sqlite_writer_acceptance)
    # Separate processes isolate temporary databases and report each reviewed case.
    foreach(case IN ITEMS
        opensWalFullOnOwnerThread
        commitsFifoBatchesBeforeCompletion
        rejectsDuplicateWriterUntilShutdown
        readerCannotWriteEvenAfterQueryOnlyOff
        readerIsOwnedByItsOpeningThread
        writerRejectsForeignThreadBeforeAdmission
        validatesLimitsAndBatchBeforeAdmission
        pendingCapacityRejectsQueuedOverflow
        rollsBackWholeBatchOnSqlFailure
        busyBeginFailsWithoutReplay
        readerSnapshotDoesNotBlockWalCommit
        shutdownDrainsAcceptedWorkAndRejectsNewWork
        shutdownIsIdempotentAndReleasesConnection
        closedLifecycleIsExplicit
        readerMissingFileDoesNotCreateDatabase
        failedOpenDoesNotPublishAWriter
        completionCanEnqueueAfterCommit
        rejectsTransactionControlStatements
        completionExceptionDoesNotUndoCommitOrStopQueue
        boundsResultRowsAndRollsBack
        boundsResultBytesAndRollsBack
        failedCommitRollsBack
    )
        add_test(NAME sqlite-writer.${case} COMMAND server_sqlite_writer_acceptance ${case})
        set_tests_properties(sqlite-writer.${case} PROPERTIES
            LABELS "sqlite-writer;acceptance;qt;server"
            TIMEOUT 15)
    endforeach()

    add_executable(server_sqlite_writer_ownership_regression sqlite_writer_ownership_tests.cpp)
    target_link_libraries(server_sqlite_writer_ownership_regression PRIVATE subathon_sqlite_writer Qt6::Test)
    subathon_target_options(server_sqlite_writer_ownership_regression)
    foreach(case IN ITEMS
        ownsRawBackedSqlAtAdmission
        ownsRawBackedTextBindingAtAdmission
        ownsRawBackedBlobBindingAtAdmission
    )
        add_test(NAME sqlite-writer-regression.${case}
            COMMAND server_sqlite_writer_ownership_regression ${case})
        set_tests_properties(sqlite-writer-regression.${case} PROPERTIES
            LABELS "sqlite-writer;regression;qt;server"
            TIMEOUT 15)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_RECEIPT_TESTS)
    add_executable(server_sqlite_receipts_environment sqlite_receipts_environment.cpp)
    target_link_libraries(server_sqlite_receipts_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    target_compile_definitions(server_sqlite_receipts_environment PRIVATE
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_receipts_environment)
    add_test(NAME scaffold.sqlite_receipts_environment COMMAND server_sqlite_receipts_environment)
    set_tests_properties(scaffold.sqlite_receipts_environment PROPERTIES
        LABELS "scaffold;qt;receipt-environment" TIMEOUT 30)

    add_executable(server_sqlite_receipts_acceptance sqlite_receipts_tests.cpp)
    target_link_libraries(server_sqlite_receipts_acceptance PRIVATE subathon_sqlite_receipts Qt6::Test)
    target_compile_definitions(server_sqlite_receipts_acceptance PRIVATE
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_receipts_acceptance)
    # Reviewed receipt cases preserve their baseline names and separate processes.
    foreach(case IN ITEMS
        codecsMatchFullRangeLiteralVectors
        codecRejectsMalformedBuffers
        manifestPreservesRetentionPrefixAndMatchesLiteralReceiptStep
        intakeCreatesRetriesConflictsAndPreservesOriginal
        allFactsParticipateInConflictWithUnknownsPreserved
        identityComponentsAndOpaqueBytesScopeReceipts
        optionalFactsAndIntegerExtremaRoundTripAfterReopen
        arrivalReferencesResolveWithoutCrosslinkEquality
        allocationCrossesSignedBoundaryAndExhaustsWithoutWrap
        failedCreateAndAppendRollBackAllocationAndRows
        adoptionCommitsRetentionAndReceiptBeforeServiceReturns
        uncommittedAndIncompleteTransactionsCannotPublish
        duplicatePrimitiveCreateIsSqlUniquenessFailure
        inputPreflightPriorityAndBoundsPreserveEarlierWrites
        selectedMalformedDataAndTargetsRequireKnownRollback
        allocatorAndAttemptHistoryCorruptionIsRefused
        findHistoryLimitDoesNotTruncateOrLimitAppend
        cumulativeBudgetFailureRollsBackStagedReceipt
        localSecondBeginPreservesFirstTransaction
        foreignStaleTerminalAndOwnedLifetimesAreSafe
        lostCommitAckReportsUnknownWithPhysicalReceipt
        lostCleanupAckOutranksLocalStorageFailure
    )
        add_test(NAME sqlite-receipt.${case} COMMAND server_sqlite_receipts_acceptance ${case})
        set_tests_properties(sqlite-receipt.${case} PROPERTIES
            LABELS "sqlite-receipt;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS)
    add_executable(server_sqlite_contributions_environment sqlite_contributions_environment.cpp)
    target_link_libraries(server_sqlite_contributions_environment PRIVATE subathon_sqlite_writer Qt6::Test)
    target_compile_definitions(server_sqlite_contributions_environment PRIVATE
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_contributions_environment)
    add_test(NAME scaffold.sqlite_contributions_environment COMMAND server_sqlite_contributions_environment)
    set_tests_properties(scaffold.sqlite_contributions_environment PROPERTIES
        LABELS "scaffold;qt;contribution-environment" TIMEOUT 30)

    add_executable(server_sqlite_contributions_acceptance sqlite_contributions_tests.cpp)
    target_link_libraries(server_sqlite_contributions_acceptance PRIVATE subathon_sqlite_contributions Qt6::Test)
    target_compile_definitions(server_sqlite_contributions_acceptance PRIVATE
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json")
    subathon_target_options(server_sqlite_contributions_acceptance)
    # Reviewed contribution cases retain their baseline names and separate processes.
    foreach(case IN ITEMS
        codecsPreserveZeroAndFullUnsignedRange
        codecRejectsMalformedValues
        manifestPreservesReceiptPrefixAndMatchesLiteralStep
        supportConfirmationAndDuplicatePreserveOriginals
        pendingConfirmationLinksToNewZeroContribution
        verifiedIdentityAndOpaqueProofBytesRemainExact
        independentStateAndCapturedConditionsRoundTrip
        committedReceiptProjectionAndTargetsAreRequired
        allocatorCrossesSignedBoundaryAndExhausts
        observationAssociationsRejectRetargetAndMissingRows
        primitiveUniquenessAndAmbiguityAreExplicit
        createAppendLinkAndUpdateFailuresRollBackAtomically
        adoptedSessionCommitsEarlierWorkBeforeObserveReturns
        ownNewAnchorReadsAndIncompleteCommitAreHonest
        inputPreflightPriorityAndExactOptions
        byteBoundsAndSelectedTargetLimitsAreExact
        unresolvedCollectionLimitNeverTruncatesOrLimitsWrites
        selectedMalformedRowsAndGroupsRequireRollback
        allocatorTypeAndOptionalZeroCannotBeConflated
        cumulativeResultBudgetRollsBackProvedWrites
        localSecondBeginAndBusyPreserveTransactionTruth
        foreignStaleTerminalAndOwnedResultsRemainSafe
        lostCommitAckReportsUnknownWithPhysicalContribution
        lostCleanupAckOutranksLocalFailure
    )
        add_test(NAME sqlite-contribution.${case} COMMAND server_sqlite_contributions_acceptance ${case})
        set_tests_properties(sqlite-contribution.${case} PROPERTIES
            LABELS "sqlite-contribution;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()
