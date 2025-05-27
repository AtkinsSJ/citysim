clang++ ./src/main.cpp -o build/citysim \
	-std=c++20 --debug \
	-Wall -Werror -Wno-unknown-pragmas -Wno-unqualified-std-cast-call \
	-DBUILD_DEBUG=1 \
	-I ./include \
	-lSDL2 -lSDL2main -lSDL2_image -lGLEW -lGLU -lGL
