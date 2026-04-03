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

    String vertex_source;
    String fragment_source;
    // FIXME: Redo this method
    data_string.split_in_two('$', &vertex_source, &fragment_source);

    return adopt_own(*new Shader(move(data_string), vertex_source, fragment_source));
}

Shader::Shader(String data, String vertex_source, String fragment_source)
    : vertexShader(move(vertex_source))
    , fragmentShader(move(fragment_source))
    , m_data(move(data))
{
}

void Shader::unload(AssetMetadata&)
{
    // FIXME: Maybe remove the shader here instead of in the Renderer's before-asset-unload callback?
    asset_manager().deallocate(m_data);
}
