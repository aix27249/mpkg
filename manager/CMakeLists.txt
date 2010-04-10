#
# MOPSLinux packaging system
#
#
cmake_minimum_required(VERSION 2.4)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MPKGMANAGER)

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
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED)
# Main part
# c++ sources
SET(manager_SRC
	aboutbox.cpp
	corethread.cpp
	errorbus.cpp
	statusthread.cpp
	db.cpp
	loading.cpp
	main.cpp
	mainwindow.cpp
	preferencesbox.cpp
	tablelabel.cpp
)
# headers for MOC
SET(manager_MOC_HDR
	aboutbox.h
	corethread.h
	db.h
	loading.h
	mainwindow.h
	preferencesbox.h
	tablelabel.h
)

# UI
SET(manager_UI
	aboutbox.ui
	db.ui
	loading.ui
	pkgmanager.ui
	preferencesbox.ui
)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )
QT4_WRAP_UI(manager_UI_HDR ${manager_UI})
QT4_WRAP_CPP(manager_MOC_SRC ${manager_MOC_HDR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(manager ${manager_SRC} ${manager_MOC_SRC} ${manager_UI_HDR})
TARGET_LINK_LIBRARIES(manager ${QT_LIBRARIES} mpkg)

# Installation
 
FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.qm")
FILE(GLOB desktop "${CMAKE_CURRENT_SOURCE_DIR}/*.desktop")
INSTALL(TARGETS manager
		RUNTIME DESTINATION bin
)
INSTALL(FILES ${files} DESTINATION share/mpkg)
INSTALL(FILES ${desktop} DESTINATION share/applications)

