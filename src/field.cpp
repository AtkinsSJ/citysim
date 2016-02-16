#pragma once

// field.h

bool plantField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition);
	if (building && building->archetype == BA_Field) {
		if (!canAfford(city, fieldPlantCost)) {
			pushUiMessage("Not enough money to plant this field.");
			return false;
		} else if (building->field.state != FieldState_Empty) {
			pushUiMessage("This field has already been planted.");
			return false;
		} else {
			SDL_Log("Marking field for planting.");
			building->field.state = FieldState_Planting;
			building->field.progress = 0;
			spend(city, fieldPlantCost);

			addPlantJob(&city->jobBoard, tilePosition);

			return true;
		}
	}

	pushUiMessage("You can only plant fields.");
	return false;
}

bool harvestField(City *city, Coord tilePosition) {
	Building *building = getBuildingAtPosition(city, tilePosition);
	if (building && building->archetype == BA_Field) {
		if (building->field.state == FieldState_Harvesting) {
			pushUiMessage("This field is already marked for harvesting.");
			return false;
		} else if (building->field.state != FieldState_Grown) {
			pushUiMessage("There are no plants here ready for harvesting.");
			return false;
		} else {
			SDL_Log("Marking field for harvesting.");

			building->field.state = FieldState_Harvesting;
			building->field.progress = 0;
			addHarvestJob(&city->jobBoard, tilePosition);

			return true;
		}
	}

	pushUiMessage("You can only harvest fields.");
	return false;
}

void updateField(FieldData *field) {
	switch (field->state) {
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
	}
}

void drawField(GLRenderer *renderer, Building *building, V4 drawColor) {

	V2 centrePos = centre(building->footprint);

	drawTextureAtlasItem(renderer, false, TextureAtlasItem_Field,
		centrePos, v2(building->footprint.dim), depthFromY(building->footprint.y), drawColor);

	switch (building->field.state) {
		case FieldState_Planting: {
			for (int32 i=0; i < building->field.progress; i++) {
				V2 plantPos = v2(building->footprint.pos.x + (i%fieldWidth) + 0.5f,
					 			 building->footprint.pos.y + (i/fieldWidth) + 0.5f);
				drawTextureAtlasItem(renderer, false, TextureAtlasItem_Crop0_0, plantPos, v2(1,1),
									depthFromY(plantPos.y), drawColor);
			}

			// 'Planting' indicator
			drawTextureAtlasItem(renderer, false, TextureAtlasItem_Icon_Planting, centrePos, v2(2,2),
								depthFromY(centrePos.y) + 10);
		} break;

		case FieldState_Growing: {
			int32 baseGrowthStage = building->field.progress / fieldSize;
			int32 beyondGrowth = building->field.progress % fieldSize;
			for (int32 i=0; i < fieldSize; i++) {
				V2 plantPos = v2(building->footprint.pos.x + (i%fieldWidth) + 0.5f,
					 			 building->footprint.pos.y + (i/fieldWidth) + 0.5f);
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
				V2 plantPos = v2(building->footprint.pos.x + (i%fieldWidth) + 0.5f,
					 			 building->footprint.pos.y + (i/fieldWidth) + 0.5f);
				drawTextureAtlasItem(renderer, false, TextureAtlasItem_Crop0_3, plantPos, v2(1,1),
									depthFromY(plantPos.y), drawColor);
			}
		} break;

		case FieldState_Harvesting: {

			for (int32 i=0; i < fieldSize; i++) {


				V2 plantPos = v2(building->footprint.pos.x + (i%fieldWidth) + 0.5f,
					 			 building->footprint.pos.y + (i/fieldWidth) + 0.5f);

				TextureAtlasItem drawItem = TextureAtlasItem_Crop0_3;
				if (i < building->field.progress)
				{
					drawItem = TextureAtlasItem_Potato;
				}
				drawTextureAtlasItem(renderer, false, drawItem, plantPos, v2(1,1),
									depthFromY(plantPos.y), drawColor);
			}

			// 'Harvesting' indicator
			drawTextureAtlasItem(renderer, false, TextureAtlasItem_Icon_Harvesting, centrePos, v2(2,2),
								depthFromY(centrePos.y) + 10);
		} break;

		case FieldState_Gathering: {
			for (int32 i=building->field.progress; i < fieldSize; i++) {

				V2 plantPos = v2(building->footprint.pos.x + (i%fieldWidth) + 0.5f,
					 			 building->footprint.pos.y + (i/fieldWidth) + 0.5f);
				drawTextureAtlasItem(renderer, false, TextureAtlasItem_Potato, plantPos, v2(1,1),
									depthFromY(plantPos.y), drawColor);
			}
		} break;
	}
}