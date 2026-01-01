#ifndef STARS_H
#define STARS_H

#include <stddef.h>

typedef struct
{
    char name[32];
    float ra_hours;
    float dec_deg;
    float mag;
} star_t;

typedef struct
{
    star_t *items;
    size_t count;
} star_catalog_t;

int stars_load_csv(star_catalog_t *cat, const char *path);
void stars_free(star_catalog_t *cat);

#endif