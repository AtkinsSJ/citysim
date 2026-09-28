/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TerrainDefs.h"
#include <Assets/AssetManager.h>
#include <Debug/Debug.h>
#include <IO/LineReader.h>
#include <Sim/TerrainCatalogue.h>
#include <Util/Lexer.h>

ErrorOr<OwnedRef<TerrainDefs>> TerrainDefs::load(AssetMetadata& metadata, Blob file_data)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, file_data };

    // Pre scan for the number of Terrains, so we can allocate enough space in the asset.
    size_t terrain_count = 0;
    while (reader.load_next_line()) {
        auto command = Lexer { reader.current_line() }.consume_token();
        if (command == ":Terrain"_s)
            terrain_count++;
    }

    auto terrain_ids = asset_manager().allocate_array<String>(terrain_count);

    reader.restart();

    auto& catalogue = TerrainCatalogue::the();

    TerrainDef* def = nullptr;

    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };

        // Commands
        if (lexer.consume_specific(':')) {
            // Define something
            auto command = lexer.consume_token();
            lexer.discard_whitespace();

            if (command == "Terrain"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                if (!name.has_value() || lexer.has_next())
                    return reader.make_error_message("Couldn't parse Terrain. Expected: ':Terrain identifier'"_s);

                Indexed<TerrainDef> slot = catalogue.terrainDefs.append();
                def = &slot.value();

                if (slot.index() > u8Max)
                    return reader.make_error_message("Too many Terrain definitions! The most we support is {0}."_s, { formatInt(u8Max) });

                def->typeID = (u8)slot.index();

                def->name = catalogue.terrainNames.intern(name.value());
                terrain_ids.append(def->name);
                catalogue.terrainDefsByName.set(def->name, def);
                catalogue.terrainNameToType.set(def->name, def->typeID);
            } else {
                reader.warn("Only :Terrain definitions are supported right now."_s);
            }
            continue;
        }

        // Properties!
        auto maybe_property = lexer.consume_token();
        if (!maybe_property.has_value())
            continue;
        auto property_name = maybe_property.release_value();
        lexer.discard_whitespace();

        if (def == nullptr)
            return reader.make_error_message("Found a property before starting a :Terrain!"_s);

        if (property_name == "borders"_s) {
            lexer.discard_whitespace();
            if (lexer.has_next())
                return reader.make_error_message("Couldn't parse borders. Expected: `borders` with nothing after."_s);
            def->borderSpriteNames = asset_manager().arena.allocate_array<String>(80);
        } else if (property_name == "border"_s) {
            auto sprite_name = lexer.consume_token();
            lexer.discard_whitespace();
            if (!sprite_name.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse border. Expected: `border SPRITE_NAME`."_s);
            def->borderSpriteNames.append(asset_manager().assetStrings.intern(sprite_name.release_value()));
        } else if (property_name == "can_build_on"_s) {
            auto can_build_on = lexer.consume_bool();
            lexer.discard_whitespace();
            if (!can_build_on.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse can_build_on. Expected: `can_build_on BOOLEAN`."_s);
            def->canBuildOn = can_build_on.release_value();
        } else if (property_name == "draw_borders_over"_s) {
            auto draw_borders_over = lexer.consume_bool();
            lexer.discard_whitespace();
            if (!draw_borders_over.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse draw_borders_over. Expected: `draw_borders_over BOOLEAN`."_s);
            def->drawBordersOver = draw_borders_over.release_value();
        } else if (property_name == "name"_s) {
            auto name = lexer.consume_token();
            lexer.discard_whitespace();
            if (!name.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse name. Expected: `name NAME`."_s);
            def->textAssetName = asset_manager().assetStrings.intern(name.release_value());
        } else if (property_name == "sprite"_s) {
            auto sprite_name = lexer.consume_token();
            lexer.discard_whitespace();
            if (!sprite_name.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse sprite. Expected: `sprite SPRITE_NAME`."_s);
            def->spriteName = asset_manager().assetStrings.intern(sprite_name.release_value());
        } else {
            reader.warn("Unrecognised property '{0}' inside command ':Terrain'"_s, { property_name });
        }
    }

    return adopt_own(*new TerrainDefs(move(terrain_ids)));
}

TerrainDefs::TerrainDefs(Array<String> terrain_ids)
    : m_terrain_ids(move(terrain_ids))
{
}

void TerrainDefs::unload(AssetMetadata& metadata)
{
    auto& terrain_catalogue = TerrainCatalogue::the();

    for (auto const& terrainName : m_terrain_ids) {
        s32 terrainIndex = findTerrainTypeByName(terrainName);
        if (terrainIndex > 0) {
            terrain_catalogue.terrainDefs.removeIndex(terrainIndex);
            terrain_catalogue.terrainNameToType.remove(terrainName);
        }
    }

    asset_manager().deallocate(m_terrain_ids);
}
