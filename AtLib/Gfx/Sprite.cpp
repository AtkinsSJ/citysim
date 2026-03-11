/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Sprite.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetRef.h>
#include <Assets/ContainerAsset.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Texture.h>

Sprite& Sprite::get(StringView name)
{
    return SpriteGroup::get(name).sprites[0];
}

NonnullOwnPtr<SpriteGroup> SpriteGroup::make_placeholder()
{
    auto sprite_group = adopt_own(*new SpriteGroup);
    sprite_group->data = Assets::assets_allocate(1 * sizeof(Sprite));
    sprite_group->count = 1;
    sprite_group->sprites = (Sprite*)sprite_group->data.writable_data();
    sprite_group->sprites[0].texture = &asset_manager().get_placeholder_asset(Texture::asset_type());
    sprite_group->sprites[0].uv = { 0.0f, 0.0f, 1.0f, 1.0f };

    return sprite_group;
}

void SpriteGroup::unload(AssetMetadata& metadata)
{
    Assets::assets_deallocate(data);
}

Sprite& SpriteRef::get() const
{
    if (asset_manager().asset_generation() > m_asset_generation) {
        auto& group = SpriteGroup::get(m_sprite_group_name);
        m_pointer = &group.sprites[m_sprite_index % group.count];
        m_asset_generation = asset_manager().asset_generation();
    }

    return *m_pointer;
}

static AssetMetadata* add_sprite_group(StringView name, s32 spriteCount)
{
    ASSERT(spriteCount > 0); // Must have a positive number of sprites in a Sprite Group!

    AssetMetadata* metadata = asset_manager().add_asset(SpriteGroup::asset_type(), name, {});
    auto asset = adopt_own(*new SpriteGroup);
    asset->data = Assets::assets_allocate(spriteCount * sizeof(Sprite));
    asset->count = spriteCount;
    asset->sprites = (Sprite*)asset->data.writable_data();

    metadata->loaded_asset = move(asset);
    metadata->state = AssetMetadata::State::Loaded;
    return metadata;
}

static AssetMetadata* add_ninepatch(StringView name, StringView filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
{
    AssetMetadata* texture_metadata = asset_manager().add_asset(Texture::asset_type(), filename);
    texture_metadata->ensure_is_loaded();

    AssetMetadata* metadata = asset_manager().add_asset(Ninepatch::asset_type(), name, {});
    metadata->loaded_asset = adopt_own(*new Ninepatch(*texture_metadata, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3));
    metadata->state = AssetMetadata::State::Loaded;
    return metadata;
}

ErrorOr<NonnullOwnPtr<Asset>> load_sprite_defs(AssetMetadata& metadata, Blob data)
{
    LineReader reader { metadata.shortName, data };

    AssetMetadata* textureAsset = nullptr;
    V2I spriteSize = v2i(0, 0);
    V2I spriteBorder = v2i(0, 0);
    AssetMetadata* current_sprite_group_metadata = nullptr;
    s32 spriteIndex = 0;

    // Count the number of child assets, so we can allocate our spriteNames array
    s32 childAssetCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command.has_value() && command.value().starts_with(':'))
            childAssetCount++;
    }
    auto children_data = Assets::assets_allocate(childAssetCount * sizeof(GenericAssetRef));
    auto children = makeArray(childAssetCount, reinterpret_cast<GenericAssetRef*>(children_data.writable_data()));
    reader.restart();

    // Now, actually read things
    while (reader.load_next_line()) {
        auto maybe_command = reader.next_token();
        if (!maybe_command.has_value())
            continue;
        auto command = maybe_command.release_value();

        if (command.starts_with(':')) // Definitions
        {
            // Define something
            command = command.substring(1).deprecated_to_string();

            textureAsset = nullptr;
            current_sprite_group_metadata = nullptr;

            if (command == "Ninepatch"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto pu0 = reader.read_int<s32>();
                auto pu1 = reader.read_int<s32>();
                auto pu2 = reader.read_int<s32>();
                auto pu3 = reader.read_int<s32>();
                auto pv0 = reader.read_int<s32>();
                auto pv1 = reader.read_int<s32>();
                auto pv2 = reader.read_int<s32>();
                auto pv3 = reader.read_int<s32>();

                if (!all_have_values(name, filename, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3)) {
                    return reader.make_error_message("Couldn't parse Ninepatch. Expected: ':Ninepatch identifier filename.png pu0 pu1 pu2 pu3 pv0 pv1 pv2 pv3'"_s);
                }

                AssetMetadata* ninepatch = add_ninepatch(name.release_value(), filename.release_value(), pu0.release_value(), pu1.release_value(), pu2.release_value(), pu3.release_value(), pv0.release_value(), pv1.release_value(), pv2.release_value(), pv3.release_value());

                children.append(ninepatch->get_ref());
            } else if (command == "Sprite"_s) {
                // @Copypasta from the SpriteGroup branch, and the 'sprite' property
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    return reader.make_error_message("Couldn't parse Sprite. Expected: ':Sprite identifier filename.png SWxSH'"_s);
                }

                spriteSize = spriteSizeIn.release_value();

                AssetMetadata* group = add_sprite_group(name.release_value(), 1);
                auto& group_asset = dynamic_cast<SpriteGroup&>(*group->loaded_asset);

                Sprite* sprite = group_asset.sprites;
                sprite->texture = asset_manager().add_asset(Texture::asset_type(), filename.release_value());
                sprite->uv = { 0, 0, spriteSize.x, spriteSize.y };
                sprite->pixelWidth = spriteSize.x;
                sprite->pixelHeight = spriteSize.y;

                children.append(group->get_ref());
            } else if (command == "SpriteGroup"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    return reader.make_error_message("Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
                }

                textureAsset = asset_manager().add_asset(Texture::asset_type(), filename.release_value());
                spriteSize = spriteSizeIn.release_value();

                s32 spriteCount = reader.count_occurrences_of_property_in_current_command("sprite"_s);
                if (spriteCount < 1) {
                    return reader.make_error_message("SpriteGroup must contain at least 1 sprite!"_s);
                }
                current_sprite_group_metadata = add_sprite_group(name.release_value(), spriteCount);
                spriteIndex = 0;

                children.append(current_sprite_group_metadata->get_ref());
            } else {
                return reader.make_error_message("Unrecognised command: '{0}'"_s, { command });
            }
        } else // Properties!
        {
            if (current_sprite_group_metadata == nullptr)
                return reader.make_error_message("Found a property outside of a :SpriteGroup!"_s);

            if (command == "border"_s) {
                auto borderW = reader.read_int<s32>();
                auto borderH = reader.read_int<s32>();
                if (borderW.has_value() && borderH.has_value()) {
                    spriteBorder = v2i(borderW.release_value(), borderH.release_value());
                } else {
                    return reader.make_error_message("Couldn't parse border. Expected 'border width height'."_s);
                }
            } else if (command == "sprite"_s) {
                auto mx = reader.read_int<s32>();
                auto my = reader.read_int<s32>();

                if (mx.has_value() && my.has_value()) {
                    s32 x = mx.release_value();
                    s32 y = my.release_value();

                    auto& group_asset = dynamic_cast<SpriteGroup&>(*current_sprite_group_metadata->loaded_asset);
                    Sprite* sprite = group_asset.sprites + spriteIndex;
                    sprite->texture = textureAsset;
                    sprite->uv = { spriteBorder.x + x * (spriteSize.x + spriteBorder.x + spriteBorder.x),
                        spriteBorder.y + y * (spriteSize.y + spriteBorder.y + spriteBorder.y),
                        spriteSize.x, spriteSize.y };
                    sprite->pixelWidth = spriteSize.x;
                    sprite->pixelHeight = spriteSize.y;

                    spriteIndex++;
                } else {
                    return reader.make_error_message("Couldn't parse {0}. Expected '{0} x y'."_s, { command });
                }
            } else {
                return reader.make_error_message("Unrecognised command '{0}'"_s, { command });
            }
        }
    }

    // Load all the sprites, now that we know their properties are all set.
    for (auto& child : children) {
        auto& sprite_group_metadata = child.metadata();
        if (sprite_group_metadata.type != SpriteGroup::asset_type())
            continue;

        auto& sprite_group = dynamic_cast<SpriteGroup&>(*sprite_group_metadata.loaded_asset);

        // Convert UVs from pixel space to 0-1 space
        for (s32 i = 0; i < sprite_group.count; i++) {
            Sprite* sprite = sprite_group.sprites + i;
            // FIXME: Should refer to textures some other way!
            AssetMetadata* texture_metadata = sprite->texture;
            texture_metadata->ensure_is_loaded();
            auto& texture = dynamic_cast<Texture&>(*texture_metadata->loaded_asset);
            float textureWidth = texture.surface->w;
            float textureHeight = texture.surface->h;

            sprite->uv = {
                sprite->uv.x() / textureWidth,
                sprite->uv.y() / textureHeight,
                sprite->uv.width() / textureWidth,
                sprite->uv.height() / textureHeight
            };
        }
    }

    return { adopt_own(*new ContainerAsset(move(children_data), move(children))) };
}
