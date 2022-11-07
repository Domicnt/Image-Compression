#pragma once

#include <stdint.h>

#include "const.h"

uint64_t gpuCompare(uint32_t* input, uint32_t* output, int size, int w, int h, uint64_t sampleGrid[100] = nullptr);

void renderDifference(uint32_t* input, uint32_t* output, uint32_t* difference, int size, int w, int h);