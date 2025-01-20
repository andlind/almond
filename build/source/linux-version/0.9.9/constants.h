#ifndef ALMOND_CONSTANTS_STRUCTURES_HEADER
#define ALMOND_CONSTANTS_STRUCTURES_HEADER

typedef void (*ConstantHandler)(void);

typedef struct Constant {
	const char* name;
	ConstantHandler handler;
} Constant;

#endif

