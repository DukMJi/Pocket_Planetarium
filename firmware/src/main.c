#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include "imu.h"
#include "stars.h"
#include "astro.h"

static void renderText(SDL_Renderer* ren, TTF_Font* font, const char* msg, int x, int y);
static void draw_horizon(SDL_Renderer *ren,
                         float rx, float ry, float rz,
                         float ux, float uy, float uz,
                         float fx, float fy, float fz,
                         int W, int H, float FOV);
static void draw_cardinals(SDL_Renderer *ren, TTF_Font *font,
                           float rx, float ry, float rz,
                           float ux, float uy, float uz,
                           float fx, float fy, float fz,
                           int W, int H, float FOV);
static int project_altaz(float alt_deg, float az_deg,
                         float rx, float ry, float rz,
                         float ux, float uy, float uz,
                         float fx, float fy, float fz,
                         int W, int H, float FOV,
                         int *outx, int *outy)
{
	float x, y, z;
	astro_altaz_to_unit(alt_deg, az_deg, &x, &y, &z);

	return astro_project_dir(x, y, z,
                            rx, ry, rz,
                            ux, uy, uz,
                            fx, fy, fz,
                            W, H, FOV,
                            outx, outy, NULL);
}

static void draw_horizon(SDL_Renderer *ren,
                         float rx, float ry, float rz,
                         float ux, float uy, float uz,
                         float fx, float fy, float fz,
                         int W, int H, float FOV)
{
	  // Draw horizon as a polyline by sampling azimuth around 0..360
    const float ALT = 0.0f;     // horizon
    const float STEP = 2.0f;    // degrees per segment (smoothness vs cost)

    int prev_ok = 0;
    int px0 = 0, py0 = 0;

    for (float az = 0.0f; az <= 360.0f; az += STEP)
    {
        int px, py;
        int ok = project_altaz(ALT, az,
                               rx, ry, rz,
                               ux, uy, uz,
                               fx, fy, fz,
                               W, H, FOV,
                               &px, &py);

        if (ok && prev_ok)
        {
            SDL_RenderDrawLine(ren, px0, py0, px, py);
        }

        prev_ok = ok;
        px0 = px;
        py0 = py;
    }
}

static void draw_cardinals(SDL_Renderer *ren, TTF_Font *font,
                           float rx, float ry, float rz,
                           float ux, float uy, float uz,
                           float fx, float fy, float fz,
                           int W, int H, float FOV)
{
    // Slightly above horizon so labels are visible
    const float ALT_LABEL = 5.0f;

    struct { const char *txt; float az; } marks[] = {
        {"N", 0.0f},
        {"E", 90.0f},
        {"S", 180.0f},
        {"W", 270.0f}
    };

    for (size_t i = 0; i < sizeof(marks)/sizeof(marks[0]); i++)
    {
        int x, y;
        if (project_altaz(ALT_LABEL, marks[i].az,
                          rx, ry, rz,
                          ux, uy, uz,
                          fx, fy, fz,
                          W, H, FOV,
                          &x, &y))
        {
            renderText(ren, font, marks[i].txt, x - 8, y - 8);
        }
    }

    // Optional: Zenith marker ("UP") at alt=90 (always useful)
    {
        int x, y;
        if (project_altaz(90.0f, 0.0f,
                          rx, ry, rz,
                          ux, uy, uz,
                          fx, fy, fz,
                          W, H, FOV,
                          &x, &y))
        {
            renderText(ren, font, "UP", x - 16, y - 12);
        }
    }
}

static void unit_to_altaz(float x, float y, float z, float *alt_deg, float *az_deg)
{
    // Input is ENU: x=East, y=North, z=Up
    float alt = asinf(z);				// radians
    float az  = atan2f(x, y);			// radians (east, north)
    if (az < 0) az += 2.0f * (float)M_PI;

    if (alt_deg) *alt_deg = alt * 180.0f / (float)M_PI;
    if (az_deg)  *az_deg  = az  * 180.0f / (float)M_PI;
}

static double get_jd_utc_now(void)
{
	time_t t = time(NULL);
	struct tm g;
#if defined(_WIN32)
	gmtime_s(&g, &t);
#else
	g = *gmtime(&t);
#endif
	return astro_julian_date_utc(g.tm_year + 1900, g.tm_mon + 1, g.tm_mday,
								g.tm_hour, g.tm_min, (double)g.tm_sec);
}

static int mag_to_radius(float mag)
{
	if (mag <= 1.0f)	// very bright
	{
		return 3;
	}

	if (mag <= 2.5f)	// bright
	{
		return 2;
	}

	if (mag <= 4.0f)	// medium
	{
		return 1;
	}

	return 0;			// faint
}

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

	// Load star catalog
	star_catalog_t catalog;
	if (stars_load_csv(&catalog, "firmware/assets/stars.csv") != 0)
	{
		fprintf(stderr, "Failed to load stars CSV\n");
		TTF_CloseFont(font);
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	printf("Loaded %zu stars\n", catalog.count);

	float *cache_lx = (float*)malloc(sizeof(float) * catalog.count);
	float *cache_ly = (float*)malloc(sizeof(float) * catalog.count);
	float *cache_lz = (float*)malloc(sizeof(float) * catalog.count);
	unsigned char *cache_vis = (unsigned char*)malloc(sizeof(unsigned char) * catalog.count);
	unsigned char *cache_rad = (unsigned char*)malloc(sizeof(unsigned char) * catalog.count);

	if (!cache_lx || !cache_ly || !cache_lz || !cache_vis || !cache_rad)
	{
		fprintf(stderr, "Failed to allocate star cache arrays\n");

		free(cache_lx);
		free(cache_ly);
		free(cache_lz);
		free(cache_vis);
		free(cache_rad);

		stars_free(&catalog);
		TTF_CloseFont(font);
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(w);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	float mag_cutoff = 5.5f;
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

	imu_data_t imu;

	const int W = 800;
	const int H = 480;
	const float FOV = 70.0f;
	const double LAT_DEG = 32.7357;
	const double LON_DEG = -97.1081;

	while (running)	// Main application loop
	{
		int best_i = -1;
		int best_dist2 = INT_MAX;
		const int cx = W / 2;
		const int cy = H / 2;
		const int pick_radius_px = 35;
		const int pick_radius2 = pick_radius_px * pick_radius_px;
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

		float rx, ry, rz, ux, uy, uz, fx, fy, fz;
		astro_camera_basis(yaw, pitch, roll,
						&rx, &ry, &rz,
						&ux, &uy, &uz,
						&fx, &fy, &fz);

		float aim_alt, aim_az;
		unit_to_altaz(fx, fy, fz, &aim_alt, &aim_az);

		static Uint32 lastCacheMs = 0;
		static double jd = 0;

		if (jd == 0 || now - lastCacheMs > 1000)
		{
			jd = get_jd_utc_now();
			lastCacheMs = now;

			// TODO: replace with GPS later
			for (size_t i = 0; i < catalog.count; i++)
			{
				float mag = catalog.items[i].mag;

				// dim stars early
				if (mag > mag_cutoff)
				{
					cache_vis[i] = 0;
					continue;
				}

				float alt_deg, az_deg;
				astro_radec_to_altaz(catalog.items[i].ra_hours,
									catalog.items[i].dec_deg,
									jd, LAT_DEG, LON_DEG,
									&alt_deg, &az_deg);

				if (alt_deg < 0.0f)
				{
					cache_vis[i] = 0;
					continue;
				}

				astro_altaz_to_unit(alt_deg, az_deg, &cache_lx[i], &cache_ly[i], &cache_lz[i]);
				cache_vis[i] = 1;
				cache_rad[i] = (unsigned char)mag_to_radius(mag);
			}
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

		SDL_SetRenderDrawColor(ren, 120, 120, 120, 255);
		draw_horizon(ren, rx, ry, rz, ux, uy, uz, fx, fy, fz, W, H, FOV);

		SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
		draw_cardinals(ren, font, rx, ry, rz, ux, uy, uz, fx, fy, fz, W, H, FOV);
		SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
		
		for (size_t i = 0; i < catalog.count; i++)
		{
			if (!cache_vis[i])
			{
				continue;
			}

			int px, py;

			if (!astro_project_dir(cache_lx[i], cache_ly[i], cache_lz[i],
						rx, ry, rz,
						ux, uy, uz,
						fx, fy, fz,
						W, H, FOV,
						&px, &py, NULL))
			{
				continue;
			}

			int dx = px - cx;
			int dy = py - cy;
			int dist2 = dx*dx + dy*dy;

			if (dist2 < best_dist2)
			{
				best_dist2 = dist2;
				best_i = (int)i;
			}

			int r = (int)cache_rad[i];
			if (r <= 0)
			{
				SDL_RenderDrawPoint(ren, px, py);
			}
			else
			{
				for (int dy = -r; dy <= r; dy++)
				{
					for (int dx = -r; dx <= r; dx++)
					{
						SDL_RenderDrawPoint(ren, px + dx, py + dy);
					}
				}
			}
		}

		if (best_i >= 0 && best_dist2 <= pick_radius2)
		{
			char starbuf[128];
			snprintf(starbuf, sizeof(starbuf), "Target: %s  Mag: %.1f",
					catalog.items[best_i].name,
					catalog.items[best_i].mag);
			renderText(ren, font, starbuf, 20, 50);
		}
		else
		{
			renderText(ren, font, "Target: (none)", 20, 50);
		}

		// Crosshair centered on screen.
		SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
		SDL_RenderDrawLine(ren, 400 - 40, 240, 400 + 40, 240);
		SDL_RenderDrawLine(ren, 400, 240 - 40, 400, 240 + 40);

		// Diagnostic overlay.
		char buf[128];
		snprintf(buf, sizeof(buf),
        		"Yaw: %.1f  Pitch: %.1f  Roll: %.1f Aim Alt: %.1f Aim Az: %.1f FPS: %.1f",
         		yaw, pitch, roll, aim_alt, aim_az, fps);

		renderText(ren, font, buf, 20, 20);

		SDL_RenderPresent(ren);
		SDL_Delay(1); // Delay to avoid maxing out CPU.
	}

	// Cleanup resources.
	free(cache_lx);
	free(cache_ly);
	free(cache_lz);
	free(cache_vis);
	free(cache_rad);

	stars_free(&catalog);
	imu_close();

	TTF_CloseFont(font);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(w);
	TTF_Quit();
	SDL_Quit();

	return 0;
}