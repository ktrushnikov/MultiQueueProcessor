PROJECT(MultiQueueProcessor)
CMAKE_MINIMUM_REQUIRED ( VERSION 2.8 )
FIND_PACKAGE ( Threads REQUIRED )
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ADD_EXECUTABLE(MultiQueueProcessor "main.cpp" "MultiQueueProcessor.h")
TARGET_LINK_LIBRARIES(MultiQueueProcessor ${CMAKE_THREAD_LIBS_INIT})