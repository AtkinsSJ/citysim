add_library(Gfx STATIC
    AssetLoader.cpp
    BitmapFont.cpp
    Camera.cpp
    Colour.cpp
    Cursor.cpp
    Ninepatch.cpp
    OpenGL.cpp
    Palette.cpp
    RenderBuffer.cpp
    Renderer.cpp
    Shader.cpp
    Sprite.cpp
    TextDocument.cpp
    Texture.cpp
)
target_include_directories(Gfx PUBLIC "../")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
target_include_directories(Gfx PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

target_link_libraries(Gfx PRIVATE Assets ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LIBRARIES} SDL2_image::SDL2_image)

target_compile_definitions(Gfx PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
