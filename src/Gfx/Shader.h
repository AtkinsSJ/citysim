/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/String.h>

struct Shader {
    static Shader& get(StringView name);

    s8 rendererShaderID;

    String vertexShader;
    String fragmentShader;
};
