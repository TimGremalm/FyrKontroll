#ifndef FYR_SETTINGS_H
#define FYR_SETTINGS_H

#include <stdint.h>

typedef struct {
	float light;
	float speed;
	float offset;
	float sector_width;
} fyrsettings_t;

extern fyrsettings_t fyrsettings;

void LoadSettings();
void SaveSettings();

#endif /* FYR_SETTINGS_H */

