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

void travelTowards(Worker *worker, Rect *rect) {
	real32 speed = 1.0f;
	V2 movement = centre(rect) - worker->pos;
	worker->pos += limit(movement, speed);
}

void updateWorker(City *city, Worker *worker) {
	if (!worker->exists) return;

	switch (worker->job.type) {
		case JobType_Idle: {
			if (workExists(&city->jobBoard)) {
				worker->job = takeJob(&city->jobBoard);
			}
		} break;

		case JobType_Plant: {
			Building *building = worker->job.building;
			if (inRect(worker->job.building->footprint, worker->pos)) {
				FieldData *field = (FieldData*)building->data;
				doPlantingWork(worker, field);
			} else {
				// Move to field
				travelTowards(worker, &building->footprint);
			}
		} break;

		case JobType_Harvest: {

		} break;
	}
}

void drawWorker(Renderer *renderer, Worker *worker) {
	if (!worker->exists) return;

	drawAtWorldPos(renderer, TextureAtlasItem_Farmer_Stand, worker->pos);
}
