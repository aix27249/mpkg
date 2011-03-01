#ifndef ACUSTOMPKGSET_H__
#define ACUSTOMPKGSET_H__
#include <stdint.h>
struct CustomPkgSet {
	string name, desc, full, hw;
	uint64_t csize, isize;
	size_t count;
	bool hasX11;
	bool hasDM;
};

#endif
