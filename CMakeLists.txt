CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(
  selain
  VERSION
    0.12
  DESCRIPTION
    "Modal Web browser."
  HOMEPAGE_URL
    "https://github.com/RauliL/selain"
  LANGUAGES
    C CXX
)

# Additional protocol support.
OPTION(SELAIN_ENABLE_FINGER "Enable finger:// protocol" ON)
OPTION(SELAIN_ENABLE_GEMINI "Enable gemini:// protocol" ON)
OPTION(SELAIN_ENABLE_GOPHER "Enable gopher:// protocol" ON)

# Integrations.
OPTION(
  SELAIN_ENABLE_PASS_INTEGRATION
  "Enable pass password manager integration"
  OFF
)
OPTION(
  SELAIN_ENABLE_BUKU_INTEGRATION
  "Enable Buku bookmark manager integration"
  OFF
)

OPTION(SELAIN_BUILD_TESTS "Build unit tests" ON)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/selain/config.hpp.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/selain/config.hpp"
)
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/selain.desktop.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/selain.desktop"
  @ONLY
)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-4.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkitgtk-6.0 REQUIRED)
PKG_CHECK_MODULES(SQLITE3 sqlite3 REQUIRED)

INCLUDE(FetchContent)
INCLUDE(cmake/CMakeRC.cmake)

SET(SELAIN_FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH
  "Base directory for FetchContent dependencies"
)
SET(FETCHCONTENT_BASE_DIR "${SELAIN_FETCHCONTENT_BASE_DIR}")

SET(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(INJA_INSTALL OFF CACHE BOOL "" FORCE)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FETCHCONTENT_DECLARE(
  inja
  GIT_REPOSITORY
    https://github.com/pantor/inja.git
  GIT_TAG
    v3.5.0
)
FETCHCONTENT_MAKEAVAILABLE(inja)

CMRC_ADD_RESOURCE_LIBRARY(
  selain-resources
  ALIAS
    selain::rc
  NAMESPACE
    selain::rc
  src/document.html
  src/theme.css
  src/webkit/find-input.js
  src/webkit/find-search.js
  src/webkit/hint-mode.js
  src/webkit/scroll-helper.js
)

ADD_LIBRARY(
  selain-utils
  STATIC
  src/userscript/metadata.cpp
  src/utils/string.cpp
  src/utils/url.cpp
)

TARGET_COMPILE_FEATURES(
  selain-utils
  PUBLIC
    cxx_std_20
)

TARGET_INCLUDE_DIRECTORIES(
  selain-utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)

TARGET_COMPILE_OPTIONS(
  selain-utils
  PUBLIC
    ${GTKMM_CFLAGS_OTHER}
    ${WEBKITGTK_CFLAGS_OTHER}
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
)

TARGET_LINK_DIRECTORIES(
  selain-utils
  PUBLIC
    ${GTKMM_LIBRARY_DIRS}
    ${WEBKITGTK_LIBRARY_DIRS}
)

TARGET_LINK_LIBRARIES(
  selain-utils
  PUBLIC
    ${GTKMM_LIBRARIES}
    ${WEBKITGTK_LIBRARIES}
)

SET(
  SELAIN_SOURCES
  src/adblock/easylist.cpp
  src/core/command.cpp
  src/core/history-manager.cpp
  src/core/keyboard.cpp
  src/core/mode.cpp
  src/core/session.cpp
  src/core/setting.cpp
  src/core/theme.cpp
  src/format/gemtext.cpp
  src/main.cpp
  src/ui/command-entry.cpp
  src/ui/list-dialog.cpp
  src/ui/main-window.cpp
  src/ui/status-bar.cpp
  src/ui/tab-completion.cpp
  src/ui/tab-label.cpp
  src/ui/tab.cpp
  src/ui/view-list-dialog.cpp
  src/ui/view.cpp
  src/webkit/adblock.cpp
  src/webkit/context.cpp
  src/webkit/hint-context.cpp
  src/webkit/settings.cpp
  src/webkit/userscript.cpp
)

IF(SELAIN_ENABLE_FINGER)
  LIST(
    APPEND
    SELAIN_SOURCES
    src/protocol/finger/client.cpp
    src/protocol/finger/scheme.cpp
  )
ENDIF()

IF(SELAIN_ENABLE_GEMINI)
  LIST(
    APPEND
    SELAIN_SOURCES
    src/protocol/gemini/client.cpp
    src/protocol/gemini/scheme.cpp
  )
ENDIF()

IF(SELAIN_ENABLE_GOPHER)
  LIST(
    APPEND
    SELAIN_SOURCES
    src/protocol/gopher/client.cpp
    src/protocol/gopher/menu.cpp
    src/protocol/gopher/scheme.cpp
  )
ENDIF()

IF(SELAIN_ENABLE_PASS_INTEGRATION)
  LIST(
    APPEND
    SELAIN_SOURCES
    src/integration/pass.cpp
  )
ENDIF()

IF(SELAIN_ENABLE_BUKU_INTEGRATION)
  LIST(
    APPEND
    SELAIN_SOURCES
    src/integration/buku.cpp
  )
ENDIF()

ADD_EXECUTABLE(selain ${SELAIN_SOURCES})

TARGET_COMPILE_FEATURES(
  selain
  PRIVATE
    cxx_std_20
)

TARGET_INCLUDE_DIRECTORIES(
  selain
  PRIVATE
    ${SQLITE3_INCLUDE_DIRS}
)

TARGET_COMPILE_OPTIONS(
  selain
  PRIVATE
    ${SQLITE3_CFLAGS_OTHER}
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
)

TARGET_LINK_DIRECTORIES(
  selain
  PRIVATE
    ${SQLITE3_LIBRARY_DIRS}
)

TARGET_LINK_LIBRARIES(
  selain
  PRIVATE
    selain-utils
    inja
    selain::rc
    ${SQLITE3_LIBRARIES}
)

INSTALL(
  TARGETS
    selain
  RUNTIME DESTINATION
    bin
)

INSTALL(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/selain.desktop
  DESTINATION
    share/applications
)

ADD_SUBDIRECTORY(icons)

IF(SELAIN_BUILD_TESTS)
  INCLUDE(CTest)
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(tests)
ENDIF()
