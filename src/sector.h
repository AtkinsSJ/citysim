#pragma once

template<typename Sector>
struct SectorGrid
{
	s32 width, height; // world size
	s32 sectorSize;
	s32 sectorsX, sectorsY;

	Array<Sector> sectors;

	s32 nextSectorUpdateIndex;
	s32 sectorsToUpdatePerTick;

	inline Sector &operator[](s32 index)
	{
		return this->sectors[index];
	}
};

struct BasicSector
{
	Rect2I bounds;
};

// NB: The Sector struct needs to contain a "Rect2I bounds;" member. This is filled-in inside initSectorGrid().

template<typename Sector>
void initSectorGrid(SectorGrid<Sector> *grid, MemoryArena *arena, s32 cityWidth, s32 cityHeight, s32 sectorSize, s32 sectorsToUpdatePerTick=0);

template<typename Sector>
Sector *getSector(SectorGrid<Sector> *grid, s32 sectorX, s32 sectorY);

template<typename Sector>
Sector *getSectorByIndex(SectorGrid<Sector> *grid, s32 index);

template<typename Sector>
inline s32 getSectorCount(SectorGrid<Sector> *grid)
{
	return grid->sectors.count;
}

template<typename Sector>
Sector *getSectorAtTilePos(SectorGrid<Sector> *grid, s32 x, s32 y);

template<typename Sector>
Rect2I getSectorsCovered(SectorGrid<Sector> *grid, Rect2I area);

template<typename Sector>
Sector *getNextSector(SectorGrid<Sector> *grid);

template<typename Sector>
Indexed<Sector *> getNextSectorWithIndex(SectorGrid<Sector> *grid);
