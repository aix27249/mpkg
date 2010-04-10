#
# MOPSLinux packaging system
#
#
cmake_minimum_required(VERSION 2.4)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MPKGPACKAGEBUILDER)

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

ADD_DEFINITIONS(${CXXFLAGS})

# Main part
# c++ sources
SET(mpkg-gui-installer_SRC
	main.cpp
	engine.cpp
	thread.cpp
	progress.cpp
)
# headers for MOC
SET(mpkg-gui-installer_MOC_HDR
	engine.h
	thread.h
	progress.h
)

# UI
SET(mpkg-gui-installer_UI
	engine.ui
	progress.ui
)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )
QT4_WRAP_UI(mpkg-gui-installer_UI_HDR ${mpkg-gui-installer_UI})
QT4_WRAP_CPP(mpkg-gui-installer_MOC_SRC ${mpkg-gui-installer_MOC_HDR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(mpkg-gui-installer ${mpkg-gui-installer_SRC} ${mpkg-gui-installer_MOC_SRC} ${mpkg-gui-installer_UI_HDR})
TARGET_LINK_LIBRARIES(mpkg-gui-installer ${QT_LIBRARIES} mpkg)

# Installation
 
FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.qm")
INSTALL(TARGETS mpkg-gui-installer
		RUNTIME DESTINATION bin
)
INSTALL(FILES ${files} DESTINATION share/mpkg)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mpkg-install-kde4.desktop" DESTINATION share/kde4/services/ServiceMenus)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/xz.xml" DESTINATION share/mime/packages)
