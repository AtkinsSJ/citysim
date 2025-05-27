#pragma once

enum InterpolationType {
    Interpolate_Linear,

    Interpolate_Sine,
    Interpolate_SineIn,
    Interpolate_SineOut,

    InterpolationTypeCount
};

// t is a value from 0 to 1, representing how far through the interpolation you are
f32 interpolate(f32 start, f32 end, f32 t, InterpolationType interpolation);
