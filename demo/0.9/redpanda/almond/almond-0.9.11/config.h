#ifndef ALMOND_CONFIG_STRUCTURES_HEADER
#define ALMOND_CONFIG_STRUCTURES_HEADER

typedef struct {
	int intval;
	char* strval;
} ConfVal;

typedef struct ConfigEntry {
	const char* name;
	void (*process)(ConfVal value);
} ConfigEntry;

#endif

