CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(selain C CXX)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-3.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkit2gtk-4.0 REQUIRED)

ADD_EXECUTABLE(
  selain
  src/command.cpp
  src/command-entry.cpp
  src/keyboard.cpp
  src/main.cpp
  src/main-window.cpp
  src/mode.cpp
  src/status-bar.cpp
  src/tab.cpp
  src/tab-label.cpp
  src/theme.cpp
  src/utils.cpp
)

TARGET_COMPILE_FEATURES(
  selain
  PRIVATE
    cxx_std_11
)

TARGET_INCLUDE_DIRECTORIES(
  selain
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(
  selain
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
