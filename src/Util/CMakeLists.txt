add_library(Util STATIC
    Alignment.cpp
    BitArray.cpp
    Interpolate.cpp
    Locale.cpp
    Log.cpp
    Maths.cpp
    Matrix4.cpp
    Memory.cpp
    MemoryArena.cpp
    Padding.cpp
    Random.cpp
    Rectangle.cpp
    Splat.cpp
    String.cpp
    StringBuilder.cpp
    StringTable.cpp
    Time.cpp
    Unicode.cpp
    Vector.cpp
)
target_include_directories(Util PUBLIC ..)
target_compile_definitions(Util PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
