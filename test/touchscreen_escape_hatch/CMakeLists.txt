add_executable(TouchscreenEscapeHatchTest
  TouchscreenEscapeHatchTest.cpp
)

target_include_directories(TouchscreenEscapeHatchTest PRIVATE
  ${REPO_ROOT}/src/util
)

target_link_libraries(TouchscreenEscapeHatchTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(TouchscreenEscapeHatchTest)
