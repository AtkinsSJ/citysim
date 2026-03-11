/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Locale.h"
#include <Util/String.h>

Optional<Locale> locale_from_string(String locale)
{
    if (locale == "en"_s)
        return Locale::En;
    if (locale == "es"_s)
        return Locale::Es;
    if (locale == "pl"_s)
        return Locale::Pl;
    return {};
}

String to_string(Locale locale)
{
    switch (locale) {
    case Locale::En:
        return "en"_s;
    case Locale::Es:
        return "es"_s;
    case Locale::Pl:
        return "pl"_s;
    case Locale::COUNT:
        break;
    }
    VERIFY_NOT_REACHED();
}
