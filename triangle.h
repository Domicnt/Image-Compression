#pragma once

#include <stdlib.h>
#include <SDL.h>
#include <algorithm>

Uint32 getpixel(SDL_Surface* surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;

	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
	case 1:
		return *p;
		break;

	case 2:
		return *(Uint16*)p;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;

	case 4:
		return *(Uint32*)p;
		break;

	default:
		return 0;
	}
}

class Triangle {
public:
	SDL_Vertex vertices[3];
	int score;

	Triangle() = default;

	Triangle(int w, int h, SDL_Surface* image, int size = 0, int areaX = 0, int areaY = 0, int areaW = 0, int areaH = 0) {
		if (size == 0) size = std::max(w, h);
		if (areaX == 0 && areaY == 0 && areaW == 0 && areaH == 0) {
			areaX = w * -.05f;
			areaY = h * -.05f;
			areaW = w * 1.1f;
			areaH = h * 1.1f;
		}

		float x = areaX + rand() % areaW;
		float y = areaY + rand() % areaH;

		SDL_Vertex vertices[3];
		for (auto& i : vertices) {
			i.position.x = x + rand() % size - size / 2;
			i.position.y = y + rand() % size - size / 2;
		}
		
		Uint32 data = getpixel(image, std::max(std::min(x, (float)w), 0.0f), std::max(std::min(y, (float)h), 0.0f));

		for (auto& i : vertices) {
			SDL_GetRGB(data, image->format, &i.color.r, &i.color.g, &i.color.b);
		}

		for (unsigned int i = 0; i < 3; i++) {
			this->vertices[i] = vertices[i];
		}
	}

	Triangle(SDL_Vertex vertices[3]) {
		for (unsigned int i = 0; i < 3; i++) {
			this->vertices[i] = vertices[i];
		}
	}

	Triangle shift(int w, int h, int rate, SDL_Surface* image, int iteration) {
		SDL_Vertex newVertices[3];
		for (unsigned int i = 0; i < 3; i++) {
			newVertices[i] = vertices[i];
		}

		int vertex = iteration / 4;
		
		if (iteration % 4 < 2) {
			newVertices[vertex].position.x = std::max(std::min(newVertices[vertex].position.x + ((iteration == 1) ? rate : -rate), (float)w + w / 4.0f), - w / 4.0f);
		} else {
			newVertices[vertex].position.y = std::max(std::min(newVertices[vertex].position.y + ((iteration == 3) ? rate : -rate), (float)h + h / 4.0f), - h / 4.0f);
		}

		int x = 0;
		int y = 0;
		for (unsigned int i = 0; i < 3; i++) {
			x += newVertices[i].position.x;
			y += newVertices[i].position.y;
		}
		
		Uint32 data = getpixel(image, std::max(std::min((float)x / 3, (float)w), 0.0f), std::max(std::min((float)y / 3, (float)h), 0.0f));

		for (auto& i : newVertices) {
			SDL_GetRGB(data, image->format, &i.color.r, &i.color.g, &i.color.b);
		}
		

		return Triangle(newVertices);
	}
};

bool operator < (const Triangle& a, const Triangle& b) {
	return a.score < b.score;
}

bool operator > (const Triangle& a, const Triangle& b) {
	return a.score > b.score;
}