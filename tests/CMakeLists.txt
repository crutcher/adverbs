project(tests)

# =========================================
# Pull gtest at configure time.
# See: https://github.com/google/googletest/blob/main/googletest/README.md
include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Now simply link against gtest or gtest_main as needed. Eg
# > add_executable(example example.cpp)
# > target_link_libraries(example gtest_main)
# > add_test(NAME example_test COMMAND example)
# =========================================

add_executable(tests_run BasicTest.cpp)
target_link_libraries(tests_run gtest_main)
target_link_libraries(tests_run adverbs_lib)