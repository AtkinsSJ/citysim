/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BMFont.h"
#include <Assets/Asset.h>
#include <Assets/AssetManager.h>
#include <Util/Memory.h>

void loadBMFont(Blob data, Asset* asset)
{
    smm pos = 0;
    BMFontHeader* header = (BMFontHeader*)(data.data() + pos);
    pos += sizeof(BMFontHeader);

    // Check it's a valid BMF
    if (header->tag[0] != BMFontTag[0]
        || header->tag[1] != BMFontTag[1]
        || header->tag[2] != BMFontTag[2]) {
        logError("Not a valid BMFont file: {0}"_s, { asset->fullName });
    } else if (header->version != 3) {
        logError("BMFont file version is unsupported: {0}, wanted {1} and got {2}"_s,
            { asset->fullName, formatInt(BMFontSupportedVersion), formatInt(header->version) });
    } else {
        BMFontBlockHeader* blockHeader = nullptr;
        BMFontBlock_Common* common = nullptr;
        BMFont_Char* chars = nullptr;
        u32 charCount = 0;
        void const* pages = nullptr;

        blockHeader = (BMFontBlockHeader*)(data.data() + pos);
        pos += sizeof(BMFontBlockHeader);

        while (pos < data.size()) {
            switch (blockHeader->type) {
            case BMF_Block_Info: {
                // Ignored
            } break;

            case BMF_Block_Common: {
                common = (BMFontBlock_Common*)(data.data() + pos);
            } break;

            case BMF_Block_Pages: {
                pages = data.data() + pos;
            } break;

            case BMF_Block_Chars: {
                chars = (BMFont_Char*)(data.data() + pos);
                charCount = blockHeader->size / sizeof(BMFont_Char);
            } break;

            case BMF_Block_KerningPairs: {
                // TODO: Kerning!
            } break;
            }

            pos += blockHeader->size;

            blockHeader = (BMFontBlockHeader*)(data.data() + pos);
            pos += sizeof(BMFontBlockHeader);
        }

        if (!(common && chars && charCount && pages)) {
            // Something didn't load correctly!
            logError("BMFont file '{0}' seems to be lacking crucial data and could not be loaded!"_s, { asset->fullName });
        } else if (common->pageCount != 1) {
            logError("BMFont file '{0}' defines a font with {1} texture pages, but we require only 1."_s, { asset->fullName, formatInt(common->pageCount) });
        } else {
            BitmapFont* font = &asset->bitmapFont;
            font->m_line_height = common->lineHeight;
            font->m_base_y = common->base;
            font->m_glyph_count = 0;

            font->m_glyph_capacity = ceil_s32(charCount * 2.0f);
            smm glyphEntryMemorySize = font->m_glyph_capacity * sizeof(BitmapFontGlyphEntry);
            asset->data = assetsAllocate(&asset_manager(), glyphEntryMemorySize);
            font->m_glyph_entries = (BitmapFontGlyphEntry*)(asset->data.data());

            String textureName = String::from_null_terminated((char*)pages);
            font->m_texture = addTexture(textureName, false);
            ensureAssetIsLoaded(font->m_texture);

            float textureWidth = (float)font->m_texture->texture.surface->w;
            float textureHeight = (float)font->m_texture->texture.surface->h;

            for (u32 charIndex = 0;
                charIndex < charCount;
                charIndex++) {
                BMFont_Char* src = chars + charIndex;

                font->add_glyph(BitmapFontGlyph {
                    .codepoint = static_cast<unichar>(src->id),
                    .width = src->w,
                    .height = src->h,
                    .xOffset = src->xOffset,
                    .yOffset = src->yOffset,
                    .xAdvance = src->xAdvance,
                    .uv = {
                        src->x / textureWidth,
                        src->y / textureHeight,
                        src->w / textureWidth,
                        src->h / textureHeight },
                });
            }
        }
    }
}
