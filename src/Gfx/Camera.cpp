/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Gfx/Camera.h>
#include <Util/Maths.h>
#include <Util/Rectangle.h>

Camera::Camera(V2 size, f32 size_ratio, f32 near_clipping_plane, f32 far_clipping_plane, V2 position)
    : m_position(position)
    , m_size(size * size_ratio)
    , m_size_ratio(size_ratio)
    , m_near_clipping_plane(near_clipping_plane)
    , m_far_clipping_plane(far_clipping_plane)
{
    update_projection_matrix();
}

Camera::Camera() = default;

void Camera::set_position(V2 new_position)
{
    m_position = new_position;
}

void Camera::move_by(V2 delta)
{
    m_position += delta;
}

void Camera::snap_to_rectangle(Rect2 const bounds)
{
    V2 cameraSize = m_size / m_zoom;
    if (bounds.w < cameraSize.x) {
        // City smaller than camera, so centre on it
        m_position.x = bounds.w * 0.5f;
    } else {
        f32 minX = (cameraSize.x * 0.5f);
        m_position.x = clamp(m_position.x, minX, bounds.right() - minX);
    }

    if (bounds.h < m_size.y / m_zoom) {
        // City smaller than camera, so centre on it
        m_position.y = bounds.h * 0.5f;
    } else {
        f32 minY = (cameraSize.y * 0.5f);
        m_position.y = clamp(m_position.y, minY, bounds.bottom() - minY);
    }
}

void Camera::set_size(V2 new_size)
{
    m_size = new_size * m_size_ratio;
}

void Camera::set_zoom(f32 new_zoom)
{
    m_zoom = snap_zoom(new_zoom);
}

void Camera::zoom_by(f32 delta)
{
    m_zoom = snap_zoom(m_zoom + delta);
}

f32 Camera::snap_zoom(f32 zoom)
{
    return clamp(round_f32(10 * (zoom)) * 0.1f, 0.1f, 10.0f);
}

void Camera::update_mouse_position(V2 input_mouse_position)
{
    m_mouse_position = unproject(input_mouse_position);
}

void Camera::update_projection_matrix()
{
    f32 half_camera_width = m_size.x * 0.5f / m_zoom;
    f32 half_camera_height = m_size.y * 0.5f / m_zoom;
    m_projection_matrix = orthographicMatrix4(
        m_position.x - half_camera_width, m_position.x + half_camera_width,
        m_position.y - half_camera_height, m_position.y + half_camera_height,
        m_near_clipping_plane, m_far_clipping_plane);

    m_inverse_projection_matrix = inverse(&m_projection_matrix);
}

V2 Camera::unproject(V2 screen_position) const
{
    // Convert into scene space
    V4 unprojected = m_inverse_projection_matrix * v4(screen_position.x, screen_position.y, 0.0f, 1.0f);
    return unprojected.xy;
}
