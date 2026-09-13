add_library(Assets STATIC
    AssetManager.cpp
    AssetMetadata.cpp
    AssetRef.cpp
    BuiltinAssetLoader.cpp
    Texts.cpp
)
target_include_directories(Assets PUBLIC "../")
