// mpkgsupport: unit tests

#include "mpkgsupport.h"

int main() {
	// Test string compare function
	// 1: svn21627 vs svn21970, should return -1
	int result;
	result = compareVersions("svn21267", "1", "svn21870", "1");
	if (result==-1) printf("Test 1: %sPASSED%s\n", CL_GREEN, CL_WHITE);
	else printf("Test 1: %sFAILED%s, result: %d\n", CL_RED, CL_WHITE, result);

	result = compareVersions("2.4.21", "1", "2.4.21", "2");
	if (result==-1) printf("Test 2: %sPASSED%s\n", CL_GREEN, CL_WHITE);
	else printf("Test 2: %sFAILED%s, result: %d\n", CL_RED, CL_WHITE, result);

	result = compareVersions("sad", "1", "tag", "1");
	if (result==-1) printf("Test 3: %sPASSED%s\n", CL_GREEN, CL_WHITE);
	else printf("Test 3: %sFAILED%s, result: %d\n", CL_RED, CL_WHITE, result);

	result = compareVersions("0.3.0_git20091230", "1", "0.3", "1");
	if (result==-1) printf("Test 4: %sPASSED%s\n", CL_GREEN, CL_WHITE);
	else printf("Test 4: %sFAILED%s, result: %d\n", CL_RED, CL_WHITE, result);



	int i1 = atoi("svn21627");
	int i2 = atoi("svn21870");
	printf("i1 = %d, i2 = %d\n", i1, i2);



	return 0;
	
}
