#by HaiyangXu @2014

project(GraphCut)
cmake_minimum_required(VERSION 2.8)

FIND_PACKAGE(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/maxflow
	)

SET(SOURCE_FILES 
	maxflow/graph.h
	maxflow/graph.cpp
	maxflow/block.h
	maxflow/maxflow.cpp
	main.cpp
)

ADD_EXECUTABLE(GraphCut  ${SOURCE_FILES} )
TARGET_LINK_LIBRARIES(GraphCut  ${OpenCV_LIBS})
