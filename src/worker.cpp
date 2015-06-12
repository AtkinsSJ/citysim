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
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No room to allocate a new worker!");
	pushUiMessage("The worker limit has been reached.");
	SDL_assert(false);
	return false;
}

void freeWorker(Worker *worker) {
	worker->exists = false;
}

void updateWorker(Worker *worker) {
	if (!worker->exists) return;

	worker->pos.x += 0.1f;
}

void drawWorker(Renderer *renderer, Worker *worker) {
	if (!worker->exists) return;

	drawAtWorldPos(renderer, TextureAtlasItem_Farmer_Stand, worker->pos);
}
