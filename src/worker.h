#pragma once

// worker.h

struct Worker {
	bool exists;
	V2 pos;
};
struct WorkerList {
	Worker list[128];
};

Worker *addWorker(WorkerList *workers) {
	for (int32 i=0; i<ArrayCount(workers->list); i++) {
		if (workers->list[i].exists == false) {
			return workers->list + i;
		}
	}
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No room to allocate a new worker!");
	SDL_assert(false);
}

void initWorker(Worker *worker, real32 x, real32 y) {
	worker->exists = true;
	worker->pos = {x, y};
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