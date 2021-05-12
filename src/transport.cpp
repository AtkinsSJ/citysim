#pragma once

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena)
{
	layer->tileTransportTypes = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);

	layer->transportMaxDistance = 8;
	initDirtyRects(&layer->dirtyRects, gameArena, layer->transportMaxDistance, city->bounds);

	for (s32 type = 0; type < TransportTypeCount; type++)
	{
		layer->tileTransportDistance[type] = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
		fill<u8>(&layer->tileTransportDistance[type], 255);
	}
}

void updateTransportLayer(City *city, TransportLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		// Calculate transport types on each tile
		// So, I have two ideas about this:
		// 1: memset the area to 0 and then iterate through all the buildings in that area, applying their transport types
		// or 2: go tile-by-tile within the area, and see if a building exists or not, and set the transport types from that.
		// I'm not sure which is better? Right now a memset is easy because we're doing the entire map, but
		// that won't be the case for long and memsetting a rectangle within the city is going to be way fiddlier.
		// Also, there's a performance cost to gathering all the buildings up in that area into an array for iterating.
		// So, I think #2 is the better option, but I should test that later if it becomes expensive performance-wise.
		// - Sam, 28/08/2019

		for (auto it = layer->dirtyRects.rects.iterate();
			it.hasNext();
			it.next())
		{
			Rect2I dirtyRect = it.getValue();

			// Transport types on tile, based on what buildings are present
			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					Building *building = getBuildingAt(city, x, y);
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building->typeID);
						layer->tileTransportTypes.set(x, y, getAll(&def->transportTypes));
					}
					else
					{
						layer->tileTransportTypes.set(x, y, 0);
					}
				}
			}

			// Clear the surrounding "distance to road" stuff from the rectangle
			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					for (s32 type = 0; type < TransportTypeCount; type++)
					{
						u8 distance = doesTileHaveTransport(city, x, y, (TransportType)type) ? 0 : 255;
						layer->tileTransportDistance[type].set(x, y, distance);
					}
				}
			}
		}

		// Transport distance recalculation
		for (s32 type = 0; type < TransportTypeCount; type++)
		{
			updateDistances(&layer->tileTransportDistance[type], &layer->dirtyRects, layer->transportMaxDistance);
		}

		clearDirtyRects(&layer->dirtyRects);
	}
}

void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

bool doesTileHaveTransport(City *city, s32 x, s32 y, TransportType type)
{
	bool result = false;

	if (tileExists(city, x, y))
	{
		u8 transportTypesAtTile = city->transportLayer.tileTransportTypes.get(x, y);
		result = (transportTypesAtTile & (1 << type)) != 0;
	}

	return result;
}

bool doesTileHaveTransport(City *city, s32 x, s32 y, Flags_TransportType types)
{
	bool result = false;

	if (tileExists(city, x, y))
	{
		u8 transportTypesAtTile = city->transportLayer.tileTransportTypes.get(x, y);
		result = (transportTypesAtTile & getAll(&types)) != 0;
	}

	return result;
}

void addTransportToTile(City *city, s32 x, s32 y, TransportType type)
{
	u8 oldValue = city->transportLayer.tileTransportTypes.get(x, y);
	u8 newValue = oldValue | (1 << type);
	city->transportLayer.tileTransportTypes.set(x, y, newValue);
}

void addTransportToTile(City *city, s32 x, s32 y, Flags_TransportType types)
{
	u8 oldValue = city->transportLayer.tileTransportTypes.get(x, y);
	u8 newValue = oldValue | getAll(&types);
	city->transportLayer.tileTransportTypes.set(x, y, newValue);
}

inline s32 getDistanceToTransport(City *city, s32 x, s32 y, TransportType type)
{
	return city->transportLayer.tileTransportDistance[type].get(x, y);
}

void debugInspectTransport(UIPanel *panel, City *city, s32 x, s32 y)
{
	panel->addText("*** TRANSPORT INFO ***"_s);

	// Transport
	for (s32 transportType = 0; transportType < TransportTypeCount; transportType++)
	{
		panel->addText(myprintf("Distance to transport #{0}: {1}"_s, {formatInt(transportType), formatInt(getDistanceToTransport(city, x, y, (TransportType)transportType))}));
	}
}

void saveTransportLayer(TransportLayer * /*layer*/, struct BinaryFileWriter *writer)
{
	writer->startSection<SAVSection_Transport>(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
	SAVSection_Transport transportSection = {};

	writer->endSection<SAVSection_Transport>(&transportSection);
}

bool loadTransportLayer(TransportLayer * /*layer*/, City * /*city*/, struct BinaryFileReader *reader)
{
	bool succeeded = false;
	while (reader->startSection(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION))
	{
		SAVSection_Transport *section = reader->readStruct<SAVSection_Transport>(0);
		if (!section) break;

		// TODO: Implement Transport!

		succeeded = true;
		break;
	}

	return succeeded;
}
