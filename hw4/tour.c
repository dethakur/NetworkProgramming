#include "common.h"


int main(int argc, char **argv) {
	struct hwaddr hwa;
	//	areq("130.245.156.21", &hwa);
	areq(argv[1], &hwa);
	return 0;
}
