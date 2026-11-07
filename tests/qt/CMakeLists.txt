set(CMAKE_AUTOMOC ON)
if(SUBATHON_BUILD_SQLITE_REFERENCE_CAPTURE_TESTS)
    add_executable(server_sqlite_reference_capture_acceptance sqlite_reference_capture_tests.cpp)
    target_link_libraries(server_sqlite_reference_capture_acceptance PRIVATE
        subathon_sqlite_reference_capture Qt6::Test)
    target_compile_definitions(server_sqlite_reference_capture_acceptance PRIVATE
        SUBATHON_REFERENCE_CAPTURE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_reference_capture_vectors.json"
        SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_sqlite_reference_capture_acceptance)
    # Enabled SQL baseline is intentionally red until separately reviewed implementation.
    foreach(case IN ITEMS
        manifestMatchesFrozenPrefixAndStepSeven
        absentLookupIgnoresUnindexedAndOffScopeData
        insertAndSameSessionReadAreProvisionalUntilCommit
        exactDuplicateReusesImmutableFirstMeaning
        existingConflictPrecedesProposedTargetResolution
        receiptAliasConflictsBeforeCandidateResolution
        bothSelectedIndexesValidateBeforeConflict
        candidateMissingRootsRemainExplicit
        healthyDifferentMeaningIsSourceMismatch
        unindexedPlanCanonicalStructurePrecedesCapturedComparison
        unindexedPlanValuationStructurePrecedesBinding
        unindexedImportedOpaqueRefsStayHealthyMismatches
        unindexedGenericChildrenDistinguishMismatchFromCorruption
        indexedSelectedPlanOwnedBudgetUsesSemanticDtoBytes
        indexedChildMetadataRejectsMalformedScalarsBeforePayload
        unindexedActivityOriginsValidateAgainstActualReceipts
        selectedCorruptionRollsBackWholeUnit
        structurallyValidDocumentsNeedCompleteMatchingClosure
        corruptOriginalOutranksReplacementConflict
        presentIntrinsicMalformedLinkIsNotMissingCandidate
        zeroAndMaxIdsAndNulKeysRemainDistinct
        currentDimensionsRetriesAndLaterPlanHistoryAreUnselected
        pureInputPriorityPrecedesStaleCapability
        pureRefusalPreservesEarlierCallerWork
        validForeignAndExpiredTicketsPreserveWriterErrors
        activeTerminalTicketAndFacadeDestructionAreHonest
        selectedDocumentsAtBoundAndOneUnder
        completeIndexProjectionPrecedesSemanticMismatch
        selectedFieldCeilingIsCheckedBeforeOwnership
        explicitRollbackRemovesPlanExecutionIndexTogether
        hostCleansCompetingStagingAfterConflict
        cumulativeSqlBudgetsRequireActualWholeRollback
        missingSqlTableMapsOnlyAfterKnownRollback
        lostCommitAcknowledgementNeverPublishesBinding
        lostCleanupAcknowledgementOutranksLocalCorruption
        postCommitBodyFailureRetainsRowsWithoutNormalResult
        queuedCompetingCandidatesPublishOnlyFirstCommit
        returnedCaptureOwnsAllReferencesBeyondFacadeAndWork
    )
        add_test(NAME sqlite-reference-capture.${case} COMMAND server_sqlite_reference_capture_acceptance ${case})
        set_tests_properties(sqlite-reference-capture.${case} PROPERTIES
            LABELS "sqlite-reference-capture;acceptance;qt;server" TIMEOUT 30)
    endforeach()
    add_executable(server_reference_capture_values_acceptance reference_capture_values_qt.cpp)
    target_link_libraries(server_reference_capture_values_acceptance PRIVATE
        subathon_reference_capture_values Qt6::Test)
    subathon_target_options(server_reference_capture_values_acceptance)
    # Reviewed codec cases run as ordinary acceptance; SQL remains a separate red baseline.
    foreach(case IN ITEMS
        configurationEncodeLiteral
        configurationDecodeLiteral
        arrivalEncodeLiteral
        arrivalDecodeLiteral
        executionEncodeLiteral
        executionDecodeLiteral
        initialEncodeLiteral
        initialDecodeLiteral
        allLimitsValidateBeforeInput
        encodedEnvelopeBoundsIncludeFormat
        borrowedInputBoundsPrecedeSemanticChecks
        decoderOuterBoundsPrecedeFormatAndFraming
        decoderFramingRejectsEveryTruncation
        nullDocumentsAndPayloadsAreNotEmpty
        payloadBoundariesRetainOpaqueBytes
        knownUnknownFactsRemainStructuralData
        invalidTagsPrecedeReferencesAndValues
        everyRequiredReferenceIsChecked
        configurationNumericAndTimerBoundaries
        arrivalTimesPreserveSignedFullRange
        executionUnsignedDomainsAndInstallationBinding
        initialRequiresRevisionOneAndPositiveReceipt
        framedSemanticFailuresUseExactPriority
        nestedConfigurationIsValidatedWithinArrival
        completeStructuralInputsUseEveryScalar
        decodedFieldsRespectReferenceAndPayloadBounds
        exactOpaqueReferencesAndOwnedResults
        unsupportedFormatPrecedesDocumentSemantics
    )
        add_test(NAME reference-capture-values.${case} COMMAND server_reference_capture_values_acceptance ${case})
        set_tests_properties(reference-capture-values.${case} PROPERTIES
            LABELS "reference-capture-values;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()
if(SUBATHON_BUILD_APPLICATION_SERVICE_TESTS)

    add_executable(server_application_service_acceptance application_service_tests.cpp)
    target_link_libraries(server_application_service_acceptance PRIVATE subathon_original_application_service Qt6::Test)
    target_compile_definitions(server_application_service_acceptance PRIVATE
        SUBATHON_SERVICE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/application_service_vectors.json"
        SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
        SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
        SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
        SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
        SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
        SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
        SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json")
    subathon_target_options(server_application_service_acceptance)
    # Reviewed service baseline; keep every case registered and run it in ordinary CI.
    foreach(case IN ITEMS
        commandEncodingMatchesFrozenFrames
        commandDecodingMatchesFrozenMembers
        codecBoundsUseActualOwnedDTOs
        codecMalformedAndPurePriority
        codecOwnsBinaryBuffers
        admissionPreflightBeforeQueue
        foreignAdmissionAndMissingCallbacks
        arithmeticCommitsCompleteFrozenRecord
        exactPreviewAndCurrentPlanRefuse
        currentEligibilityKeepsIndependentConditions
        currentDimensionsAreCopiedAtExecution
        accessBeforeProtectedLookup
        historicalCompleteCommandAndDisclosure
        globalClaimsBeforeCurrentWork
        allActiveHoldsGateNewKeys
        numericOverflowCommitsOnlyDurableHold
        unresolvedPolicyAndClockAreHonest
        capturedActionUnavailableHolds
        closedDetectionRetryCannotClaimHeld
        lifecyclePauseAndDeadlineDoNotCredit
        portBindingsAndIntermediateBounds
        issuerCollisionsNeverRegenerate
        referenceViewRetainsSelectedKeyLimit
        retainedStateBudgetPrecedesPolicy
        historicalBeforePolicyMatchesAudit
        historicalJobMeaningIsNotOnlyAnEcho
        explicitUnresolvedPolicyRecordsKnownHold
        retainedCommandBoundIsNotMalformedData
        negativeCounterRequiresCorrectionPurpose
        retainedClockFinalUtcMatchesObservation
        referenceViewPureAndCallbackLifetime
        referenceViewForeignReadAndUnderlyingBudget
        recordSemanticFormatsAreChecked
        resultBoundsCountOwnedPayloadOnly
        actualKnownRollbackAtLateJobWrite
        commitAckLossHasNoNormalResponse
        rollbackAckLossOutranksLocalFailure
        postCommitBodyTailCannotPublish
        wrongWorkValueCannotPublish
        completionFailureDoesNotUndoDurability
        fifoAcceptanceAwardsOnce
        queuedAndReturnedBytesStayOwned
        freshServiceReopenReturnsOriginal
    )
        add_test(NAME application-service.${case} COMMAND server_application_service_acceptance ${case})
        set_tests_properties(application-service.${case} PROPERTIES
            LABELS "application-service;acceptance;qt;server" TIMEOUT 30)
    endforeach()
endif()
if(SUBATHON_BUILD_SQLITE_HOLD_TESTS)

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
    # Reviewed hold cases retain separate processes.
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

if(SUBATHON_BUILD_SQLITE_MIGRATION_TESTS)

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

if(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
    add_executable(translation_catalog_probe translation_catalog_probe.cpp)
    target_link_libraries(translation_catalog_probe PRIVATE Qt6::Core)
    subathon_target_options(translation_catalog_probe)
    # Register every authored method without importing tests or invoking feature stubs.
    file(STRINGS "${PROJECT_SOURCE_DIR}/tests/localization/case_inventory.txt"
        translation_catalog_cases)
    foreach(case IN LISTS translation_catalog_cases)
        if(NOT case MATCHES "^test_translation_catalog\\.[A-Za-z]+\\.test_[a-z_]+$")
            message(FATAL_ERROR "Invalid translation catalog case inventory entry")
        endif()
        add_test(NAME translation-catalog.${case}
            COMMAND "${CMAKE_COMMAND}" -E env
                "SUBATHON_TRANSLATION_LRELEASE=$<TARGET_FILE:Qt6::lrelease>"
                "SUBATHON_TRANSLATION_PROBE=$<TARGET_FILE:translation_catalog_probe>"
                "${Python3_EXECUTABLE}" -m unittest "${case}" -v)
        set_tests_properties(translation-catalog.${case} PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/localization"
            LABELS "translation-catalog;acceptance" TIMEOUT 60)
    endforeach()
endif()
