#pragma once

#include "random_mt.h"

typedef RandomMT Random;
#define randomSeed(...) MT_randomSeed(__VA_ARGS__)
#define randomNext(...) MT_randomNext(__VA_ARGS__)