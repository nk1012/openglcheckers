#ifndef _TREE_ELEMENT_H_
#define _TREE_ELEMENT_H_

#include "checker.h"

class TreeElement
{
	public:
		bool was_jump;
		Checker board[SIZE][SIZE];
		TreeElement *parent;
		TreeElement(Checker[SIZE][SIZE],TreeElement*,bool=false,int=-1,int=-1,int=-1,int=-1);
		void set_last_move(int,int);
		int path_utility, node_utility, from_x, from_y, to_x, to_y;
};

#endif
