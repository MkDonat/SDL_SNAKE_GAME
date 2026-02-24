add_library(
    engine_libs
    world.cpp
    collectable.cpp
)

# Link to the actual SDL3 library.
target_link_libraries(engine_libs PRIVATE SDL3::SDL3)