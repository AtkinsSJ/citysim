#pragma once

// job.cpp

void addPlantJob(JobBoard *board, Building *building) {
	ASSERT((board->jobCount < ArrayCount(board->jobs)), "No room in JobBoard to add new job!");

	Job *job = board->jobs + board->jobCount;
	job->type = JobType_Plant;
	job->building = building;

	board->jobCount++;
} 