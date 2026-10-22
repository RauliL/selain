CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(selain C CXX)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-4.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkitgtk-6.0 REQUIRED)

ADD_EXECUTABLE(
  selain
  src/command.cpp
  src/command-entry.cpp
  src/hint-context.cpp
  src/keyboard.cpp
  src/main.cpp
  src/main-window.cpp
  src/mode.cpp
  src/status-bar.cpp
  src/tab.cpp
  src/tab-label.cpp
  src/theme.cpp
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
