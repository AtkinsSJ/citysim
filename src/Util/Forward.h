/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Assert.h"
#include <utility>

using std::move, std::forward;

struct BitArray;
struct BitArrayIterator;
class Blob;
struct DateTime;
struct Matrix4;
struct MemoryArena;
struct Padding;
struct Random;
struct Rect2;
struct Rect2I;
struct String;
struct V2;
struct V2I;
struct V3;
struct V4;

template<typename T>
class Optional;
