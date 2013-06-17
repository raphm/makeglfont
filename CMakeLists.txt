CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if(APPLE)
option(USE_CLANG "Build with clang" ON) # ON is the default
endif(APPLE)

IF(APPLE)
        SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++" )
        SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++" )
        SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++" )
        SET( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++" )
    
    IF( CMAKE_GENERATOR STREQUAL Xcode )
        SET( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11" )
        SET( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++" )
    ENDIF()

ENDIF(APPLE)

FILE(GLOB inc_files
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

FILE(GLOB src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

PROJECT(makeglfont)
SET(makeglfont_SRC
bin_packer/Rect.cpp
bin_packer/SkylineBinPack.cpp
bin_packer/GuillotineBinPack.cpp
${src_files}
${inc_files}
)

ADD_EXECUTABLE(glfont ${makeglfont_SRC})

include_directories(utf8 bin_packer)

FIND_PATH(FT_INCLUDE_PATH ft2build.h
  /usr/include
  /usr/local/include)
FIND_PACKAGE(Freetype)
if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (glfont ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

set(CMAKE_BUILD_TYPE Debug)
SET (CMAKE_CXX_FLAGS                "-Wall -std=c++11 -stdlib=libc++")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
