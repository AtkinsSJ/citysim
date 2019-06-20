#pragma once

template<typename Sector>
struct SectorGrid
{
	s32 width, height; // world size
	s32 sectorSize;
	s32 sectorsX, sectorsY;

	s32 count;
	Sector *sectors;
};

template<typename Sector>
void initSectorGrid(SectorGrid<Sector> *grid, MemoryArena *arena, s32 cityWidth, s32 cityHeight, s32 sectorSize);

template<typename Sector>
Sector *getSector(SectorGrid<Sector> *grid, s32 sectorX, s32 sectorY);

template<typename Sector>
Sector *getSectorAtTilePos(SectorGrid<Sector> *grid, s32 x, s32 y);

template<typename Sector>
Rect2I getSectorsCovered(SectorGrid<Sector> *grid, Rect2I area);

template<typename Sector, typename T>
inline T *getSectorTile(Sector *sector, T *tiles, s32 x, s32 y)
{
	return tiles + (y * sector->bounds.w) + x;
}

template<typename Sector, typename T>
inline void setSectorTile(Sector *sector, T *tiles, s32 x, s32 y, T value)
{
	tiles[(y * sector->bounds.w) + x] = value;
}
