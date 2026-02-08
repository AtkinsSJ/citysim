/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

namespace Assets {

enum class AssetType : u8;

class Asset;
class AssetMetadata;
struct AssetManager;
class AssetManagerListener;
class AssetRef;
class DeprecatedAsset;

}

using Assets::Asset;
using Assets::AssetManager;
using Assets::AssetManagerListener;
using Assets::AssetMetadata;
using Assets::AssetRef;
using Assets::AssetType;
using Assets::DeprecatedAsset;
