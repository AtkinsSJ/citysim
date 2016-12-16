#pragma once

#include "random_mt.h"

typedef RandomMT Random;
#define randomSeed(...) MT_randomSeed(...)
#define randomNext(...) MT_randomNext(...)