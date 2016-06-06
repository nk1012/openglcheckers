#ifndef _CHECKERS_H_
#define _CHECKERS_H_

#include <vector>
#include "checker.h"
#include "tree_element.h"

#define SIZE 8

int board_position_from_coord_x(int);
int board_position_from_coord_y(int);
bool check_in_radius(int,int);
void construct_tree_from_board(Checker[SIZE][SIZE]);
void copy_board(Checker[SIZE][SIZE],Checker[SIZE][SIZE]);
void disable_animation(void);
void display(void);
void draw_pieces(Checker[SIZE][SIZE]);
void draw_board(void);
void enable_animation(int,int,int,int,float=0.5f);
void initialize_board(void);
void insert_into_tree(int,std::vector<TreeElement>&,TreeElement*,Checker[SIZE][SIZE],bool,int,int,int,int);
void make_best_move(void);
bool move_if_valid(Checker[SIZE][SIZE],int,int,int,int,bool=false);
void keyboard_action(unsigned char,int,int);
int main(int,char**);
void mouse_action(int button,int state,int,int);
void mouse_action_listener(int,int);
void print_checker(Checker[SIZE][SIZE],int,int,char* m="");
void reset_game_state(void);
void resize_window(int,int);
bool someone_won(Checker[SIZE][SIZE]);
bool this_colors_turn(checker_type);
char* verbose_type(Checker);

#endif
