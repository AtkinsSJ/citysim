#pragma once

//
// A "Splat" is basically a circle with a wobbly circumference.
// As such, it produces roundish blob shapes.
// (I wanted to call it a Blob but that's already taken...)
// You can query the radius at any given angle with getRadiusAtAngle() and it'll lerp
// the value based on the two nearest samples. The resolution given determines how many
// radius samples there are. For the couple of places I've used it, 36 works fine, but
// if you want to generate large shapes in greater detail. The smoothness value is passed
// to generate1DNoise() so represents how many averaging passes are done on the random
// noise.
//
// Standard usage is to get the bounding box, and then call contains() for each tile
// to see if it's inside the splat.
//
// - Sam, 16/09/2019
//
// TODO: This SO answer: https://stackoverflow.com/a/54829058/1178345 explains a way of
// making the waviness more interesting, by using a combination of periodic functions
// that are randomly offset. (Instead of using our generate1DNoise() function). Actually,
// maybe we should just use this system in generate1DNoise()!
// Hmmm, actually, that system would mean not needing to store a radius array, as you could
// just compute the exact radius at any given angle. Interesting!
//
struct Splat {
    V2I centre;

    f32 minRadius;
    f32 maxRadius;
    f32 radiusRange;
    Array<f32> radius;
    f32 degreesToIndex;
};

// `resolution` is how many radius values are generated. eg, if it's 36, we get 1 value for every 10 degrees.
Splat createRandomSplat(s32 centreX, s32 centreY, f32 minRadius, f32 maxRadius, s32 resolution, Random* random, s32 smoothness = 4, MemoryArena* memoryArena = &temp_arena());

f32 getRadiusAtAngle(Splat* splat, f32 degrees);
Rect2I getBoundingBox(Splat* splat);
bool contains(Splat* splat, s32 x, s32 y);
