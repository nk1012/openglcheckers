#include <cmath>

#ifndef _MACROS_H_
#define _MACROS_H_
enum rgba { R = 0, G, B, A };

#ifndef DEG2RAD
#define DEG2RAD( theta ) ((theta) * (M_PI/180.0))
#endif

#ifndef RAD2DEG
#define RAD2DEG( theta ) ((theta) * (180.0/M_PI))
#endif

#ifndef SQUARE
#define SQUARE( x ) ((x) * (x))
#endif

#ifndef MAX
#define MAX( a, b ) a > b ? a : b
#endif

#ifndef MIN
#define MIN( a, b ) a < b ? a : b
#endif

#endif
