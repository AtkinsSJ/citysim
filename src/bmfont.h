#pragma once

// See: http://www.angelcode.com/products/bmfont/doc/file_format.html

#pragma pack(push, 1)
struct BMFontHeader
{
	u8 tag[3];
	u8 version;
};
const u8 BMFontTag[3] = {'B', 'M', 'F'};
const u8 BMFontSupportedVersion = 3;

struct BMFontBlockHeader
{
	u8 type;
	u32 size;
};
enum BMFontBlockTypes
{
	BMF_Block_Info = 1,
	BMF_Block_Common = 2,
	BMF_Block_Pages = 3,
	BMF_Block_Chars = 4,
	BMF_Block_KerningPairs = 5,
};

enum BMFont_ColorChannelType
{
	BMFont_CCT_Glyph = 0,
	BMFont_CCT_Outline = 1,
	BMFont_CCT_Combined = 2, // Glyph and outline
	BMFont_CCT_Zero = 3,
	BMFont_CCT_One = 4,
};
struct BMFontBlock_Common
{
	u16 lineHeight;
	u16 base; // Distance from top of line to the character baseline
	u16 scaleW, scaleH; // GL_Texture dimensions
	u16 pageCount; // How many texture pages?

	u8 bitfield; // 1 if 8-bit character data is packed into all channels
	u8 alphaChannel;
	u8 redChannel;
	u8 greenChannel;
	u8 blueChannel;
};

struct BMFont_Char
{
	u32 id;
	u16 x, y;
	u16 w, h;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character
	u8 page; // GL_Texture page
	u8 channel; // Bitfield, 1 = blue, 2 = green, 4 = red, 8 = alpha
};

#pragma pack(pop)

void loadBMFont(struct AssetManager *assets, Blob data, struct Asset *asset);
