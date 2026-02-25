/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Palette.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetRef.h>
#include <Assets/ContainerAsset.h>

Palette::Palette(Blob data, Type type, Array<Colour> colours)
    : m_data(data)
    , m_type(type)
    , m_colours(move(colours))
{
}

ErrorOr<NonnullOwnPtr<Asset>> Palette::load_defs(AssetMetadata& metadata, Blob file_data)
{
    LineReader reader { metadata.shortName, file_data };

    struct PaletteData {
        StringView name;
        Type type { Type::Fixed };
        size_t size { 0 };
        ChunkedArray<Colour> fixed_colors;
        Optional<Colour> from_color;
        Optional<Colour> to_color;
    };
    ChunkedArray<PaletteData> palettes;
    initChunkedArray(&palettes, &temp_arena(), 128);
    while (reader.load_next_line()) {
        auto command_token = reader.next_token();
        if (!command_token.has_value())
            continue;
        auto command = command_token.release_value();

        if (command.starts_with(':')) {
            command = command.substring(1);

            if (command == "Palette"_s) {
                if (auto palette_name = reader.next_token(); palette_name.has_value()) {
                    PaletteData data;
                    data.name = palette_name.release_value();
                    initChunkedArray(&data.fixed_colors, &temp_arena(), 128);
                    palettes.append(move(data));
                } else {
                    return reader.make_error_message("Missing name for Palette"_s);
                }
            } else {
                return reader.make_error_message("Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!"_s, { command });
            }
        } else {
            if (palettes.is_empty())
                return reader.make_error_message("Unexpected command '{0}' before the start of a :Palette"_s, { command });

            auto& palette = palettes.get(palettes.count - 1);

            if (command == "type"_s) {
                auto type = reader.next_token();
                if (!type.has_value()) {
                    return reader.make_error_message("Missing palette type"_s);
                }

                if (type == "fixed"_s) {
                    palette.type = Palette::Type::Fixed;
                } else if (type == "gradient"_s) {
                    palette.type = Palette::Type::Gradient;
                } else {
                    return reader.make_error_message("Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, { type.value() });
                }
            } else if (command == "size"_s) {
                if (auto size = reader.read_int<s32>(); size.has_value()) {
                    palette.size = size.release_value();
                } else {
                    return "Failed to read size"_s;
                }
            } else if (command == "color"_s) {
                if (auto color = Colour::read(reader); color.has_value()) {
                    if (palette.type == Palette::Type::Fixed) {
                        s32 colorIndex = palette.fixed_colors.count;
                        // FIXME: Is `size` actually necessary/useful?
                        if (colorIndex >= palette.size)
                            return reader.make_error_message("Too many 'color' definitions! 'size' must be large enough."_s);
                        palette.fixed_colors.append(color.release_value());
                    } else {
                        return reader.make_error_message("'color' is only a valid command for fixed palettes."_s);
                    }
                }
            } else if (command == "from"_s) {
                if (auto from = Colour::read(reader); from.has_value()) {
                    if (palette.type == Palette::Type::Gradient) {
                        palette.from_color = from.release_value();
                    } else {
                        return reader.make_error_message("'from' is only a valid command for gradient palettes."_s);
                    }
                }
            } else if (command == "to"_s) {
                if (auto to = Colour::read(reader); to.has_value()) {
                    if (palette.type == Palette::Type::Gradient) {
                        palette.to_color = to.release_value();
                    } else {
                        return reader.make_error_message("'to' is only a valid command for gradient palettes."_s);
                    }
                }
            } else {
                return reader.make_error_message("Unrecognised command '{0}'"_s, { command });
            }
        }
    }

    // Load all the palettes, now that we know their properties are all set.
    auto children_data = Assets::assets_allocate(palettes.count * sizeof(GenericAssetRef));
    auto children = makeArray(palettes.count, reinterpret_cast<GenericAssetRef*>(children_data.writable_data()));

    for (auto it = palettes.iterate(); it.hasNext(); it.next()) {
        auto& palette = it.get();

        switch (palette.type) {
        case Type::Gradient: {
            auto data = Assets::assets_allocate(palette.size * sizeof(Colour));
            auto colours_array = makeArray<Colour>(palette.size, reinterpret_cast<Colour*>(data.writable_data()), palette.size);

            float ratio = 1.0f / static_cast<float>(palette.size);
            for (auto i = 0u; i < palette.size; i++) {
                colours_array[i] = lerp(palette.from_color.value(), palette.to_color.value(), i * ratio);
            }

            auto& palette_metadata = *asset_manager().add_asset(asset_type(), palette.name, {});
            palette_metadata.loaded_asset = adopt_own(*new Palette(move(data), palette.type, move(colours_array)));
            palette_metadata.state = AssetMetadata::State::Loaded;
            children.append(palette_metadata.get_ref());
        } break;

        case Type::Fixed: {
            auto data = Assets::assets_allocate(palette.fixed_colors.count * sizeof(Colour));
            auto colours_array = makeArray<Colour>(palette.fixed_colors.count, reinterpret_cast<Colour*>(data.writable_data()), palette.fixed_colors.count);
            for (auto i = 0u; i < colours_array.count; i++)
                colours_array[i] = palette.fixed_colors.get(i);

            auto& palette_metadata = *asset_manager().add_asset(asset_type(), palette.name, {});
            palette_metadata.loaded_asset = adopt_own(*new Palette(move(data), palette.type, move(colours_array)));
            palette_metadata.state = AssetMetadata::State::Loaded;
            children.append(palette_metadata.get_ref());
        } break;
        }
    }

    return { adopt_own(*new ContainerAsset(move(children_data), move(children))) };
}

Colour Palette::colour_at(size_t index) const
{
    return m_colours[index];
}

Colour Palette::first() const
{
    return m_colours.first();
}

Colour Palette::last() const
{
    return m_colours.last();
}

Colour* Palette::raw_colour_data() const
{
    return m_colours.items;
}

size_t Palette::size() const
{
    return m_colours.count;
}

void Palette::unload(AssetMetadata&)
{
    Assets::assets_deallocate(m_data);
    m_colours = {};
}
