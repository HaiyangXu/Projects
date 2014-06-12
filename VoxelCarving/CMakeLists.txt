project(VoxelCarving)
cmake_minimum_required(VERSION 2.8)
#set OpenCV
FIND_PACKAGE(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

ADD_EXECUTABLE(VoxelCarving  VoxelCarving.cpp )

SET(LIBRARY_FILES  	${OpenCV_LIBS}
	 )

TARGET_LINK_LIBRARIES(VoxelCarving ${LIBRARY_FILES} )
