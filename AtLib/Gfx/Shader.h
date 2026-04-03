/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Gfx/Forward.h>
#include <Util/String.h>

class Shader final : public Asset {
    ASSET_SUBCLASS_METHODS(Shader);

public:
    static NonnullOwnPtr<Shader> make_placeholder();
    static ErrorOr<NonnullOwnPtr<Shader>> load(AssetMetadata&, Blob data);
    virtual ~Shader() override = default;

    virtual void unload(AssetMetadata& metadata) override;

    StringView vertex_source() const { return m_vertex_source; }
    StringView fragment_source() const { return m_fragment_source; }

    s8 renderer_shader_id() const { return m_renderer_shader_id; }
    void set_renderer_shader_id(Badge<GL::Renderer>, s8 shader_id)
    {
        m_renderer_shader_id = shader_id;
    }

private:
    Shader(String data, StringView vertex_source, StringView fragment_source);

    String m_data;
    StringView m_vertex_source;
    StringView m_fragment_source;

    s8 m_renderer_shader_id { -1 };
};
