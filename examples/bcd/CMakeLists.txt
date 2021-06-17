find_package(Crashpad REQUIRED)

link_directories(${CRASHPAD_LIBRARY_DIRECTORIES})
add_executable(bcd_example crashpad.cpp crashpad.hpp main.cpp)
target_include_directories(bcd_example PUBLIC ${CRASHPAD_INCLUDE_DIRS})
target_link_libraries(bcd_example PRIVATE ${CRASHPAD_LIBRARIES} bun)
target_sources(bcd_example PRIVATE ${CRASHPAD_OBJECTS})

target_link_libraries(bcd_example PRIVATE bun bcd)
