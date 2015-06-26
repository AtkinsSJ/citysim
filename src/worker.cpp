#pragma once

// worker.cpp

bool hireWorker(City *city, V2 position) {
	if (!city->farmhouse) {
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

	FieldData *field = (FieldData*) building->data;
	if (field->state != FieldState_Harvesting) {
		// Not harvesting!
		return true;
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
				1,1
			);

			addStoreCropJob(&city->jobBoard, potato);

			field->progressCounter -= fieldProgressToHarvest;
			field->progress++;
		}

		if (field->progress >= fieldSize) {
			field->state = FieldState_Empty;
			field->progress = 0;
			field->progressCounter = 0;

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
bool workerMoveTo(Worker *worker, RealRect rect, real32 speed = 1.0f) {
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

	// Set-up movement for this day
	V2 movement = centre(&rect) - worker->pos;
	worker->dayEndPos = worker->pos + limit(movement, speed);

	return inRect(rect, worker->pos);
}

void updateWorker(City *city, Worker *worker) {
	if (!worker->exists) return;

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
			 if (!worker->isAtDestination && city->farmhouse) {
				// Slowly wander back to the farmhouse
				workerMoveTo(worker, realRect(city->farmhouse->footprint), 0.5f);
			}
		} break;

		case JobType_Plant: {
			Building *building = worker->job.building;
			FieldData *field = (FieldData*)building->data;

			// Check job is valid
			if (!worker->job.building->exists
				|| worker->job.building->archetype != BA_Field
				|| !field
				|| field->state != FieldState_Planting) {

				endJob(worker);
			} else { // Job is valid, yay!

				if (worker->isAtDestination) {
					if (doPlantingWork(worker, field)) {
						endJob(worker);
					}
				} else {
					workerMoveTo(worker, realRect(building->footprint));
				}
			}
			
		} break;

		case JobType_Harvest: {
			Building *building = worker->job.building;
			FieldData *field = (FieldData*)building->data;

			// Check job is valid
			if (!worker->job.building->exists
				|| worker->job.building->archetype != BA_Field
				|| !field
				|| field->state != FieldState_Harvesting) {

				endJob(worker);
			} else { // Job is valid, yay!
				if (worker->isAtDestination) {
					if (doHarvestingWork(city, worker, building)) {
						endJob(worker);
					}
				} else {
					workerMoveTo(worker, realRect(building->footprint));
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
			}

			if (worker->job.building) {
				Building *building = worker->job.building;
				if (worker->isCarryingPotato) {
					if (worker->isAtDestination) {
						// Deposit potato for fun and profit
						sellAPotato(city);
						worker->isCarryingPotato = false;
						endJob(worker);

					} else {
						workerMoveTo(worker, realRect(building->footprint));
					}
				} else {
					Potato *potato = worker->job.potato;
					if (worker->isAtDestination) {
						// Pick-up potato for fun and profit
						potato->exists = false;
						worker->isCarryingPotato = true;
						worker->isAtDestination = false;

					} else {
						workerMoveTo(worker, potato->bounds);
					}
				}
			} else {
				pushUiMessage("Construct a barn to store harvested crops!");
			}
		} break;
	}
}

void drawWorker(Renderer *renderer, Worker *worker, real32 daysPerFrame) {
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

	// Interpolate position!
	if (worker->isMoving) {
		worker->movementInterpolation += daysPerFrame;
		drawPos = worker->renderPos = interpolate(worker->pos, worker->dayEndPos, worker->movementInterpolation);
	}

	drawAnimator(renderer, &worker->animator, daysPerFrame, drawPos);

	if (worker->isCarryingPotato) {
		drawAtWorldPos(renderer, TextureAtlasItem_Potato, drawPos + potatoCarryOffset);
	}
}
