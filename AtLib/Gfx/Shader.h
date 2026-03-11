/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Util/Blob.h>
#include <Util/String.h>

class Shader final : public Asset {
    ASSET_SUBCLASS_METHODS(Shader);

public:
    static NonnullOwnPtr<Shader> make_placeholder();
    static ErrorOr<NonnullOwnPtr<Shader>> load(AssetMetadata&, Blob data);
    virtual ~Shader() override = default;

    virtual void unload(AssetMetadata& metadata) override;

    s8 rendererShaderID;

    String vertexShader;
    String fragmentShader;

private:
    Shader(Blob data, String vertex_source, String fragment_source);
    Blob m_data;
};
