#ifndef COMPOSITE_SETUP_H_
#define COMPOSITE_SETUP_H_

#define CARD_INTEL 0
#define CARD_NVIDIA 1
#define CARD_ATI 2
#define CARD_GENERIC 3
#define VI(m, q, j) conf.insert(conf.begin()+m, q + string(j))
#include <string>
using namespace std;
void enableComposite(string filename, bool disable=false);
void enableSynaptics(string filename);

#endif //COMPOSITE_SETUP_H_
