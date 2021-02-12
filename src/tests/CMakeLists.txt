Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.0-preview3)

FetchContent_MakeAvailable(Catch2)
if(NOT catch2_POPULATED)
  message(FATAL_ERROR "Did NOT get catch2")
else()
  message(STATUS "Catch2 source dir: ${catch2_SOURCE_DIR}")
endif()


set(CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

set(APP_SRCS
  ../Cleaner.h
  ../Cleaner.cpp
)

set(TEST_SRCS
  TestCleaner.cpp
)

set(TEST_AUX
  cpp.hint
)

add_executable(tests ${TEST_SRCS} ${APP_SRCS})
target_link_libraries(tests PUBLIC Qt5::Core PRIVATE Catch2::Catch2WithMain)

if(TEST_AUX)
    add_custom_command(
            TARGET tests POST_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${TEST_AUX}
                    ${CMAKE_CURRENT_BINARY_DIR}/)
endif()


catch_discover_tests(tests)
