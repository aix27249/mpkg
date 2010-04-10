#
# MOPSLinux packaging system
#
#
cmake_minimum_required(VERSION 2.6)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MPKGMANAGER2)

# Enabling building of shared libs
SET(BUILD_SHARED_LIBS ON)
# If you want to build a standalone server app, set SERVERAPP to ON
SET(SERVERAPP OFF)
SET(MAXSTATIC OFF)
# Check 64 bit
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 OFF)
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 ON)
	ADD_DEFINITIONS(-DX86_64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED)
ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

ADD_DEFINITIONS(${CXXFLAGS})
# Main part
# c++ sources
SET(manager2_SRC
	main.cpp
	mainwindow.cpp
	engine.cpp
	commitdlg.cpp
	progresswidget.cpp
	settings.cpp
)
# headers for MOC
SET(manager2_MOC_HDR
	mainwindow.h
	engine.h
	commitdlg.h
	progresswidget.h
	settings.h
)

# UI
SET(manager2_UI
	mainwindow.ui
	commitdlg.ui
	progresswidget.ui
	settings.ui
)
SET(manager2_QRC
	mainwindow.qrc
)
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )
QT4_ADD_RESOURCES(manager2_RCC_SRCS ${manager2_QRC})
QT4_WRAP_UI(manager2_UI_HDR ${manager2_UI})
QT4_WRAP_CPP(manager2_MOC_SRC ${manager2_MOC_HDR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(manager2 ${manager2_SRC} ${manager2_MOC_SRC} ${manager2_UI_HDR} ${manager2_RCC_SRCS})
TARGET_LINK_LIBRARIES(manager2 ${QT_LIBRARIES} mpkg)

# Installation
 
FILE(GLOB appicon128 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/128/mpkg-manager2.png")
FILE(GLOB appicon16 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/16/mpkg-manager2.png")
FILE(GLOB appicon22 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/22/mpkg-manager2.png")
FILE(GLOB appicon24 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/24/mpkg-manager2.png")
FILE(GLOB appicon32 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/32/mpkg-manager2.png")
FILE(GLOB appicon48 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/48/mpkg-manager2.png")
FILE(GLOB appicon72 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/72/mpkg-manager2.png")

FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.qm")
FILE(GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.png")
FILE(GLOB desktop "${CMAKE_CURRENT_SOURCE_DIR}/*.desktop")
INSTALL(TARGETS manager2
		RUNTIME DESTINATION bin
)
INSTALL(FILES ${files} DESTINATION share/mpkg)
INSTALL(FILES ${icons} DESTINATION share/mpkg/icons)
INSTALL(FILES ${desktop} DESTINATION share/applications)

INSTALL(FILES ${appicon128} DESTINATION share/icons/hicolor/128x128/apps/)
INSTALL(FILES ${appicon16} DESTINATION share/icons/hicolor/16x16/apps/)
INSTALL(FILES ${appicon22} DESTINATION share/icons/hicolor/22x22/apps/)
INSTALL(FILES ${appicon24} DESTINATION share/icons/hicolor/24x24/apps/)
INSTALL(FILES ${appicon32} DESTINATION share/icons/hicolor/32x32/apps/)
INSTALL(FILES ${appicon48} DESTINATION share/icons/hicolor/48x48/apps/)
INSTALL(FILES ${appicon72} DESTINATION share/icons/hicolor/72x72/apps/)

