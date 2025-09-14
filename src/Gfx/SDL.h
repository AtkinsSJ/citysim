/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL_surface.h>
#include <Util/Forward.h>

SDL_Surface* create_surface_from_file_data(Blob bytes, String file_name);
