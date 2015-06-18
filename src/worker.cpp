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
		field->growthCounter += 1;
		if (field->growthCounter >= 2) {
			field->growthCounter -= 2;
			field->growth++;
		}

		if (field->growth >= fieldSize) {
			field->state = FieldState_Growing;
			field->growth = 0;
			field->growthCounter = 0;

			endJob(worker);
		}
	}
}

void continueMoving(Worker *worker, Rect *rect) {
	// Move to position for end of previous day
	worker->pos = worker->dayEndPos;
	worker->movementInterpolation = 0;

	// Set-up movement for this day
	real32 speed = 1.0f;
	V2 movement = centre(rect) - worker->pos;
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
					continueMoving(worker, &building->footprint);
				} else {
					// Start move
					worker->isMoving = true;
					worker->dayEndPos = worker->pos;
					worker->movementInterpolation = 0;

					continueMoving(worker, &building->footprint);
				}
			}
			
		} break;

		case JobType_Harvest: {

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
