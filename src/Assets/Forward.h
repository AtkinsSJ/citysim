/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

namespace Assets {

using AssetType = u32;

class Asset;
class AssetMetadata;
struct AssetManager;
class AssetManagerListener;

template<typename T>
class TypedAssetRef;

class ContainerAsset;
class DeprecatedAsset;
class Texts;

}

using Assets::Asset;
using Assets::AssetManager;
using Assets::AssetManagerListener;
using Assets::AssetMetadata;
using Assets::AssetType;
using Assets::ContainerAsset;
using Assets::DeprecatedAsset;
using Assets::Texts;
using Assets::TypedAssetRef;
