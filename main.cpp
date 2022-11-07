#include <SDL.h>
#include <stdio.h>
#include <iostream>

#include <string.h>
#include <stdlib.h>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <cmath>

#include "const.h"
#include "gpu.h"

#include "triangle.h"

void displayDifference(SDL_Surface* input, SDL_Surface* output, SDL_Surface* difference) {
	SDL_LockSurface(input);
	SDL_LockSurface(output);
	SDL_LockSurface(difference);

	int size = input->format->BytesPerPixel * input->w * input->h;
	renderDifference((uint32_t*)input->pixels, (uint32_t*)output->pixels, (uint32_t*)difference->pixels, size, input->w, input->h);

	SDL_UnlockSurface(input);
	SDL_UnlockSurface(output);
	SDL_UnlockSurface(difference);
}

int compare(SDL_Surface* input, SDL_Surface* output, uint64_t sampleGrid[100] = nullptr) {
	SDL_LockSurface(input);
	SDL_LockSurface(output);

	int size = input->format->BytesPerPixel * input->w * input->h;
	int result = gpuCompare((uint32_t*)input->pixels, (uint32_t*)output->pixels, size, input->w, input->h, sampleGrid);

	SDL_UnlockSurface(input);
	SDL_UnlockSurface(output);

	return result;
}

int scoreTriangles(SDL_Renderer* renderer, SDL_Surface* inputImg, SDL_Surface* outputImg, Triangle triangles[5000], int count = -1, uint64_t sampleGrid[100] = nullptr) {
	SDL_RenderClear(renderer);
	if (count != -1) {
		for (unsigned int i = 0; i <= count; i++) {
			SDL_RenderGeometry(renderer, NULL, triangles[i].vertices, 3, NULL, 0);
		}
	}
	SDL_RenderPresent(renderer);

	//current rendertarget texture to currentImg
	SDL_RenderReadPixels(renderer, NULL, 0, outputImg->pixels, outputImg->pitch);

	return compare(inputImg, outputImg, sampleGrid);
}

bool checkInput(SDL_Event &e) {
	if (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return true;
		}
	}
	return false;
}

int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Surface* inputImg = SDL_LoadBMP("input.bmp");
	int w = inputImg->w;
	int h = inputImg->h;
	SDL_Surface* outputImg = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
	//SDL_Surface* differenceImg = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);

	SDL_Window* window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
	//SDL_Window* debugWindow = SDL_CreateWindow("debug", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	//SDL_Renderer* debugRenderer = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET, w, h);

	SDL_SetRenderTarget(renderer, texture);

	Triangle triangles[5000];

	srand(time(0));

	bool quit = false;
	SDL_Event e;
	int startDiff = scoreTriangles(renderer, inputImg, outputImg, triangles);
	int currentDiff = startDiff;
	for (unsigned int i = 0; i < 5000 && !quit; i++) {
		uint64_t sampleGrid[100] = {};
		scoreTriangles(renderer, inputImg, outputImg, triangles, i-1, sampleGrid);
		uint64_t copyGrid[100];
		std::copy(std::begin(sampleGrid), std::end(sampleGrid), std::begin(copyGrid));
		std::sort(copyGrid, copyGrid + sizeof(copyGrid) / sizeof(copyGrid[0]));

		int copyPos = rand() % 100 < 75 ? 99 : rand() % 100;
		int pos = 0;
		for (unsigned int j = 0; j < 100; j++) {
			if (copyGrid[copyPos] == sampleGrid[j]) {
				pos = j;
				break;
			}
		}
		int areaX = w / 10 * (pos % 10);
		int areaY = h / 10 * (pos / 10);
		int areaW = w / 10;
		int areaH = h / 10;

		Triangle candidates[10];
		int size = ((float)currentDiff / (float)startDiff) * std::max(w, h);
		for (unsigned int j = 0; j < 10; j++) {
			candidates[j] = Triangle(w, h, inputImg, size, areaX, areaY, areaW, areaH);
			triangles[i] = candidates[j];
			candidates[j].score = scoreTriangles(renderer, inputImg, outputImg, triangles, i);
		}
		std::sort(candidates, candidates + sizeof(candidates) / sizeof(candidates[0]));

		Triangle generation[13];
		generation[0] = candidates[0];
		
		for (int rate = 150; rate > 0 && !quit; rate--) {
			if (checkInput(e)) {
				quit = true;
				break;
			}

			triangles[i] = generation[0];
			generation[0].score = scoreTriangles(renderer, inputImg, outputImg, triangles, i);
			for (unsigned int j = 1; j < 13 && !quit; j++) {
				if (checkInput(e)) {
					quit = true;
					break;
				}

				generation[j] = generation[0].shift(w, h, rate / 10 + 1, inputImg, j - 1);
				triangles[i] = generation[j];
				generation[j].score = scoreTriangles(renderer, inputImg, outputImg, triangles, i);
			}

			std::sort(generation, generation + sizeof(generation) / sizeof(generation[0]));
			if (generation[0].score == currentDiff) {
				rate -= 10;
			}
			currentDiff = generation[0].score;
		}

		triangles[i] = generation[0];
		triangles[i].score = scoreTriangles(renderer, inputImg, outputImg, triangles, i);
		//if the score of the image without the current triangle was better, remove it
		if (i > 0 && triangles[i].score >= triangles[i-1].score) {
			i--;
		} else {
			std::cout << "Triangles used: " << i + 1 << std::endl;
		}
		/*
		SDL_RenderClear(debugRenderer);
		displayDifference(inputImg, outputImg, differenceImg);

		SDL_Texture* temp = SDL_CreateTextureFromSurface(debugRenderer, differenceImg);
		SDL_RenderCopy(debugRenderer, temp, NULL, NULL);
		SDL_RenderPresent(debugRenderer);

		SDL_DestroyTexture(temp);
		*/
    }

	SDL_SaveBMP(outputImg, "output.bmp");

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	//SDL_DestroyRenderer(debugRenderer);
	SDL_DestroyWindow(window);
	//SDL_DestroyWindow(debugWindow);

    SDL_Quit();

  return 0;
}
