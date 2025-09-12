#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>

static void renderText(SDL_Renderer* ren, TTF_Font* font, const char* msg, int x, int y)
{
	SDL_Color white = {255,255,255,255};
	SDL_Surface* surf = TTF_RenderText_Solid(font, msg, white);
	if (!surf) return;
	SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
	if (tex)
	{
		SDL_Rect dst = {x, y, surf->w, surf->h};
		SDL_RenderCopy(ren, tex, NULL, &dst);
		SDL_DestroyTexture(tex);
	}
	SDL_FreeSurface(surf);
}

int main(void)
{


	// For SDL video
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr,"SDL_Init %s\n",SDL_GetError());
		return 1;
	}

	// SDL_ttf for text
	if (TTF_Init() != 0)
	{
		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Window* w = SDL_CreateWindow("Pocket Planetarium", SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		800, 480, 0);

	if (!w)
	{
		fprintf(stderr,"CreateWindow; %s\n",SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* ren = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
	if (!ren)
	{
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	TTF_Font* font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Courier New.ttf", 24);
	if (!font)
	{
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	// Simulated IMu state
	float yaw=0.f, pitch=0.f, roll=0.f;
	Uint32 last = SDL_GetTicks();

	// FPS
	float fps = 0.f; Uint32 fpsLast = last; int frames=0;

	SDL_Event e;
	int running = 1;

	while (running)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				running = 0;
			}

			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
			{
				running = 0;
			}
		}

		Uint32 now = SDL_GetTicks();
		float dt = (now - last) / 1000.0f; //seconds
		last = now;

		// Animate fake IMU
		yaw += 30.f * dt;
		pitch += 20.f * dt;
		roll += 15.f * dt;
		if (yaw>360) yaw-=360;
		if (pitch>360) pitch-=360;
		if (roll>360) roll-=360;

		// FPS calculated
		frames++;
		if (now - fpsLast >= 500)
		{
			fps = frames * 1000.0f / (now - fpsLast);
			frames = 0;
			fpsLast = now;
		}

		// background
		SDL_SetRenderDrawColor(ren, 10, 10, 40, 255);
		SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
		SDL_RenderDrawLine(ren, 400 - 40, 240, 400 + 40, 240);
		SDL_RenderDrawLine(ren, 400, 240 - 40, 400, 240 + 40);

		char buf[128];
		snprintf(buf, sizeof(buf), "Yaw: %.1f  Pitch: %.1f  Roll: %.1f  FPS:%.1f", yaw, pitch, roll, fps);
		renderText(ren, font, buf, 20, 20);

		SDL_RenderPresent(ren);
		SDL_Delay(1); 
	}

	TTF_CloseFont(font);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(w);
	TTF_Quit();
	SDL_Quit();

	return 0;
}