add_executable(case_1 case_1.cc)
target_link_libraries(case_1 verifier_main_helper)

add_executable(case_2 case_2.cc)
target_link_libraries(case_2 verifier_main_helper)

add_executable(verify_from_bytes verify_from_bytes.cc)
target_link_libraries(verify_from_bytes verifier_main_helper)
