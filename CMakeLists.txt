CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(selain C CXX)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-4.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkitgtk-6.0 REQUIRED)

INCLUDE(FetchContent)
INCLUDE(cmake/CMakeRC.cmake)

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
  src/hint-mode.js
  src/theme.css
)

ADD_EXECUTABLE(
  selain
  src/command-entry.cpp
  src/command.cpp
  src/format/gemtext.cpp
  src/gemini-client.cpp
  src/gemini-scheme.cpp
  src/gopher-client.cpp
  src/gopher-menu.cpp
  src/gopher-scheme.cpp
  src/hint-context.cpp
  src/keyboard.cpp
  src/main-window.cpp
  src/main.cpp
  src/mode.cpp
  src/setting.cpp
  src/status-bar.cpp
  src/tab-completion.cpp
  src/tab-label.cpp
  src/tab.cpp
  src/theme.cpp
  src/utils.cpp
  src/view.cpp
  src/web-context.cpp
  src/web-settings.cpp
)

TARGET_COMPILE_FEATURES(
  selain
  PRIVATE
    cxx_std_20
)

TARGET_INCLUDE_DIRECTORIES(
  selain
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)

TARGET_COMPILE_OPTIONS(
  selain
  PRIVATE
    ${GTKMM_CFLAGS_OTHER}
    ${WEBKITGTK_CFLAGS_OTHER}
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
)

TARGET_LINK_DIRECTORIES(
  selain
  PRIVATE
    ${GTKMM_LIBRARY_DIRS}
    ${WEBKITGTK_LIBRARY_DIRS}
)

TARGET_LINK_LIBRARIES(
  selain
  PRIVATE
    ${GTKMM_LIBRARIES}
    ${WEBKITGTK_LIBRARIES}
    inja
    selain::rc
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
