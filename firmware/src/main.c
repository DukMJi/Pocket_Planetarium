#include <stdio.h>
#include <SDL.h>

int main(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr,"SDL_InitL %s\n",SDL_GetError());
		return 1;
	}

	SDL_Window* w = SDL_CreateWindow("Pocket Planetarium", SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		800, 480, 0);

	if (!w)
	{
		fprintf(stderr,"CreateWindow; %s\n",SDL_GetError());
		SDL_Quit();
		return 1;
	}

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
		SDL_Delay(16);
	}
	SDL_DestroyWindow(w);
	SDL_Quit();

	return 0;
}