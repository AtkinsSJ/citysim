/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "bmfont.h"
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
                // Ignored for now
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
            font->lineHeight = common->lineHeight;
            font->baseY = common->base;
            font->glyphCount = 0;

            font->glyphCapacity = ceil_s32(charCount * 2.0f);
            smm glyphEntryMemorySize = font->glyphCapacity * sizeof(BitmapFontGlyphEntry);
            asset->data = assetsAllocate(&asset_manager(), glyphEntryMemorySize);
            font->glyphEntries = (BitmapFontGlyphEntry*)(asset->data.data());

            String textureName = makeString((char*)pages);
            font->texture = addTexture(textureName, false);
            ensureAssetIsLoaded(font->texture);

            f32 textureWidth = (f32)font->texture->texture.surface->w;
            f32 textureHeight = (f32)font->texture->texture.surface->h;

            for (u32 charIndex = 0;
                charIndex < charCount;
                charIndex++) {
                BMFont_Char* src = chars + charIndex;

                BitmapFontGlyph* dest = addGlyph(font, src->id);

                dest->codepoint = src->id;
                dest->width = src->w;
                dest->height = src->h;
                dest->xOffset = src->xOffset;
                dest->yOffset = src->yOffset;
                dest->xAdvance = src->xAdvance;
                dest->uv = rectXYWH(
                    (f32)src->x / textureWidth,
                    (f32)src->y / textureHeight,
                    (f32)src->w / textureWidth,
                    (f32)src->h / textureHeight);
            }
        }
    }
}
