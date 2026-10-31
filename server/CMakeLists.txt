# Framework-free repository-port baseline; no Qt application/writer integration.
add_library(subathon_receipt_intake STATIC src/receipt_intake.cpp)
target_include_directories(subathon_receipt_intake PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_receipt_intake PUBLIC subathon_core)
subathon_target_options(subathon_receipt_intake)
