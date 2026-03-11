/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Paths.h"
#include <SDL_filesystem.h>

namespace Paths {

String user_data()
{
    static String s_user_data_path = String::from_null_terminated(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
    return s_user_data_path;
}

String user_settings_file()
{
    static String settings_file_name = "settings.cnf"_s;
    return myprintf("{0}{1}"_s, { user_data(), settings_file_name }, true);
}

}
