#pragma once

// job.cpp

void addPlantJob(JobBoard *board, Building *building) {
	ASSERT((board->jobCount < ArrayCount(board->jobs)), "No room in JobBoard to add new job!");

	Job *job = board->jobs + board->jobCount;
	job->type = JobType_Plant;
	job->building = building;

	board->jobCount++;
}

void addHarvestJob(JobBoard *board, Building *building) {
	ASSERT((board->jobCount < ArrayCount(board->jobs)), "No room in JobBoard to add new job!");

	Job *job = board->jobs + board->jobCount;
	job->type = JobType_Harvest;
	job->building = building;

	board->jobCount++;
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