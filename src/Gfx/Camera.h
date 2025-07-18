/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Matrix4.h>
#include <Util/Vector.h>

class Camera {
public:
    Camera(V2 size, f32 size_ratio, f32 near_clipping_plane, f32 far_clipping_plane, V2 position = v2(0, 0));
    Camera();

    Matrix4 const& projection_matrix() const { return m_projection_matrix; }

    V2 position() const { return m_position; }
    void set_position(V2);
    void move_by(V2);
    void snap_to_rectangle(Rect2);

    V2 size() const { return m_size; }
    void set_size(V2);

    f32 zoom() const { return m_zoom; }
    void set_zoom(f32);
    void zoom_by(f32);

    V2 mouse_position() const { return m_mouse_position; }
    void update_mouse_position(V2 input_mouse_position);

    // FIXME: Should be private, and just automatically updated when needed. (Probably with a dirty flag.)
    void update_projection_matrix();

    V2 unproject(V2 screen_position) const;

private:
    static f32 snap_zoom(f32);

    V2 m_position {};       // Centre of camera, in camera units
    V2 m_size {};           // Size of camera, in camera units
    f32 m_size_ratio { 1 }; // Size of window is multiplied by this to produce the camera's size
    f32 m_zoom { 1 };       // 1 = normal, 2 = things appear twice their size, etc.
    Matrix4 m_projection_matrix {};
    Matrix4 m_inverse_projection_matrix {};

    // NB: We don't use depth anywhere any more, but these do get used in generating the
    // projection matrix. - Sam, 26/07/2019
    f32 m_near_clipping_plane { 0 };
    f32 m_far_clipping_plane { f32Max };

    V2 m_mouse_position {};
};
