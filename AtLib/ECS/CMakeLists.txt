add_library(ECS STATIC
    Component.cpp
    Entity.cpp
    System.cpp
    World.cpp
)
target_include_directories(ECS PUBLIC "../")

target_compile_definitions(ECS PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
