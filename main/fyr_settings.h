#ifndef FYR_SETTINGS_H
#define FYR_SETTINGS_H

#include <stdint.h>

typedef struct {
	int32_t light;
	int32_t speed;
	int32_t offset;
	int32_t sector_width;
} fyrsettings_t;

extern fyrsettings_t fyrsettings;

void LoadSettings();
void SaveSettings();

#endif /* FYR_SETTINGS_H */

