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
    auto asset_data = Assets::assets_allocate(data.size());
    memcpy(asset_data.writable_data(), data.data(), data.size());

    String vertex_source;
    String fragment_source;
    String::from_blob(asset_data).value().split_in_two('$', &vertex_source, &fragment_source);

    return adopt_own(*new Shader(move(asset_data), vertex_source, fragment_source));
}

Shader::Shader(Blob data, String vertex_source, String fragment_source)
    : vertexShader(move(vertex_source))
    , fragmentShader(move(fragment_source))
    , m_data(move(data))
{
}

void Shader::unload(AssetMetadata&)
{
    // FIXME: Maybe remove the shader here instead of in the Renderer's before-asset-unload callback?
    Assets::assets_deallocate(m_data);
}
