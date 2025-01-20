#ifndef ALMOND_CONSTANTS_STRUCTURES_HEADER
#define ALMOND_CONSTANTS_STRUCTURES_HEADER

//typedef void (*ConstantHandler)(void);

typedef struct Constant {
	int id;
	const char* name;
	size_t value;
	//ConstantHandler handler;
} Constant;

#endif

