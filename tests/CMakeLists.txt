add_executable(flow-tests
        main.cpp tests.cpp)

find_package(GTest
        REQUIRED)

target_link_libraries(flow-tests
        PRIVATE GTest::GTest flow)

add_test(
        NAME flow
        COMMAND flow-tests)