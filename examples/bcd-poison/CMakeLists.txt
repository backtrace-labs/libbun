find_package(Crashpad REQUIRED)

link_directories(${CRASHPAD_LIBRARY_DIRECTORIES})
add_executable(bcd_example_poison crashpad.cpp crashpad.hpp main.cpp)
target_include_directories(bcd_example_poison PUBLIC ${CRASHPAD_INCLUDE_DIRS})
target_link_libraries(bcd_example_poison PRIVATE ${CRASHPAD_LIBRARIES} bun)
target_sources(bcd_example_poison PRIVATE ${CRASHPAD_OBJECTS})

target_link_libraries(bcd_example_poison PRIVATE bun bcd poison_malloc)
