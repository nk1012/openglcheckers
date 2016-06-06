#ifndef _CHECKER_H_
#define _CHECKER_H_

#define SIZE 8

enum checker_type { RED_CHECKER = -2, BLACK_CHECKER = 2, RED_KING = -10, BLACK_KING = 10, EMPTY = 0, INVALID };

class Checker
{
	public:
		checker_type type;
		Checker(checker_type=EMPTY);
};

#endif
