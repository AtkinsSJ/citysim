#pragma once

// job.cpp

Job* getFreeJob(JobBoard *board) {
	ASSERT((board->jobCount < ArrayCount(board->jobs)), "No room in JobBoard to add new job!");

	Job *job = board->jobs + board->jobCount++;
	*job = {};
	return job;
}

void addPlantJob(JobBoard *board, Building *building) {
	Job *job = getFreeJob(board);
	job->type = JobType_Plant;
	job->building = building;
}

void addHarvestJob(JobBoard *board, Building *building) {
	Job *job = getFreeJob(board);
	job->type = JobType_Harvest;
	job->building = building;
}

void addStoreCropJob(JobBoard *board, Potato *potato) {
	Job *job = getFreeJob(board);
	job->type = JobType_StoreCrop;
	job->potato = potato;
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