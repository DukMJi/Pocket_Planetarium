#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "imu.h"

/*
 * Helper Function to render ASCII text to SDL renderer using SDL_tff.
 * Converts a string to a surface, then to a texture, and finally
 * copies it to the renderer at specified screen position.
 * 
 * Also kept separate from main render loop to avoid clutter.
 */
static void renderText(SDL_Renderer* ren, TTF_Font* font, const char* msg, int x, int y)
{
	SDL_Color white = {255,255,255,255};

	// Renders text into intermediate surface.
	SDL_Surface* surf = TTF_RenderText_Solid(font, msg, white);
	if (!surf) return;

	// Converts surface to GPU texture.
	SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
	if (tex)
	{
		SDL_Rect dst = {x, y, surf->w, surf->h};
		SDL_RenderCopy(ren, tex, NULL, &dst);
		SDL_DestroyTexture(tex);
	}
	SDL_FreeSurface(surf);
}

/*
 * Attemps to open a font from several known locations.
 * This allows the same binary to work across macOS and Linux
 * without hard-coding a single font path.
 */
static TTF_Font* open_font(int ptsize)
{
	const char* candidates[] = {
		"fonts/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/freefont/FreeSans.ttf",
		"/System/Library/Fonts/Supplemental/Menlo.ttc",
		"/System/Library/Fonts/Supplemental/Courier New.ttf"
	};

	for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i)
	{
		TTF_Font* f = TTF_OpenFont(candidates[i], ptsize);
		if (f)
		{
			printf("Loaded font: %s\n", candidates[i]);
			return f;
		}
	}
	return NULL;
}

int main(void)
{
	// Initialize SDL video subsystem.
	// This sets up graphic drivers and windowing.
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr,"SDL_Init %s\n",SDL_GetError());
		return 1;
	}

	// Initiailize SDL_tff for TrueType font rendering.
	if (TTF_Init() != 0)
	{
		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
		SDL_Quit();
		return 1;
	}

	// Create the main application window.
	// Resolution matches the target handheld display (800x480).
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

	// Creates a hardware-accelerated renderer for drawing.
	SDL_Renderer* ren = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
	if (!ren)
	{
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	// Load a font for on-screen diagnostics (angles, FPS).
	TTF_Font* font = open_font(24);

	if (!font)
	{
		fprintf(stderr, "No usable font found! %s\n", TTF_GetError());
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	// Tries to initialize the IMU once.
	// If it fails, fall back to SIM mode automatically.
	int imu_ok = (imu_init() == 0);

	// Press 'S' to force SIM mode even if IMU works (for demo/testing)).
	int force_sim = 0;

	// Orientation values come from IMU when available.
	float yaw = 0.f, pitch = 0.f, roll = 0.f;

	// Timing reference for frame delta calculation.
	Uint32 last = SDL_GetTicks();

	// FPS tracking variables (.5s)
	float fps = 0.f;
	Uint32 fpsLast = last;
	int frames = 0;

	SDL_Event e; // Event object (keyboard, quit, etc)
	int running = 1;

	while (running)	// Main application loop
	{
		// Handles user input and window events.
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT) // window close
			{
				running = 0; 
			}
			// ESC
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
			{
				running = 0;
			}

			// Toggle SIM mode with 'S'
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s)
			{
				force_sim = !force_sim;
			}
		}

		// Calculate frame delta time (seconds).
		Uint32 now = SDL_GetTicks();
		float dt = (now - last) / 1000.0f;
		last = now;

		imu_data_t imu;

		// Attempt to read from IMU.
		if (!force_sim && imu_ok && imu_read(&imu) == 0)
		{
			yaw = imu.yaw;
			pitch = imu.pitch;
			roll = imu.roll;
		}
		else
		{
			// SIM fallback
			imu_sim_step(&imu, dt);
			yaw = imu.yaw;
			pitch = imu.pitch;
			roll = imu.roll;
		}

		// FPS calculated
		frames++;
		if (now - fpsLast >= 500)
		{
			fps = frames * 1000.0f / (now - fpsLast);
			frames = 0;
			fpsLast = now;
		}

		// Rendering phase.
		SDL_SetRenderDrawColor(ren, 10, 10, 40, 255);
		SDL_RenderClear(ren);

		// Crosshair centered on screen.
		SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
		SDL_RenderDrawLine(ren, 400 - 40, 240, 400 + 40, 240);
		SDL_RenderDrawLine(ren, 400, 240 - 40, 400, 240 + 40);

		// Diagnostic overlay.
		char buf[128];
		snprintf(buf, sizeof(buf),
        		"Yaw: %.1f  Pitch: %.1f  Roll: %.1f  FPS: %.1f  MODE:%s",
         		yaw, pitch, roll, fps, (force_sim || !imu_ok) ? "SIM" : "IMU");

		renderText(ren, font, buf, 20, 20);

		SDL_RenderPresent(ren);
		SDL_Delay(1); // Delay to avoid maxing out CPU.
	}

	// Cleanup resources.
	TTF_CloseFont(font);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(w);
	TTF_Quit();
	SDL_Quit();

	imu_close();
	return 0;
}