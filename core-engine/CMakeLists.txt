cmake_minimum_required(VERSION 3.10)
project(MedRouteEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(medroute_engine
    src/main.cpp
    src/Graph.cpp
    src/PriorityQueue.cpp
    src/LinkedList.cpp
    src/Ambulance.cpp
    src/Hospital.cpp
    src/Emergency.cpp
)

target_include_directories(medroute_engine PRIVATE src)
