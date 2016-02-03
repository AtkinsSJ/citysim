#pragma once

// worker.cpp

bool hireWorker(City *city, V2 position) {
	if (!city->firstBuildingOfType[BA_Farmhouse]) {
		pushUiMessage("You need a headquarters to hire workers.");
		return false;
	}

	if (!canAfford(city, workerHireCost)) {
		pushUiMessage("Not enough money to hire a worker.");
		return false;
	}

	for (int32 i=0; i<ArrayCount(city->workers); i++) {
		Worker *worker = city->workers + i;
		if (worker->exists == false) {
			// We found an unused worker!
			spend(city, workerHireCost);
			city->workerCount++;
			worker->exists = true;
			worker->pos = position; //v2(city->farmhouse->footprint.pos);
			return true;
		}
	}
	ASSERT(false, "No room to allocate a new worker!");
	return false;
}

void freeWorker(Worker *worker) {
	worker->exists = false;
} 

/**
 * Contribute a day of work to the field.
 * Returns whether the job is done.
 */
bool doPlantingWork(Worker *worker, FieldData *field) {
	if (!worker) {} // Here to keep the compiler quiet re: unused parameter

	if (field->state != FieldState_Planting) {
		// Not planting!
		return true;
	} else {
		field->progressCounter += 1;
		if (field->progressCounter >= fieldProgressToPlant) {
			field->progressCounter -= fieldProgressToPlant;
			field->progress++;
		}

		if (field->progress >= fieldSize) {
			field->state = FieldState_Growing;
			field->progress = 0;
			field->progressCounter = 0;

			return true;
		}
	}
	return false;
}

/**
 * Contribute a day of work to the field.
 * Returns whether the job is done.
 */
bool doHarvestingWork(City *city, Worker *worker, Building *building) {
	if (!worker) {} // Here to keep the compiler quiet re: unused parameter

	if (building->field.state != FieldState_Harvesting) {
		// Not harvesting!
		return true;
	} else {
		building->field.progressCounter += 1;
		if (building->field.progressCounter >= fieldProgressToHarvest) {
			// Create a crop item!
			Potato *potato = 0;
			for (int32 i=0; i<ArrayCount(city->potatoes); i++) {
				if (!city->potatoes[i].exists) {
					potato = city->potatoes + i;
					break;
				}
			}
			ASSERT(potato, "Failed to find a free potato slot!");

			potato->exists = true;
			potato->bounds = realRect(
				v2(building->footprint.pos)
					 + v2(building->field.progress % fieldWidth,
					 		building->field.progress / fieldWidth),
				1,1
			);

			addStoreCropJob(&city->jobBoard, potato);

			building->field.progressCounter -= fieldProgressToHarvest;
			building->field.progress++;
		}

		if (building->field.progress >= fieldSize) {
			building->field.state = FieldState_Empty;
			building->field.progress = 0;
			building->field.progressCounter = 0;

			return true;
		}
	}

	return false;
}

void endJob(Worker *worker) {
	worker->job = {};
	worker->isAtDestination = false;
}

/**
 * Returns whether the worker has reached the destination.
 */
bool workerMoveTo(Worker *worker, Rect rect, GameState *gameState) {
	if (inRect(rect, worker->pos)) {
		// We've reached the destination
		if (worker->isMoving) {
			worker->pos = worker->dayEndPos;
		}
		worker->isAtDestination = true;
		worker->isMoving = false;
		return true;
	}
	if (!worker->isMoving) {
		// Start move
		worker->isMoving = true;
		worker->dayEndPos = worker->pos;
		worker->movementInterpolation = 0;
	}

	// Move to position for end of previous day
	worker->pos = worker->dayEndPos;
	worker->movementInterpolation = 0;

	// // Set-up movement for this day
	// V2 movement = centre(&rect) - worker->pos;
	// worker->dayEndPos = worker->pos + limit(movement, 1.0f);
	Coord pos = coord(worker->pos);
	if (canPathTo(&gameState->city, rect, pos, gameState->arena))
	{
		Coord nextPos = pathToRectangle(&gameState->city, rect, pos, gameState->arena);
		worker->dayEndPos = v2(nextPos.x  + 0.5f, nextPos.y + 0.5f);
	}
	else
	{
		// TODO: Can't path! Need to do something here.
	}

	return inRect(rect, worker->pos);
}

bool workerMoveTo(Worker *worker, Coord coord, GameState *gameState)
{
	return workerMoveTo(worker, irectXYWH(coord.x, coord.y, 1, 1), gameState);
}

void updateWorker(GameState *gameState, Worker *worker) {
	if (!worker->exists) return;

	City *city = &gameState->city;

	// Find a job!
	if (worker->job.type == JobType_Idle && workExists(&city->jobBoard)) {
		
		// Prevent jumping if we were moving towards the farmhouse.
		if (worker->isMoving) {
			worker->pos = worker->renderPos;
		}
		takeJob(&city->jobBoard, worker);
	}

	switch (worker->job.type) {
		case JobType_Idle: {
			 if (!worker->isAtDestination && city->firstBuildingOfType[BA_Farmhouse]) {
				// Walk back to the farmhouse
				workerMoveTo(worker, city->firstBuildingOfType[BA_Farmhouse]->footprint, gameState);
			}
		} break;

		case JobType_Plant: {
			JobData_Plant *jobData = &worker->job.data_Plant;

			// Check job is valid
			Building *field = getBuildingAtPosition(city, jobData->fieldPosition);
			if (!field
				|| field->archetype != BA_Field
				|| field->field.state != FieldState_Planting) {

				endJob(worker);
			} else { // Job is valid, yay!

				if (worker->isAtDestination) {
					if (doPlantingWork(worker, &field->field)) {
						endJob(worker);
					}
				} else {
					workerMoveTo(worker, field->footprint, gameState);
				}
			}
			
		} break;

		case JobType_Harvest: {
			JobData_Harvest *jobData = &worker->job.data_Harvest;

			// Check job is valid
			Building *field = getBuildingAtPosition(city, jobData->fieldPosition);
			if (!field
				|| field->archetype != BA_Field
				|| field->field.state != FieldState_Harvesting) {

				endJob(worker);
			} else { // Job is valid, yay!
				if (worker->isAtDestination) {
					if (doHarvestingWork(city, worker, field)) {
						endJob(worker);
					}
				} else {
					workerMoveTo(worker, field->footprint, gameState);
				}
			}
		} break;

		case JobType_StoreCrop: {
			JobData_StoreCrop *jobData = &worker->job.data_StoreCrop;

			// Find a storage barn!
			Building *barn = getBuildingAtPosition(city, jobData->barnPosition);
			if (!barn)
			{
				Building *closestBarn = null;
				real32 closestBarnDistance = real32Max;

				for (Building *currentBarn = city->firstBuildingOfType[BA_Barn];
					currentBarn; // Should only be null if no barns exist!
					currentBarn = currentBarn->nextOfType)
				{
					real32 distance = v2Length(jobData->potato->bounds.pos - centre(&currentBarn->footprint));
					if (distance < closestBarnDistance) {
						closestBarnDistance = distance;
						closestBarn = currentBarn;
					}

					// Stop when we hit the first one again
					if (currentBarn->nextOfType == city->firstBuildingOfType[BA_Barn])
					{
						break;
					}
				}

				if (closestBarn)
				{
					barn = closestBarn;
					jobData->barnPosition = barn->footprint.pos;
				}
			}

			if (barn) {
				if (worker->isCarryingPotato) {
					if (worker->isAtDestination) {
						// Deposit potato for fun and profit
						sellAPotato(city);
						worker->isCarryingPotato = false;
						endJob(worker);

					} else {
						workerMoveTo(worker, barn->footprint, gameState);
					}
				} else {
					if (worker->isAtDestination) {
						// Pick-up potato for fun and profit
						jobData->potato->exists = false;
						worker->isCarryingPotato = true;
						worker->isAtDestination = false;

					} else {
						workerMoveTo(worker, coord(jobData->potato->bounds.pos), gameState);
					}
				}
			} else {
				worker->isMoving = false;
				pushUiMessage("Construct a barn to store harvested crops!");
			}
		} break;
	}
}

void drawWorker(GLRenderer *renderer, Worker *worker, real32 daysPerFrame) {
	if (!worker->exists) return;

	// TODO: Replace this animation-choosing code, because it's horrible.
	// We shouldset the animation when stuff happens, not calculate it every frame!
	AnimationID targetAnimation = Animation_Farmer_Stand;
	if (worker->isMoving) {
		if (worker->isCarryingPotato) {
			targetAnimation = Animation_Farmer_Carry;
		} else {
			targetAnimation = Animation_Farmer_Walk;
		}
	} else {
		if (worker->isAtDestination && worker->job.type == JobType_Plant) {
			targetAnimation = Animation_Farmer_Plant;
		} else if (worker->isAtDestination && worker->job.type == JobType_Harvest) {
			targetAnimation = Animation_Farmer_Harvest;
		} else if (worker->isCarryingPotato) {
			targetAnimation = Animation_Farmer_Hold;
		} else {
			targetAnimation = Animation_Farmer_Stand;
		}
	}

	setAnimation(&worker->animator, renderer, targetAnimation);

	V2 drawPos = worker->pos;
	real32 depth = depthFromY(drawPos.y) + 1;

	// Interpolate position!
	// FIXME: Workers teleport if their destination is changed, because our interpolation is then undone!
	// We really need a proper system for motion where the workers actually move.
	if (worker->isMoving) {
		worker->movementInterpolation += daysPerFrame;
		drawPos = worker->renderPos = interpolate(worker->pos, worker->dayEndPos, worker->movementInterpolation);

		// DEBUG POTATO!
		Color debugPotatoColor = {0, 0, 255, 255};
		drawTextureAtlasItem(renderer, false, TextureAtlasItem_Potato,
				worker->dayEndPos, v2(1,1), depth, &debugPotatoColor);
	}

	drawAnimator(renderer, false, &worker->animator, daysPerFrame,
				drawPos, v2(0.5f, 0.5f), depth);

	if (worker->isCarryingPotato) {
		drawTextureAtlasItem(renderer, false, TextureAtlasItem_Potato,
				drawPos + potatoCarryOffset, v2(1,1), depth);
	}
}
