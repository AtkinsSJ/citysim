#pragma once

// job.h

struct Building;
struct Potato;
enum JobType
{
	JobType_Idle = 0,
	JobType_Plant = 1,
	JobType_Harvest,
	JobType_StoreCrop,

	JobTypeCount
};

struct JobData_Plant
{
	Coord fieldPosition;
};
struct JobData_Harvest
{
	Coord fieldPosition;
};
struct JobData_StoreCrop
{
	Coord fieldPosition;
	Coord barnPosition;
};

struct Job
{
	JobType type;
	union
	{
		JobData_Plant data_Plant;
		JobData_Harvest data_Harvest;
		JobData_StoreCrop data_StoreCrop;
	};
};
struct JobBoard
{
	Job jobs[128];
	int32 jobCount;
};