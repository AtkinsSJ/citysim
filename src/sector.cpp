#pragma once

template<typename SectorType>
void initSectorGrid(SectorGrid<SectorType> *grid, MemoryArena *arena, s32 cityWidth, s32 cityHeight, s32 sectorSize)
{
	grid->width  = cityWidth;
	grid->height = cityHeight;
	grid->sectorSize = sectorSize;
	grid->sectorsX = divideCeil(cityWidth, sectorSize);
	grid->sectorsY = divideCeil(cityHeight, sectorSize);

	grid->sectors = allocateArray<SectorType>(arena, grid->sectorsX * grid->sectorsY);

	s32 remainderWidth  = cityWidth  % sectorSize;
	s32 remainderHeight = cityHeight % sectorSize;
	for (s32 y = 0; y < grid->sectorsY; y++)
	{
		for (s32 x = 0; x < grid->sectorsX; x++)
		{
			SectorType *sector = &grid->sectors[(grid->sectorsX * y) + x];

			*sector = {};
			sector->bounds = irectXYWH(x * sectorSize, y * sectorSize, sectorSize, sectorSize);

			if ((x == grid->sectorsX - 1) && remainderWidth > 0)
			{
				sector->bounds.w = remainderWidth;
			}
			if ((y == grid->sectorsY - 1) && remainderHeight > 0)
			{
				sector->bounds.h = remainderHeight;
			}
		}
	}
}

template<typename SectorType>
inline SectorType *getSector(SectorGrid<SectorType> *grid, s32 sectorX, s32 sectorY)
{
	SectorType *result = null;

	if (sectorX >= 0 && sectorX < grid->sectorsX && sectorY >= 0 && sectorY < grid->sectorsY)
	{
		result = &grid->sectors[(sectorY * grid->sectorsX) + sectorX];
	}

	return result;
}

template<typename SectorType>
inline SectorType *getSectorAtTilePos(SectorGrid<SectorType> *grid, s32 x, s32 y)
{
	SectorType *result = null;

	if (x >= 0 && x < grid->width && y >= 0 && y < grid->height)
	{
		result = getSector(grid, x / grid->sectorSize, y / grid->sectorSize);
	}

	return result;
}

template<typename SectorType>
inline Rect2I getSectorsCovered(SectorGrid<SectorType> *grid, Rect2I area)
{
	area = intersect(area, irectXYWH(0, 0, grid->width, grid->height));

	Rect2I result = irectMinMax(
		area.x / grid->sectorSize,
		area.y / grid->sectorSize,

		(area.x + area.w) / grid->sectorSize,
		(area.y + area.h) / grid->sectorSize
	);

	return result;
}
