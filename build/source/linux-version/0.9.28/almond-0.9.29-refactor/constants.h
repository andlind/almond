#ifndef ALMOND_CONSTANTS_STRUCTURES_HEADER
#define ALMOND_CONSTANTS_STRUCTURES_HEADER

#define MAX_HOSTS 256

//typedef void (*ConstantHandler)(void);

typedef struct Constant {
	int id;
	const char* name;
	size_t value;
	//ConstantHandler handler;
} Constant;

#endif

