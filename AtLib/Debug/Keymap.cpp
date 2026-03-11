/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Keymap.h"

ErrorOr<NonnullOwnPtr<Keymap>> Keymap::load(AssetMetadata& metadata, Blob file_data)
{
    // Separate reader just for the command count.
    auto command_count = 0u;
    {
        LineReader reader { metadata.shortName, file_data };
        while (reader.load_next_line()) {
            if (auto shortcut_string = reader.next_token(); shortcut_string.has_value())
                command_count++;
        }
    }

    // `data` contains both the file data, and the CommandShortcut array, in that order.
    // FIXME: I should really figure out a nicer way of doing asset memory management, but that's a future task.
    Blob data = Assets::assets_allocate(file_data.size() + (command_count * sizeof(CommandShortcut)));
    memcpy(data.writable_data(), file_data.data(), file_data.size());
    Array<CommandShortcut> shortcuts { static_cast<s32>(command_count), 0, reinterpret_cast<CommandShortcut*>(data.writable_data() + file_data.size()) };

    // Now we create a reader on the stored copy of the file data, so that we can point StringViews into it.
    LineReader reader { metadata.shortName, data.sub_blob(0, file_data.size()) };
    while (reader.load_next_line()) {
        auto shortcut_string = reader.next_token();
        if (!shortcut_string.has_value())
            continue;
        auto command = reader.remainder_of_current_line();

        if (auto shortcut = KeyboardShortcut::from_string(shortcut_string.value()); shortcut.has_value()) {
            shortcuts.append({
                .shortcut = shortcut.release_value(),
                .command = command,
            });
        } else {
            return reader.make_error_message("Unrecognised key in keyboard shortcut sequence '{0}'"_s, { shortcut_string.value() });
        }
    }

    return adopt_own(*new Keymap(move(data), move(shortcuts)));
}

Keymap::Keymap(Blob data, Array<CommandShortcut> shortcuts)
    : m_data(move(data))
    , m_shortcuts(move(shortcuts))
{
}

void Keymap::unload(AssetMetadata&)
{
    Assets::assets_deallocate(m_data);
    m_shortcuts = {};
}
