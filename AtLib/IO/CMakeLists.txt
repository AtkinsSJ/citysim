add_library(IO STATIC
    BinaryFile.cpp
    BinaryFileReader.cpp
    BinaryFileWriter.cpp
    DirectoryIterator.cpp
    DirectoryWatcher.cpp
    File.cpp
    LineReader.cpp
    Paths.cpp
    WriteBuffer.cpp
)
target_include_directories(IO PUBLIC "../")

find_package(SDL2 REQUIRED)

target_include_directories(IO PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(IO PRIVATE ${SDL2_LIBRARIES})
