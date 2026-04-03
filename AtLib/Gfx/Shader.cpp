/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Shader.h"
#include <Assets/AssetManager.h>

NonnullOwnPtr<Shader> Shader::make_placeholder()
{
    return adopt_own(*new Shader({}, ""_s, ""_s));
}

ErrorOr<NonnullOwnPtr<Shader>> Shader::load(AssetMetadata&, Blob data)
{
    auto data_string = asset_manager().allocate_string({ reinterpret_cast<char const*>(data.data()), data.size() });

    auto parts = data_string.split_in_two('$');
    if (!parts.has_value())
        return Error { "Shader file must contain a vertex and fragment shader, separated by `$`. No `$` was found."_s };
    auto [vertex_source, fragment_source] = parts.value();
    return adopt_own(*new Shader(move(data_string), vertex_source, fragment_source));
}

Shader::Shader(String data, StringView vertex_source, StringView fragment_source)
    : m_data(move(data))
    , m_vertex_source(move(vertex_source))
    , m_fragment_source(move(fragment_source))
{
}

void Shader::unload(AssetMetadata&)
{
    // FIXME: Maybe remove the shader here instead of in the Renderer's before-asset-unload callback?
    asset_manager().deallocate(m_data);
}
