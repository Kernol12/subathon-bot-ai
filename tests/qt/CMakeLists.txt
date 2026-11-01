set(CMAKE_AUTOMOC ON)
if(SUBATHON_BUILD_QT_PROBE)
    add_executable(qt_environment_probe qt_environment_probe.cpp)
    target_link_libraries(qt_environment_probe PRIVATE Qt6::Core Qt6::Test)
    subathon_target_options(qt_environment_probe)
    add_test(NAME scaffold.qt_event_loop COMMAND qt_environment_probe)
    set_tests_properties(scaffold.qt_event_loop PROPERTIES LABELS "scaffold;qt")
endif()

if(SUBATHON_BUILD_SQLITE_WORK_TESTS)
    add_executable(server_sqlite_work_acceptance sqlite_writer_work_tests.cpp)
    target_link_libraries(server_sqlite_work_acceptance PRIVATE
        subathon_sqlite_writer subathon_receipt_intake Qt6::Test)
    subathon_target_options(server_sqlite_work_acceptance)
    # Intentionally red until STORAGE-010; each case runs in an isolated process.
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
            LABELS "sqlite-work;acceptance;draft;qt;server"
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
