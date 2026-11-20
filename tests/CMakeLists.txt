SET(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
SET(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)

FETCHCONTENT_DECLARE(
  Catch2
  GIT_REPOSITORY
    https://github.com/catchorg/Catch2.git
  GIT_TAG
    v3.16.1
)
FETCHCONTENT_MAKEAVAILABLE(Catch2)

LIST(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

ADD_EXECUTABLE(
  selain_tests
  utils_test.cpp
)

TARGET_LINK_LIBRARIES(
  selain_tests
  PRIVATE
    selain_utils
    Catch2::Catch2WithMain
)

TARGET_COMPILE_OPTIONS(
  selain_tests
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
)

INCLUDE(Catch)
CATCH_DISCOVER_TESTS(selain_tests)
