#pragma once

// job.cpp

Job* getFreeJob(JobBoard *board) {
	ASSERT((board->jobCount < ArrayCount(board->jobs)), "No room in JobBoard to add new job!");

	Job *job = board->jobs + board->jobCount++;
	*job = {};
	return job;
}

void addPlantJob(JobBoard *board, Coord fieldPosition) {
	Job *job = getFreeJob(board);
	job->type = JobType_Plant;
	job->data_Plant.fieldPosition = fieldPosition;
}

void addHarvestJob(JobBoard *board, Coord fieldPosition) {
	Job *job = getFreeJob(board);
	job->type = JobType_Harvest;
	job->data_Harvest.fieldPosition = fieldPosition;
}

void addStoreCropJob(JobBoard *board, Coord fieldPosition) {
	Job *job = getFreeJob(board);
	job->type = JobType_StoreCrop;
	job->data_StoreCrop.fieldPosition = fieldPosition;
}

bool workExists(JobBoard *board) {
	return board->jobCount > 0;
}

void takeJob(JobBoard *board, Worker *worker) {
	worker->job = board->jobs[0];
	worker->isAtDestination = false;

	// Move the last job to the top of the list, and decrease the job count, in one go!
	board->jobs[0] = board->jobs[--board->jobCount];
}