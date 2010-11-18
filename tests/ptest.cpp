#include <mpkg/libmpkg.h>

int main() {
	mpkg core;
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	printf("Getting\n");
	time_t a = time(NULL);
	core.get_packagelist(sqlSearch, &pkgList);
	time_t b = time(NULL);
	printf("FULLGET: %d\n", b-a);
	core.db->get_full_filelist(&pkgList);
	time_t c = time(NULL);
	printf("FILE: %d\n", c-b);

}
