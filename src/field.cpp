#pragma once

// field.h

bool plantField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition.x, tilePosition.y);
	if (building && building->archetype == BA_Field) {
		FieldData *field = (FieldData*)building->data;
		if (city->funds < 350) {
			pushUiMessage("Not enough money to plant this field.");
			return false;
		} else if (field->state != FieldState_Empty) {
			pushUiMessage("This field has already been planted.");
			return false;
		} else {
			SDL_Log("Marking field for planting.");
			field->state = FieldState_Planting;
			field->progress = 0;
			city->funds -= 500;

			addPlantJob(&city->jobBoard, building);

			return true;
		}
	}

	pushUiMessage("You can only plant fields.");
	return false;
}

bool harvestField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition.x, tilePosition.y);
	if (building && building->archetype == BA_Field) {
		FieldData *field = (FieldData*)building->data;
		if (field->state != FieldState_Grown) {
			pushUiMessage("There are no plants here ready for harvesting.");
			return false;
		} else {
			SDL_Log("Marking field for harvesting.");
			// city->funds += field->growth * 150;
			// field->state = FieldState_Empty;
			// field->growth = 0;

			field->state = FieldState_Harvesting;
			field->progress = 0;
			addHarvestJob(&city->jobBoard, building);

			return true;
		}
	}

	pushUiMessage("You can only harvest fields.");
	return false;
}

void updateField(FieldData *field) {
	if (!field->exists) return;

	switch (field->state) {
		case FieldState_Planting: {
			// Planting a seed at each position
			// We reuse growth/growthCounter because we can

			// field->growthCounter += 1;
			// if (field->growthCounter >= 2) {
			// 	field->growthCounter -= 2;
			// 	field->growth++;
			// }

			// if (field->growth >= fieldSize) {
			// 	field->state = FieldState_Growing;
			// 	field->growth = 0;
			// 	field->growthCounter = 0;
			// }
		} break;

		case FieldState_Growing: {
			// Growing each plant in turn
			field->progressCounter += 1;
			if (field->progressCounter >= 1) {
				field->progressCounter -= 1;
				field->progress++;
			}
			if (field->progress >= fieldMaxGrowth) {
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
			for (int32 i=0; i < field->progress; i++) {
				drawAtWorldPos(renderer,
					TextureAtlasItem_Crop0_0,
					v2(	building->footprint.pos.x + (i%4),
					 	building->footprint.pos.y + (i/4)),
					drawColor
				);
			}

			// 'Planting' indicator
			drawAtWorldPos(renderer,
				TextureAtlasItem_Icon_Planting,
				v2( building->footprint.pos.x - 1 + building->footprint.w/2,
					building->footprint.pos.y - 2 + building->footprint.h/2 )
			);
		} break;

		case FieldState_Growing: {
			int32 baseGrowthStage = field->progress / fieldSize;
			int32 beyondGrowth = field->progress % fieldSize;
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

			for (int32 i=0; i < fieldSize; i++) {
				if (i < field->progress) continue;

				drawAtWorldPos(renderer,
					TextureAtlasItem_Crop0_3,
					v2({building->footprint.pos.x + (i%4),
					 building->footprint.pos.y + (i/4)}),
					drawColor
				);
			}

			// 'Harvesting' indicator
			drawAtWorldPos(renderer,
				TextureAtlasItem_Icon_Harvesting,
				v2( building->footprint.pos.x - 1 + building->footprint.w/2,
					building->footprint.pos.y - 2 + building->footprint.h/2 )
			);
		} break;
	}
}