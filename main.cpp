// #include "game_engine.h"
// // #include "initMain.h"
// #include <fstream>
// #include <iostream>
// #include <string>

// #include <iomanip>
// #include <locale>
// // #include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
// #include <bitset>

#include "level1.h"
// #include "boostSerilize.h"
int main(int argc, char **argv)
{

	// boostSerialize(argc,argv);


	if (argc > 1)
		maxGameDuration = (float)stoi(argv[1]);

	uint uintBits = 1 << 31;
	bitset<32> bits{uintBits};
	cout << bits << endl;

	hideMouse = false;

	cout << sizeof(tbb::spin_mutex) << " : " << sizeof(tbb::mutex) << endl;

	bool load = false;
	cin >> load;
	::init();
	level1(load);
	run();

	// delete[] heightData;

	return 0;
}
