# CitySim

A city simulation game I've tinkered with on-and-off since 2015.

## Building

The build uses CMake. I don't know a lot about CMake, so these instructions may be lacking.

Set up the CMake build using something like this:

```shell
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -B ./build/debug
```

Then compile like this:
```shell
cd ./build/debug
ninja
```

And the resulting executable will be `./build/debug/CitySim`.
