add_library(subathon_core STATIC src/time_credit.cpp)
target_include_directories(subathon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_core)

if(TARGET Boost::headers)
    add_library(subathon_effective_permissions STATIC EXCLUDE_FROM_ALL
        src/effective_permissions.cpp
        src/delegated_permissions.cpp)
    target_include_directories(subathon_effective_permissions PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_effective_permissions PRIVATE Boost::headers)
    subathon_target_options(subathon_effective_permissions)

    add_library(subathon_exact_numeric STATIC EXCLUDE_FROM_ALL src/exact_decimal.cpp)
    target_include_directories(subathon_exact_numeric PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_exact_numeric PUBLIC Boost::headers)
    subathon_target_options(subathon_exact_numeric)

    add_library(subathon_milestone_progression STATIC EXCLUDE_FROM_ALL src/milestone_progression.cpp)
    target_include_directories(subathon_milestone_progression PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_milestone_progression PUBLIC subathon_exact_numeric)
    subathon_target_options(subathon_milestone_progression)

    add_library(subathon_application_plan STATIC EXCLUDE_FROM_ALL src/application_plan.cpp)
    target_include_directories(subathon_application_plan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_application_plan PUBLIC Boost::headers)
    subathon_target_options(subathon_application_plan)

    add_library(subathon_rate_snapshots STATIC EXCLUDE_FROM_ALL src/rate_snapshot.cpp)
    target_include_directories(subathon_rate_snapshots PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_rate_snapshots PUBLIC subathon_exact_numeric)
    subathon_target_options(subathon_rate_snapshots)

    add_library(subathon_reference_rule STATIC EXCLUDE_FROM_ALL src/reference_rule.cpp)
    target_include_directories(subathon_reference_rule PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_reference_rule PUBLIC Boost::headers)
    subathon_target_options(subathon_reference_rule)
endif()

add_library(subathon_timer_lifecycle STATIC EXCLUDE_FROM_ALL src/timer_lifecycle.cpp)
target_include_directories(subathon_timer_lifecycle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_timer_lifecycle)

add_library(subathon_time_debt STATIC EXCLUDE_FROM_ALL src/time_debt.cpp)
target_include_directories(subathon_time_debt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_time_debt PUBLIC subathon_core)
subathon_target_options(subathon_time_debt)

add_library(subathon_contribution STATIC EXCLUDE_FROM_ALL src/contribution.cpp)
target_include_directories(subathon_contribution PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_contribution)

add_library(subathon_pause_policy STATIC EXCLUDE_FROM_ALL src/pause_policy.cpp)
target_include_directories(subathon_pause_policy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_pause_policy)

add_library(subathon_timer_editorial_commands STATIC EXCLUDE_FROM_ALL src/timer_editorial_commands.cpp)
target_include_directories(subathon_timer_editorial_commands PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_timer_editorial_commands PUBLIC subathon_timer_lifecycle subathon_time_debt)
subathon_target_options(subathon_timer_editorial_commands)
