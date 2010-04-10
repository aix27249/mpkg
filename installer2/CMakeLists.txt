cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT(installer)
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 OFF)
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 ON)
	ADD_DEFINITIONS(-DX86_64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

ADD_DEFINITIONS( -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED )


ADD_DEFINITIONS(${CXXFLAGS})

ADD_EXECUTABLE(select_language language)
TARGET_LINK_LIBRARIES(select_language nwidgets)

ADD_EXECUTABLE(select_partitioning partitioning)
TARGET_LINK_LIBRARIES(select_partitioning nwidgets mpkg-parted)

ADD_EXECUTABLE(select_swap swap)
TARGET_LINK_LIBRARIES(select_swap nwidgets mpkg-parted)

ADD_EXECUTABLE(select_root root)
TARGET_LINK_LIBRARIES(select_root nwidgets mpkg-parted)

ADD_EXECUTABLE(select_formatting formatting)
TARGET_LINK_LIBRARIES(select_formatting nwidgets mpkg-parted)

ADD_EXECUTABLE(select_mountpoints mountpoints)
TARGET_LINK_LIBRARIES(select_mountpoints nwidgets mpkg-parted)

ADD_EXECUTABLE(select_pkgsource pkgsource helper_functions)
TARGET_LINK_LIBRARIES(select_pkgsource nwidgets mpkg-parted)

ADD_EXECUTABLE(select_pkgset pkgset helper_functions)
TARGET_LINK_LIBRARIES(select_pkgset mpkg mpkg-parted)

ADD_EXECUTABLE(update_cache update_cache helper_functions)
TARGET_LINK_LIBRARIES(update_cache nwidgets mpkg-parted)

ADD_EXECUTABLE(select_bootloader bootloader)
TARGET_LINK_LIBRARIES(select_bootloader nwidgets)

ADD_EXECUTABLE(select_bootloader_target bootloader_target)
TARGET_LINK_LIBRARIES(select_bootloader_target nwidgets mpkg-parted)

ADD_EXECUTABLE(select_framebuffer framebuffer)
TARGET_LINK_LIBRARIES(select_framebuffer nwidgets)

ADD_EXECUTABLE(select_alternatives alternatives)
TARGET_LINK_LIBRARIES(select_alternatives nwidgets)

ADD_EXECUTABLE(select_nvidia_drivers nvidia_drivers)
TARGET_LINK_LIBRARIES(select_nvidia_drivers nwidgets)

ADD_EXECUTABLE(run_setup setup helper_functions)
TARGET_LINK_LIBRARIES(run_setup mpkg mpkg-parted)


