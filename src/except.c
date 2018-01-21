#include <stdlib.h>

void except(const char* message) {
	if(message != NULL) {
		perror(message);
	}
	exit(EXIT_FAILURE);
}

// void except_catch(void* ret, const char* message, void (*f)(void* result)) {
// 	if(message != NULL) {
// 		perror(message);
// 	}
// 	f(ret);
// }