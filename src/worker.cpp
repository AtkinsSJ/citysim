#pragma once

// worker.cpp

bool hireWorker(City *city) {
	if (!city->farmhouse) {
		pushUiMessage("You need a headquarters to hire workers.");
		return false;
	}

	if (city->funds < workerHireCost) {
		pushUiMessage("Not enough money to hire a worker.");
		return false;
	}

	for (int32 i=0; i<ArrayCount(city->workers); i++) {
		Worker *worker = city->workers + i;
		if (worker->exists == false) {
			// We found an unused worker!
			city->funds -= workerHireCost;
			worker->exists = true;
			worker->pos = v2(city->farmhouse->footprint.pos);
			return true;
		}
	}
	ASSERT(false, "No room to allocate a new worker!");
	return false;
}

void freeWorker(Worker *worker) {
	worker->exists = false;
}

void endJob(Worker *worker) {
	worker->job = {};
} 

/**
 * Contribute a day of work to the field.
 */
void doPlantingWork(Worker *worker, FieldData *field) {
	if (field->state != FieldState_Planting) {
		// Not planting!
		endJob(worker);
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

			endJob(worker);
		}
	}
}

void doHarvestingWork(City *city, Worker *worker, Building *building) {
	FieldData *field = (FieldData*) building->data;
	if (field->state != FieldState_Harvesting) {
		// Not harvesting!
		endJob(worker);
	} else {
		field->progressCounter += 1;
		if (field->progressCounter >= fieldProgressToHarvest) {
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
					 + v2(field->progress % fieldWidth, field->progress / fieldWidth),
				16, 16
			);

			addStoreCropJob(&city->jobBoard, potato);

			field->progressCounter -= fieldProgressToHarvest;
			field->progress++;
		}

		if (field->progress >= fieldSize) {
			field->state = FieldState_Empty;
			field->progress = 0;
			field->progressCounter = 0;

			endJob(worker);
		}
	}
}

void continueMoving(Worker *worker, RealRect rect) {
	// Move to position for end of previous day
	worker->pos = worker->dayEndPos;
	worker->movementInterpolation = 0;

	// Set-up movement for this day
	real32 speed = 1.0f;
	V2 movement = centre(&rect) - worker->pos;
	worker->dayEndPos = worker->pos + limit(movement, speed);
}

void updateWorker(City *city, Worker *worker) {
	if (!worker->exists) return;

	switch (worker->job.type) {
		case JobType_Idle: {
			if (workExists(&city->jobBoard)) {
				takeJob(&city->jobBoard, worker);
			}
		} break;

		case JobType_Plant: {
			Building *building = worker->job.building;

			if (worker->isAtDestination) {
				FieldData *field = (FieldData*)building->data;
				doPlantingWork(worker, field);
			} else {
				// Move to the destination

				if (inRect(building->footprint, worker->pos)) {
					// We've reached the destination
					worker->pos = worker->dayEndPos;
					worker->isAtDestination = true;
					worker->isMoving = false;
				} else if (worker->isMoving) {
					// Continue move
					continueMoving(worker, realRect(building->footprint));
				} else {
					// Start move
					worker->isMoving = true;
					worker->dayEndPos = worker->pos;
					worker->movementInterpolation = 0;

					continueMoving(worker, realRect(building->footprint));
				}
			}
			
		} break;

		case JobType_Harvest: {
			Building *building = worker->job.building;

			if (worker->isAtDestination) {
				doHarvestingWork(city, worker, building);
			} else {
				// Move to the destination

				if (inRect(building->footprint, worker->pos)) {
					// We've reached the destination
					worker->pos = worker->dayEndPos;
					worker->isAtDestination = true;
					worker->isMoving = false;
				} else if (worker->isMoving) {
					// Continue move
					continueMoving(worker, realRect(building->footprint));
				} else {
					// Start move
					worker->isMoving = true;
					worker->dayEndPos = worker->pos;
					worker->movementInterpolation = 0;

					continueMoving(worker, realRect(building->footprint));
				}
			}
		} break;

		case JobType_StoreCrop: {
			// Find a storage barn!
			if (!worker->job.building) {
				if (city->barnCount) {
					Building *closestBarn = 0;
					real32 closestBarnDistance = real32Max;

					for (uint32 i=0; i<city->barnCount; i++) {
						Building *barn = city->barns[i];
						real32 distance = v2Length(worker->pos - centre(&barn->footprint));
						if (distance < closestBarnDistance) {
							closestBarnDistance = distance;
							closestBarn = barn;
						}
					}

					worker->job.building = closestBarn;
				}
			} else {
				Building *building = worker->job.building;
				if (worker->isCarryingPotato) {
					if (worker->isAtDestination) {
						// Deposit potato for fun and profit
						sellAPotato(city);
						worker->isCarryingPotato = false;
						endJob(worker);

					} else if (worker->isMoving) {
						continueMoving(worker, realRect(building->footprint));
					} else {
						// Start move
						worker->isMoving = true;
						worker->dayEndPos = worker->pos;
						worker->movementInterpolation = 0;

						continueMoving(worker, realRect(building->footprint));
					}
				} else {
					Potato *potato = worker->job.potato;
					if (worker->isAtDestination) {
						// Pick-up potato for fun and profit
						potato->exists = false;
						worker->isCarryingPotato = true;
						worker->isAtDestination = false;

					} else if (worker->isMoving) {
						continueMoving(worker, potato->bounds);
					} else {
						// Start move
						worker->isMoving = true;
						worker->dayEndPos = worker->pos;
						worker->movementInterpolation = 0;

						continueMoving(worker, potato->bounds);
					}
				}
			}
		} break;
	}
}

void drawWorker(Renderer *renderer, Worker *worker, real32 daysPerFrame) {
	if (!worker->exists) return;

	V2 drawPos = worker->pos;

	// Interpolate position!
	if (worker->isMoving) {
		worker->movementInterpolation += daysPerFrame;
		drawPos = worker->renderPos = interpolate(worker->pos, worker->dayEndPos, worker->movementInterpolation);
	}

	drawAtWorldPos(renderer, TextureAtlasItem_Farmer_Stand, drawPos);
}
