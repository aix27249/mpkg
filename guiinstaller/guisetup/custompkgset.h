#ifndef ACUSTOMPKGSET_H__
#define ACUSTOMPKGSET_H__
#include <stdint.h>
struct CustomPkgSet {
	string name, desc, full;
	uint64_t csize, isize;
	size_t count;
};

#endif
