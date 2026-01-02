#include "stars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_comment_or_blank(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return (*s == '\0' || *s == '\n' || *s == '#');
}

int stars_load_csv(star_catalog_t *cat, const char *path)
{
    if (!cat || !path)
    {
        return -1;
    }

    cat->items = NULL;
    cat->count = 0;

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        perror("fopen stars csv");
        return -1;
    }

    size_t cap = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp))
    {
        if (is_comment_or_blank(line)) continue;

        // name, ra, dec, mag
        char name[64];
        float ra = 0.f, dec = 0.f, mag = 0.f;

        // crude CSV
        if (sscanf(line, " %63[^, ],%f,%f,%f", name, &ra, &dec, &mag) != 4)
        {
            continue;
        }

        if (cat->count >= cap)
        {
            size_t newcap = (cap == 0) ? 32 : cap * 2;
            star_t *p = (star_t*)realloc(cat->items, newcap * sizeof(star_t));
            if (!p)
            {
                fclose(fp);
                stars_free(cat);
                return -1;
            }

            cat->items = p;
            cap = newcap;
        }

        star_t *s = &cat->items[cat->count++];
        memset(s, 0, sizeof(*s));
        strncpy(s->name, name, sizeof(s->name) - 1);
        s->ra_hours = ra;
        s->dec_deg = dec;
        s->mag = mag;
    }

    fclose(fp);
    return 0;
}

void stars_free(star_catalog_t *cat)
{
    if (!cat)
    {
        return;
    }
    free(cat->items);
    cat->items = NULL;
    cat->count = 0;
}