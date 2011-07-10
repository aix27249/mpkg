#
# MPKG packaging system
#
#
cmake_minimum_required(VERSION 2.4)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MPKG)

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


set(SYSCONFDIR "/etc")

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED)

ADD_DEFINITIONS(${CXXFLAGS})
#ADD_DEFINITIONS(-O0 -DDEBUG -g)
LINK_LIBRARIES(mpkg)
# TARGET: building executable (installpkg-ng)
ADD_EXECUTABLE(mpkg-core installpkg-ng2)
SET_TARGET_PROPERTIES(mpkg-core PROPERTIES OUTPUT_NAME "mpkg")
ADD_EXECUTABLE(mpkg-validate validator)
#ADD_EXECUTABLE(mpkg-create-deltas createdeltas)
#ADD_EXECUTABLE(testing testing)
#ADD_EXECUTABLE(mpkgeditor pkgeditor)
ADD_EXECUTABLE(mpkg-setmeta setmeta)
ADD_EXECUTABLE(mpkg-setprofile setprofile)
ADD_EXECUTABLE(mpkg-saveprofile saveprofile)
ADD_EXECUTABLE(mpkg-listprofiles listprofiles)
ADD_EXECUTABLE(mpkg-deleteprofile deleteprofile)
ADD_EXECUTABLE(mpkg-depgraph depgraph)
#ADD_EXECUTABLE(mpkg-collectfileindex collectfileindex)
ADD_EXECUTABLE(mpkg-checkdeps checkdeps)
ADD_EXECUTABLE(mpkg-altlist altlist)
ADD_EXECUTABLE(mpkg-altshow altshow)
ADD_EXECUTABLE(mpkg-quicklist quicklist)
ADD_EXECUTABLE(mpkg-checklibdeps checklibdeps)
ADD_EXECUTABLE(mpkg-maintainer maintainer)
ADD_EXECUTABLE(mpkg-spkg2abuild spkg2abuild)
ADD_EXECUTABLE(mpkg-deporder deporder)
#ADD_EXECUTABLE(mpkg-deppath deppath)
#ADD_EXECUTABLE(mpkg-depclean depclean)
ADD_EXECUTABLE(mpkg-fixdeps fixdeps)
ADD_EXECUTABLE(mpkg-checkconfigs checkconfig)
ADD_EXECUTABLE(mpkg-transactions transactions)
ADD_EXECUTABLE(mpkg-rollback rollback)
ADD_EXECUTABLE(mpkg-conflicts conflicts)
#ADD_EXECUTABLE(mpkg-converttosql converttosql)
# Installation
FILE(GLOB aliases "${CMAKE_CURRENT_SOURCE_DIR}/aliases/mpkg-*")
FILE(GLOB update_all_caches "${CMAKE_CURRENT_SOURCE_DIR}/update-all-caches")
IF(X86_64)
	INSTALL(TARGETS mpkg-core mpkg-validate mpkg-setmeta mpkg-setprofile mpkg-saveprofile mpkg-listprofiles mpkg-deleteprofile mpkg-depgraph mpkg-checkdeps mpkg-altlist mpkg-altshow mpkg-quicklist mpkg-checklibdeps mpkg-maintainer mpkg-spkg2abuild mpkg-deporder mpkg-fixdeps mpkg-transactions mpkg-rollback mpkg-conflicts mpkg-checkconfigs
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib64
		ARCHIVE DESTINATION lib64
	)
ELSE(X86_64)
	INSTALL(TARGETS mpkg-core mpkg-validate mpkg-setmeta mpkg-setprofile mpkg-saveprofile mpkg-listprofiles mpkg-deleteprofile mpkg-depgraph mpkg-checkdeps mpkg-altlist mpkg-altshow mpkg-quicklist mpkg-checklibdeps mpkg-maintainer mpkg-spkg2abuild mpkg-deporder mpkg-fixdeps mpkg-transactions mpkg-rollback mpkg-conflicts mpkg-checkconfigs
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
ENDIF(X86_64)
INSTALL(FILES ${aliases} DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE )
INSTALL(FILES ${update_all_caches} DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE )
#INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mpkg-importsrc" DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE )
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mpkglddcheck.sh" DESTINATION libexec PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mkpkg_generator.sh" DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/amerge" DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mkabuild" DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mpkgsrc" DESTINATION bin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/makepkg" DESTINATION /sbin PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mpkg" DESTINATION ${SYSCONFDIR}/bash_completion.d)
