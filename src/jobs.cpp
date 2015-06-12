#pragma once

// jobs.cpp

bool addPlantJob(JobBoard *board, Building *building) {
	if (board->jobCount >= ArrayCount(board->jobs)) {
		ASSERT(false);
		return false;
	}
} 