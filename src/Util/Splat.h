/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Basic.h>
#include <Util/MemoryArena.h>
#include <Util/Vector.h>

//
// A "Splat" is basically a circle with a wobbly circumference.
// As such, it produces roundish blob shapes.
// (I wanted to call it a Blob but that's already taken...)
// You can query the radius at any given angle with radius_at_angle() and it'll lerp
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
    // `resolution` is how many radius values are generated. eg, if it's 36, we get 1 value for every 10 degrees.
    static Splat create_random(s32 centre_x, s32 centre_y, float min_radius, float max_radius, s32 resolution, Random*, s32 smoothness = 4, MemoryArena* = &temp_arena());

    float radius_at_angle(float degrees) const;
    Rect2I bounding_box() const;
    bool contains(s32 x, s32 y) const;

private:
    Splat(V2I centre, float min_radius, float max_radius, Array<float> radius, float degrees_per_index)
        : m_centre(centre)
        , m_min_radius(min_radius)
        , m_max_radius(max_radius)
        , m_radius(move(radius))
        , m_degrees_per_index(degrees_per_index)
    {
    }

    V2I m_centre;
    float m_min_radius;
    float m_max_radius;
    Array<float> m_radius;
    float m_degrees_per_index;
};
