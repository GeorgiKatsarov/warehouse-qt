cmake_minimum_required(VERSION 3.21)
project(WarehouseQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Sql Widgets Test)
enable_testing()

add_library(warehouse_core STATIC core/Environment.h core/Environment.cpp
    core/Product.h core/Product.cpp core/User.h core/User.cpp core/Discount.h core/Discount.cpp)
target_include_directories(warehouse_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core)
target_link_libraries(warehouse_core PUBLIC Qt6::Core Qt6::Sql)

add_executable(warehouse_app app/main.cpp)
target_link_libraries(warehouse_app PRIVATE warehouse_core Qt6::Widgets)

add_executable(warehouse_tests tests/tst_environment.cpp)
target_link_libraries(warehouse_tests PRIVATE warehouse_core Qt6::Test)
add_test(NAME warehouse_tests COMMAND warehouse_tests)

foreach(suite inventory discounts permissions)
    add_executable(tst_${suite} tests/tst_${suite}.cpp)
    target_link_libraries(tst_${suite} PRIVATE warehouse_core Qt6::Test)
    add_test(NAME tst_${suite} COMMAND tst_${suite})
endforeach()

foreach(target warehouse_core warehouse_app warehouse_tests tst_inventory tst_discounts tst_permissions)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()
