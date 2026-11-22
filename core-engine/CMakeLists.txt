cmake_minimum_required(VERSION 3.10)
project(MedRouteEngine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ENGINE_SOURCES
    src/Graph.cpp
    src/PriorityQueue.cpp
    src/LinkedList.cpp
    src/Ambulance.cpp
    src/Hospital.cpp
    src/Emergency.cpp
    src/Dijkstra.cpp
    src/HopCountPlanner.cpp
    src/SeverityTable.cpp
    src/Dispatcher.cpp
)

# The demo program: run it to see the scenarios.
add_executable(medroute_engine src/main.cpp ${ENGINE_SOURCES})
target_include_directories(medroute_engine PRIVATE src)

# The automated tests: run it to check every module.
add_executable(medroute_tests tests/test_engine.cpp ${ENGINE_SOURCES})
target_include_directories(medroute_tests PRIVATE src)

# The service program: the engine as a command-line tool for the backend (reads state + one action, prints JSON).
add_executable(medroute_service service/service_main.cpp ${ENGINE_SOURCES})
target_include_directories(medroute_service PRIVATE src)
