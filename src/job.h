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
	Building *field;
};
struct JobData_Harvest
{
	Building *field;
};
struct JobData_StoreCrop
{
	Potato *potato;
	Building *barn;
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