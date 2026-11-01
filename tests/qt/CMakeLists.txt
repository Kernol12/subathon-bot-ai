set(CMAKE_AUTOMOC ON)
if(SUBATHON_BUILD_QT_PROBE)
    add_executable(qt_environment_probe qt_environment_probe.cpp)
    target_link_libraries(qt_environment_probe PRIVATE Qt6::Core Qt6::Test)
    subathon_target_options(qt_environment_probe)
    add_test(NAME scaffold.qt_event_loop COMMAND qt_environment_probe)
    set_tests_properties(scaffold.qt_event_loop PROPERTIES LABELS "scaffold;qt")
endif()

if(SUBATHON_BUILD_SQLITE_WRITER_TESTS)
    add_executable(server_sqlite_writer_acceptance sqlite_writer_tests.cpp)
    target_link_libraries(server_sqlite_writer_acceptance PRIVATE subathon_sqlite_writer Qt6::Test)
    subathon_target_options(server_sqlite_writer_acceptance)
    # Separate processes retain every intended failure and isolate temporary databases.
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
            LABELS "sqlite-writer;acceptance;draft;qt;server"
            TIMEOUT 15)
    endforeach()
endif()
