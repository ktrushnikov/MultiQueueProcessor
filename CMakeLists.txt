SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ADD_EXECUTABLE(MultiQueueProcessor "main.cpp" "MultiQueueProcessor.h")
TARGET_LINK_LIBRARIES(MultiQueueProcessor pthread)