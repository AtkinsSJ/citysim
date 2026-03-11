add_library(Assets STATIC
    AssetManager.cpp
    AssetMetadata.cpp
    AssetRef.cpp
    BuiltinAssetLoader.cpp
    Texts.cpp
)
target_include_directories(Assets PUBLIC "../")

target_compile_definitions(Assets PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
