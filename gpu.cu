#include "gpu.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <cstdlib>
#include <iostream>

__device__ uint64_t getDiff(uint32_t* buf1, uint32_t* buf2, int pos) {
	int32_t red1 = (uint8_t)(buf1[pos] >> 16);
	int32_t green1 = (uint8_t)(buf1[pos] >> 8);
	int32_t blue1 = (uint8_t)(buf1[pos]);
	int32_t red2 = (uint8_t)(buf2[pos] >> 16);
	int32_t green2 = (uint8_t)(buf2[pos] >> 8);
	int32_t blue2 = (uint8_t)(buf2[pos]);

	int64_t redDiff = abs(red1 - red2);
	int64_t greenDiff = abs(green1 - green2);
	int64_t blueDiff = abs(blue1 - blue2);

	return  redDiff + greenDiff + blueDiff;
}

__global__ void compare_kernel(uint32_t* buf1, uint32_t* buf2, uint64_t* sum, int w, int h, uint64_t sampleGrid[100] = nullptr) {
	const int xPix = blockDim.x * blockIdx.x + threadIdx.x;
	const int yPix = blockDim.y * blockIdx.y + threadIdx.y;

	unsigned int pos = w * yPix + xPix;

	uint64_t diff = getDiff(buf1, buf2, pos);
	atomicAdd(sum, diff);
	if (sampleGrid != nullptr) {
		atomicAdd(&sampleGrid[xPix * 10 / w + 10 * (yPix * 10 / (h+1))], diff);
	}
}

uint64_t gpuCompare(uint32_t* input, uint32_t* output, int size, int w, int h, uint64_t sampleGrid[100])
{
	const dim3 blocksPerGrid((w / TILE_WIDTH), (h / TILE_HEIGHT));
	const dim3 threadsPerBlock(TILE_WIDTH, TILE_HEIGHT);

	uint64_t* sum;
	cudaMallocManaged((void**)&sum, sizeof(uint64_t));

	uint32_t* in = nullptr;
	uint32_t* out = nullptr;

	cudaMallocManaged((void**)&in, size);
	cudaMallocManaged((void**)&out, size);

	cudaMemcpy(in, input, size, cudaMemcpyHostToDevice);
	cudaMemcpy(out, output, size, cudaMemcpyHostToDevice);

	if (sampleGrid != nullptr) {
		uint64_t* grid = nullptr;
		int gridSize = sizeof(uint64_t) * 100;
		cudaMallocManaged((void**)&grid, gridSize);
		cudaMemcpy(grid, sampleGrid, gridSize, cudaMemcpyHostToDevice);

		compare_kernel <<<blocksPerGrid, threadsPerBlock>>> (in, out, sum, w, h, grid);

		cudaMemcpy(sampleGrid, grid, gridSize, cudaMemcpyDeviceToHost);
		cudaFree(grid);
	} else {
		compare_kernel <<<blocksPerGrid, threadsPerBlock>>> (in, out, sum, w, h);
	}

	cudaDeviceSynchronize();

	uint64_t result = *sum;

	cudaFree(sum);
	cudaFree(in);
	cudaFree(out);

	return result;
}

__global__ void difference_kernel(uint32_t* buf1, uint32_t* buf2, uint32_t* buf3, int w) {
	const int xPix = blockDim.x * blockIdx.x + threadIdx.x;
	const int yPix = blockDim.y * blockIdx.y + threadIdx.y;
	unsigned int pos = w * yPix + xPix;

	buf3[pos] = getDiff(buf1, buf2, pos);
}

void renderDifference(uint32_t* input, uint32_t* output, uint32_t* difference, int size, int w, int h)
{
	const dim3 blocksPerGrid((w / TILE_WIDTH), (h / TILE_HEIGHT));
	const dim3 threadsPerBlock(TILE_WIDTH, TILE_HEIGHT);

	uint32_t* in = nullptr;
	uint32_t* out = nullptr;
	uint32_t* diff = nullptr;

	cudaMallocManaged((void**)&in, size);
	cudaMallocManaged((void**)&out, size);
	cudaMallocManaged((void**)&diff, size);

	cudaMemcpy(in, input, size, cudaMemcpyHostToDevice);
	cudaMemcpy(out, output, size, cudaMemcpyHostToDevice);
	cudaMemcpy(diff, difference, size, cudaMemcpyHostToDevice);

	difference_kernel <<<blocksPerGrid, threadsPerBlock>>> (in, out, diff, w);

	cudaDeviceSynchronize();

	cudaMemcpy(difference, diff, size, cudaMemcpyDeviceToHost);

	cudaFree(in);
	cudaFree(out);
	cudaFree(diff);
}
