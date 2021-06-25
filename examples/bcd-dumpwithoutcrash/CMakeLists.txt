find_package(Crashpad REQUIRED)

link_directories(${CRASHPAD_LIBRARY_DIRECTORIES})
add_executable(bcd_dump_without_crash crashpad.cpp crashpad.hpp main.cpp)
target_include_directories(bcd_dump_without_crash PUBLIC ${CRASHPAD_INCLUDE_DIRS})
target_link_libraries(bcd_dump_without_crash PRIVATE ${CRASHPAD_LIBRARIES} bun)
target_sources(bcd_dump_without_crash PRIVATE ${CRASHPAD_OBJECTS})

target_link_libraries(bcd_dump_without_crash PRIVATE bun bcd)
