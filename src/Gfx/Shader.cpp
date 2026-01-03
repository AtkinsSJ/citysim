/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Shader.h"
#include <Assets/AssetManager.h>

Shader& Shader::get(StringView name)
{
    return getAsset(AssetType::Shader, name.deprecated_to_string()).shader;
}
