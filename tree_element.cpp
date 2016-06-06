#include "tree_element.h"
#include "checker.h"

#ifndef NULL
#define NULL 0
#endif

// конструктор

TreeElement::TreeElement(Checker temp_board[SIZE][SIZE], TreeElement *_parent, bool jump, int f_x, int f_y, int t_x, int t_y) : was_jump(jump), parent(_parent), from_x(f_x), from_y(f_y), to_x(t_x), to_y(t_y)
{
	node_utility = 0;
	checker_type type;
	path_utility = parent != NULL ? parent->path_utility : 0;

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			type = temp_board[i][j].type;
			board[i][j].type = type;
			if (type != INVALID) node_utility += type + (int)((i == 0 || i == SIZE-1 || j == 0 || j == SIZE-1) * (type < 0 ? -1 : 1));
		}
	}

	path_utility += node_utility;
}
