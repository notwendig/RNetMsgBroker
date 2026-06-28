cmake_minimum_required(VERSION 3.16)

project(RNetMsgBroker VERSION 0.2.13 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core LinguistTools)

set(TS_FILES RNetMsgBroker_de_DE.ts)

add_library(RNetMsgBroker SHARED
  RNetMsgBroker_global.h
  rnetmsgbroker.cpp
  rnetmsgbroker.h
  ${TS_FILES}
)

target_include_directories(RNetMsgBroker
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(RNetMsgBroker PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(RNetMsgBroker
  PRIVATE
    RNETMSGBROKER_LIBRARY
    RNETMSGBROKER_VERSION_STRING="${PROJECT_VERSION}"
)

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

add_executable(RNetMsgBrokerTest
  testapp/main.cpp
)

target_link_libraries(RNetMsgBrokerTest PRIVATE RNetMsgBroker Qt${QT_VERSION_MAJOR}::Core)

add_custom_command(TARGET RNetMsgBrokerTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/R-Net.json
          $<TARGET_FILE_DIR:RNetMsgBrokerTest>/R-Net.json
  COMMAND ${CMAKE_COMMAND} -E make_directory
          $<TARGET_FILE_DIR:RNetMsgBrokerTest>/testdata
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/testdata/candump_sample.txt
          $<TARGET_FILE_DIR:RNetMsgBrokerTest>/testdata/candump_sample.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/testdata/candump_open_rnet_sample.txt
          $<TARGET_FILE_DIR:RNetMsgBrokerTest>/testdata/candump_open_rnet_sample.txt
)


# Optional install target for packaging or downstream CMake projects.
include(GNUInstallDirs)
install(TARGETS RNetMsgBroker
  EXPORT RNetMsgBrokerTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES rnetmsgbroker.h RNetMsgBroker_global.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RNetMsgBroker
)
install(FILES R-Net.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/RNetMsgBroker
)
