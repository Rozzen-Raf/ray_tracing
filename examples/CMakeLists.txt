project(example C)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} geometry)
target_link_libraries(${PROJECT_NAME} thread_pool)
