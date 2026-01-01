/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

struct BitArray;
struct BitArrayIterator;
class Blob;
struct DateTime;
struct Matrix4;
class MemoryArena;
struct Padding;
class Random;
class Rect2;
class Rect2I;
class String;
class StringBase;
class StringTable;
class StringView;
class TokenReader;
struct V2;
struct V2I;
struct V3;
struct V4;

// FIXME: Actual error type
using Error = String;

template<typename T>
class ErrorOr;

template<typename T>
class Indexed;

template<typename T>
class Optional;

template<typename T>
class OwnPtr;

template<typename T>
class NonnullOwnPtr;

template<typename T>
class Ref;
