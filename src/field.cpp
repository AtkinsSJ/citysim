#pragma once

// field.h

bool plantField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition.x, tilePosition.y);
	if (building && building->archetype == BA_Field) {
		FieldData *field = (FieldData*)building->data;
		if (!canAfford(city, fieldPlantCost)) {
			pushUiMessage("Not enough money to plant this field.");
			return false;
		} else if (field->state != FieldState_Empty) {
			pushUiMessage("This field has already been planted.");
			return false;
		} else {
			SDL_Log("Marking field for planting.");
			field->state = FieldState_Planting;
			field->progress = 0;
			spend(city, fieldPlantCost);

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
		if (field->state == FieldState_Harvesting) {
			pushUiMessage("This field is already marked for harvesting.");
			return false;
		} else if (field->state != FieldState_Grown) {
			pushUiMessage("There are no plants here ready for harvesting.");
			return false;
		} else {
			SDL_Log("Marking field for harvesting.");

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

void drawField(GLRenderer *renderer, Building *building, Color *drawColor) {

	V2 centrePos = centre(&building->footprint);

	drawTextureAtlasItem(renderer, false, TextureAtlasItem_Field,
		centrePos, v2(building->footprint.dim), depthFromY(building->footprint.y), drawColor);

	FieldData *field = (FieldData*)building->data;

	switch (field->state) {
		case FieldState_Planting: {
			for (int32 i=0; i < field->progress; i++) {
				V2 plantPos = v2(building->footprint.pos.x + (i%4) + 0.5f,
					 			 building->footprint.pos.y + (i/4) + 0.5f);
				drawTextureAtlasItem(
					renderer,
					false,
					TextureAtlasItem_Crop0_0,
					plantPos,
					v2(1,1),
					depthFromY(plantPos.y),
					drawColor
				);
			}

			// 'Planting' indicator
			drawTextureAtlasItem(
				renderer,
				false,
				TextureAtlasItem_Icon_Planting,
				centrePos,
				v2(2,2),
				depthFromY(centrePos.y) + 10
			);
		} break;

		case FieldState_Growing: {
			int32 baseGrowthStage = field->progress / fieldSize;
			int32 beyondGrowth = field->progress % fieldSize;
			for (int32 i=0; i < fieldSize; i++) {
				V2 plantPos = v2(building->footprint.pos.x + (i%4) + 0.5f,
					 			 building->footprint.pos.y + (i/4) + 0.5f);
				drawTextureAtlasItem(
					renderer,
					false,
					(TextureAtlasItem)(TextureAtlasItem_Crop0_0 + baseGrowthStage + (i < beyondGrowth ? 1 : 0)),
					plantPos,
					v2(1,1),
					depthFromY(plantPos.y),
					drawColor
				);
			}
		} break;

		case FieldState_Grown: {
			for (int32 i=0; i < fieldSize; i++) {
				V2 plantPos = v2(building->footprint.pos.x + (i%4) + 0.5f,
					 			 building->footprint.pos.y + (i/4) + 0.5f);
				drawTextureAtlasItem(
					renderer,
					false,
					TextureAtlasItem_Crop0_3,
					plantPos,
					v2(1,1),
					depthFromY(plantPos.y),
					drawColor
				);
			}
		} break;

		case FieldState_Harvesting: {

			for (int32 i=0; i < fieldSize; i++) {
				if (i < field->progress) continue;

				V2 plantPos = v2(building->footprint.pos.x + (i%4) + 0.5f,
					 			 building->footprint.pos.y + (i/4) + 0.5f);
				drawTextureAtlasItem(
					renderer,
					false,
					TextureAtlasItem_Crop0_3,
					plantPos,
					v2(1,1),
					depthFromY(plantPos.y),
					drawColor
				);
			}

			// 'Harvesting' indicator
			drawTextureAtlasItem(
				renderer,
				false,
				TextureAtlasItem_Icon_Harvesting,
				centrePos,
				v2(2,2),
				depthFromY(centrePos.y) + 10
			);
		} break;
	}
}