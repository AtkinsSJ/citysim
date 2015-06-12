#pragma once

// field.h

bool plantField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition.x, tilePosition.y);
	if (building && building->archetype == BA_Field) {
		FieldData *field = (FieldData*)building->data;
		if ((field->state == FieldState_Empty) && (city->funds >= 350)) {
			SDL_Log("Planting something in this field.");
			field->state = FieldState_Planting;
			field->growth = 0;
			city->funds -= 500;
			return true;
		}
	}

	return false;
}

bool harvestField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition.x, tilePosition.y);
	if (building && building->archetype == BA_Field) {
		FieldData *field = (FieldData*)building->data;
		if (field->state == FieldState_Grown) {
			SDL_Log("Harvested %d plants from this field.", field->growth);
			city->funds += field->growth * 150;
			field->state = FieldState_Empty;
			field->growth = 0;
			return true;
		}
	}

	return false;
}

void updateField(FieldData *field) {
	if (!field->exists) return;

	switch (field->state) {
		case FieldState_Planting: {
			// Planting a seed at each position
			// We reuse growth/growthCounter because we can
			field->growthCounter += 1;
			if (field->growthCounter >= 2) {
				field->growthCounter -= 2;
				field->growth++;
			}
			if (field->growth >= fieldSize) {
				field->state = FieldState_Growing;
				field->growth = 0;
				field->growthCounter = 0;
			}
		} break;

		case FieldState_Growing: {
			// Growing each plant in turn
			field->growthCounter += 1;
			if (field->growthCounter >= 2) {
				field->growthCounter -= 2;
				field->growth++;
			}
			if (field->growth >= fieldMaxGrowth) {
				field->state = FieldState_Grown;
			}
		} break;

		case FieldState_Harvesting: {
			// Harvesting each plant in turn
		} break;
	}
}

void drawField(Renderer *renderer, Building *building, Color *drawColor) {

	drawAtWorldPos(renderer, TextureAtlasItem_Field, v2(building->footprint.pos), drawColor);

	FieldData *field = (FieldData*)building->data;

	switch (field->state) {
		case FieldState_Planting: {
			for (int32 i=0; i < field->growth; i++) {
				drawAtWorldPos(renderer,
					TextureAtlasItem_Crop0_0,
					v2({building->footprint.pos.x + (i%4),
					 building->footprint.pos.y + (i/4)}),
					drawColor
				);
			}
		} break;

		case FieldState_Growing: {
			int32 baseGrowthStage = field->growth / fieldSize;
			int32 beyondGrowth = field->growth % fieldSize;
			for (int32 i=0; i < fieldSize; i++) {
				drawAtWorldPos(renderer,
					(TextureAtlasItem)(TextureAtlasItem_Crop0_0 + baseGrowthStage + (i < beyondGrowth ? 1 : 0)),
					v2({building->footprint.pos.x + (i%4),
					 building->footprint.pos.y + (i/4)}),
					drawColor
				);
			}
		} break;

		case FieldState_Grown: {
			for (int32 i=0; i < fieldSize; i++) {
				drawAtWorldPos(renderer,
					TextureAtlasItem_Crop0_3,
					v2({building->footprint.pos.x + (i%4),
					 building->footprint.pos.y + (i/4)}),
					drawColor
				);
			}
		} break;

		case FieldState_Harvesting: {

		} break;
	}
}