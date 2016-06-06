#include <iostream>
#include <cstdio>
#include <vector>			// для дерева
#include <random>
#include <time.h>
#include <GL/freeglut.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif


#include "checkers.h"
#include "macro_definitions.h"
#include "checker.h"
#include "tree_element.h"


// текущая доска (game_board)

Checker	temp, *to_piece, *from_piece, game_board[SIZE][SIZE], temp_board[SIZE][SIZE];

// minimax игровое дерево

std::vector<TreeElement> game_tree[SIZE+2];
std::vector<TreeElement> double_jump_vector;
std::vector<int> best_elements;

TreeElement *delete_pointer;
TreeElement temp_element(game_board,NULL);
TreeElement double_jump_parent(game_board,NULL);

bool debug, is_playable, is_reds_turn, is_first_node, pruning_enabled, piece_being_dragged, piece_being_animated;

int row, col, old_mouse_x, old_mouse_y, pruned_count, drag_offset_x, drag_offset_y, animated_to_x, animated_to_y, animated_from_x, animated_from_y, board_position_x, board_position_y, comparison_utility, level_max_or_min[SIZE+2], global_window_width = 600 * SIZE / 8, global_window_height = 600 * SIZE / 8;

#ifdef _WIN32
LARGE_INTEGER current_time, timer_resolution, animation_start;
#else
timeval current_time, animation_started;
#endif

static int MAX_SEARCH_DEPTH = 5;

float piece_radius = 0.05f / SIZE * 8, accent_width = 0.003f / SIZE * 8, inner_position_x, inner_position_y, animation_duration, move_complete_ratio;

// для круга

GLUquadricObj *circle_drawer;


// цвета шашек, квадратов, кружков

float global_brown_color[] = { 0.45f, 0.24f, 0.1f, 1.0f };
float global_light_brown_color[] = { 0.87f, 0.72f, 0.53f, 1.0f };

//float global_white_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

float global_red_color[] = { 0.9f, 0.0f, 0.0f, 1.0f };
float global_red_accent_color[] = { 1.0f, 0.3f, 0.0f, 1.0f };

float global_black_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
float global_black_accent_color[] = { 0.25f, 0.25f, 0.25f, 1.0f };





//============================================================================
//============================================================================
//============================================================================
//============================================================================
// ==========================START============================================
//============================================================================
//============================================================================
//============================================================================
//============================================================================





int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(global_window_width, global_window_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);

	initialize_board();
	reset_game_state();

	glutDisplayFunc(display);
	glutReshapeFunc(resize_window);
	glutKeyboardFunc(keyboard_action);
	glutMotionFunc(mouse_action_listener);
	glutMouseFunc(mouse_action);
	glutMainLoop();

	return 0;
}
// рисует на экране

void display(void)
{
//	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	draw_board();
	draw_pieces(game_board);

	glFlush();
	glutSwapBuffers();
	if (piece_being_animated) glutPostRedisplay();
}

// инициализировать все, чтобы нарисовать доску

void initialize_board(void)
{
//	glClearColor( global_white_color[R], global_white_color[G], global_white_color[B], global_white_color[A] );
	circle_drawer = gluNewQuadric();
	gluQuadricDrawStyle(circle_drawer,GLU_FILL);

	debug = false;
	pruning_enabled = true;

	piece_being_dragged = false;
	piece_being_animated = false;

#ifdef _WIN32
	QueryPerformanceFrequency(&timer_resolution);
#endif

	glLineWidth(3);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_DEPTH_TEST);
}

// сбросить доску к стандартным позициям для шашек
// также для инициализации

void reset_game_state(void)
{
	is_playable = true;
	is_reds_turn = true;

	for(row = 0; row < SIZE; ++row)
	{
		for(col = 0; col < SIZE; ++col)
		{
			if(col != 0) is_playable = !is_playable;

			if(is_playable)
			{
				if(row < SIZE/2-1)
				{
					game_board[row][col] = Checker(BLACK_CHECKER);
				}
				else if(row > SIZE/2)
				{
					game_board[row][col] = Checker(RED_CHECKER);
				}
				else
				{
					game_board[row][col] = Checker();
				}
			}
			else
			{
				game_board[row][col] = Checker(INVALID);
			}
		}
	}

	glutPostRedisplay();
}

// пространство на доске из позиции x на экране (возможно клик мыши)

int board_position_from_coord_x(int x)
{
	return (int)((float)x / (float)global_window_width * float(SIZE));
}

// пространство на доске из позиции y на экране (возможно клик мыши)

int board_position_from_coord_y(int y)
{
	return (int)((float)y / (float)global_window_height * float(SIZE));
}

// проверка, находится ли клик в радиусе шашки

bool check_in_radius(int click_x, int click_y)
{
	click_y = global_window_height - click_y;

	board_position_x = board_position_from_coord_x(click_x);
	board_position_y = board_position_from_coord_y(click_y);

	if (game_board[board_position_y][board_position_x].type != INVALID && game_board[board_position_y][board_position_x].type != EMPTY)
	{
//		if (debug) std::cout << "got click at xy: (" << click_x << "," << click_y << "), bp: <" << board_position_x << "," << board_position_y << ">, inner: (" << inner_position_x << "," << inner_position_y << "), rad:" << sqrt( SQUARE( inner_position_x - 0.0625 ) + SQUARE( inner_position_y - 0.0625 ) ) << std::endl;
		inner_position_x = (click_x - (board_position_x * ((float)global_window_width / float(SIZE)))) / (float)global_window_width;
		inner_position_y = (click_y - (board_position_y * ((float)global_window_height / float(SIZE)))) / (float)global_window_height;
		return( sqrt( SQUARE( inner_position_x - (0.0625 / SIZE * 8) ) + SQUARE( inner_position_y - (0.0625 / SIZE * 8) ) ) <= piece_radius ? true : false );
	}

	if (debug) std::cout << "You can't click there!" << std::endl;
	return false;
}

// когда нажимается кнопка

void keyboard_action(unsigned char key, int x, int y)
{
	if (!piece_being_animated)
	{
		switch (key)
		{
			case 'd':	std::cout << "debug = " << (debug = !debug) << std::endl; glutPostRedisplay(); break;
			case 'r':	std::cout << "reset_game_state()" << std::endl; reset_game_state(); break;
			case 'p':	std::cout << "pruning_enabled = " << (pruning_enabled = !pruning_enabled) << std::endl; break;
			case '1':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 3) << std::endl; reset_game_state();	break;
			case '2':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 4) << std::endl; reset_game_state();	break;
			case '3':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 5) << std::endl; reset_game_state();	break;
			case '4':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 6) << std::endl; reset_game_state();	break;
			case '5':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 7) << std::endl; reset_game_state();	break;
			case '6':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 8) << std::endl; reset_game_state();	break;
			case '7':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 9) << std::endl; reset_game_state();	break;
			case '8':	std::cout << "MAX_SEARCH_DEPTH = " << (MAX_SEARCH_DEPTH = 10) << std::endl; reset_game_state();	break;
			case 'q': case 27 :	exit(0);							break;
		}
	}
}

// listener мышки

void mouse_action_listener(int mouse_x, int mouse_y)
{
	if (!piece_being_animated)
	{
		if (piece_being_dragged)
		{
			drag_offset_x += (mouse_x - old_mouse_x);
			drag_offset_y += (mouse_y - old_mouse_y);
			old_mouse_x = mouse_x;
			old_mouse_y = mouse_y;
			glutPostRedisplay();
	//		if (debug) std::cout << "Current drag offset: <" << drag_offset_x << "," << drag_offset_y << ">" << std::endl;
		}
	}
}

// нажатие / отпускание мыши

// тут заканчивается основная часть геймплея,
// так как программа управляется мышью, когда игрок отпускает шашку,
// ход передается компьютеру и рассчитывается ход

void mouse_action(int button, int state, int x, int y)
{
	if (!piece_being_animated)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			if (state == GLUT_DOWN) // если нажали ЛКМ
			{
				piece_being_dragged = check_in_radius(x,y);

				if (piece_being_dragged)
				{
					drag_offset_x = drag_offset_y = 0;
					old_mouse_x = x;
					old_mouse_y = y;
					if(debug) print_checker(game_board,board_position_x,board_position_y,"Clicked a");
				}
			}
			else
			{
				// отпустили шашку
				y = global_window_height - y;

				if (piece_being_dragged && move_if_valid(game_board, board_position_x, board_position_y, board_position_from_coord_x(x), board_position_from_coord_y(y)))
				{
					if (debug) print_checker(game_board,board_position_from_coord_x(x),board_position_from_coord_y(y),"Dropped a");
					piece_being_dragged = false;
					glutPostRedisplay();

					is_reds_turn = !is_reds_turn;

					// создает глобальное игровое дереве (массив очередей, отображающий каждый уровень minimax)
					construct_tree_from_board(game_board);

					// сделать в новом дереве лучший ход
					make_best_move();

					is_reds_turn = !is_reds_turn;
				}
				else
				{
					if (debug) std::cout << "Not a valid move." << std::endl;
					piece_being_dragged = false;
				}
			}
		}
	}
	glutPostRedisplay();
}

// нарисовать доску

void draw_board(void)
{
	is_playable = true; // сменить цвета доски

	glBegin(GL_QUADS);

	for (row = 0; row < SIZE; row++)
	{
		for (col = 0; col < SIZE; col++)
		{
			if (col != 0) is_playable = !is_playable;

			// red or black according to row and column
			glColor3fv( is_playable ? global_light_brown_color : global_brown_color );

			// draw a square on the board
			glVertex2f(col / float(SIZE), row / float(SIZE));
			glVertex2f((col+1) / float(SIZE), row / float(SIZE));
			glVertex2f((col+1) / float(SIZE), (row+1) / float(SIZE));
			glVertex2f(col / float(SIZE), (row+1) / float(SIZE));
		}
	}

	glEnd();
}

// нарисовать шашки на доске

void draw_pieces(Checker board[SIZE][SIZE])
{
	for (row = 0; row < SIZE; ++row)
	{
		for (col = 0; col < SIZE; ++col)
		{
			// если валидная шашка
			if (board[row][col].type != EMPTY && board[row][col].type != INVALID)
			{
				// покрасить ее
				glColor3fv(board[row][col].type == RED_CHECKER || board[row][col].type == RED_KING ? global_red_color : global_black_color);

				glPushMatrix();

					// переместить согласно ходу
					if (piece_being_dragged && (col == board_position_x) && (row == board_position_y))
					{
//						if (debug) std::cout << "Moving piece <" << board_position_x << "," << board_position_y << ">" << std::endl;
						glTranslatef((float)drag_offset_x / (float)global_window_width, -(float)drag_offset_y / (float)global_window_height, 0.1f );
					}

					if (piece_being_animated && (col == animated_from_x) && (row == animated_from_y))
					{
#ifdef _WIN32
						QueryPerformanceCounter(&current_time);
						move_complete_ratio = (float)(((current_time.QuadPart - animation_start.QuadPart) / (float)timer_resolution.QuadPart) * 1000.0) / animation_duration;
#else
						gettimeofday(&current_time, NULL);
						move_complete_ratio = (float)((current_time.tv_usec - animation_started.tv_usec) / 1000.0) / animation_duration;
#endif
						glTranslatef( move_complete_ratio / float(SIZE) * (float)(animated_to_x - animated_from_x), -move_complete_ratio / float(SIZE) * (float)(animated_from_y - animated_to_y), 0.3f);
						if (debug) std::cout << "We've completed " << (move_complete_ratio * 100.f) << "% of the animation" << std::endl;
#ifdef _WIN32
						if (move_complete_ratio >= 1.0f) disable_animation();
#else
						if (move_complete_ratio < 0.0f || move_complete_ratio >= 0.9f) disable_animation();
#endif
					}

					// нарисовать шашку
					glTranslatef(col / float(SIZE) + (0.0625f / SIZE * 8), row / float(SIZE) + (0.0625f / SIZE * 8), 0.1f);
					gluDisk(circle_drawer, 0.f, piece_radius, 32, 1);

					// если дамка, нарисовать кружок
					if (board[row][col].type == RED_KING || board[row][col].type == BLACK_KING)
					{
						glTranslatef(0.0f,0.0f,0.05f);
						glColor3fv(board[row][col].type == RED_KING ? global_red_accent_color : global_black_accent_color);
						gluDisk(circle_drawer, piece_radius / 1.5f - accent_width, piece_radius / 1.5f, 32, 1);
					}

				glPopMatrix();
			}
			else if (debug) // иначе чисто для дебага (пустые и невалидные)
			{
				// весь дебаг красным
				glColor3fv(global_red_color);

				glBegin(GL_LINES);

				if (board[row][col].type == INVALID)
				{
					// нарисовать крестики для невалидных
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.03125f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.09375f, row / float(SIZE) + 0.09375f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.09375f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.09375f, row / float(SIZE) + 0.03125f, 0.1f);
				}
				else
				{
					// E для пустых (Empty)
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.09375f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.09375f, row / float(SIZE) + 0.09375f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.03125f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.09375f, row / float(SIZE) + 0.03125f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.06250f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.09375f, row / float(SIZE) + 0.06250f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.03125f, 0.1f);
					glVertex3f(col / float(SIZE) + 0.03125f, row / float(SIZE) + 0.09375f, 0.1f);
				}

				glEnd();
			}
		}
	}

	glEnd();
}

// инфо о шашке

void print_checker(Checker board[SIZE][SIZE], int x, int y, char  *message)
{
	printf("%s %s at <%i,%i>\n", message, verbose_type(board[y][x]), x, y);
}

// красивый вывод типаы

char* verbose_type(Checker check)
{
	switch (check.type)
	{
		case INVALID:		return "invalid spot";		break;
		case EMPTY:			return "empty space";		break;
		case RED_CHECKER:	return "red checker";		break;
		case RED_KING:		return "red king";			break;
		case BLACK_CHECKER:	return "black checker";		break;
		case BLACK_KING:	return "black king";		break;
	}
	return "error checker type";
}

// когда ресайзим окно

void resize_window(int w, int h)
{
	global_window_width = w;
	global_window_height = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// проверидь валидность хода

bool move_if_valid(Checker board[SIZE][SIZE], int from_board_position_x, int from_board_position_y, int to_board_position_x, int to_board_position_y, bool ignore_turn)
{
	// условия раннего завершения (попытка ходить за пределы доски или на занятое место)
	to_piece = &board[to_board_position_y][to_board_position_x];
	from_piece = &board[from_board_position_y][from_board_position_x];

	// если наш ход
	if (!this_colors_turn(from_piece->type) && !ignore_turn) return false;

	// если мы на доске и пытаемся пойти в пустую клетку
	if (to_board_position_x < 0 || to_board_position_x > SIZE-1 || to_board_position_y < 0 || to_board_position_y > SIZE-1 || to_piece->type != EMPTY) return false;

	// считаем разность
	col = to_board_position_x - from_board_position_x;
	row = to_board_position_y - from_board_position_y;

	if (col == 0 || row == 0) return false;

//	if (debug) std::cout	<< (abs(row)) << " != " << (abs(col)) << (abs(row) != abs(col)) << ", "
//							<< (abs(col)) << " > 2 " << (abs(col) > 2) << ", "
//							<< (abs(row)) << " > 2" << (abs(row) > 2) << std::endl;

	// проверяем, если диагональ радиуса <=2 и разности равны
	if (abs(row) != abs(col) || abs(col) > 2 || abs(row) > 2) return false;

	// проверяем, идут ли шашки вперед
	if (row < 0 && from_piece->type == BLACK_CHECKER) return false;
	if (row > 0 && from_piece->type == RED_CHECKER) return false;

	// если на две клетки по диагонали, то есть ли шашка посередине?
	if (abs(row) == 2)
	{
		Checker *middle_piece = &board[from_board_position_y + row/2][from_board_position_x + col/2];

		// ищем врага по шашке
		switch (from_piece->type)
		{
			case RED_CHECKER:
			case RED_KING:

				switch (middle_piece->type) // позиция по центру
				{
					// если там оппонент, перепрыгиваем

					case BLACK_CHECKER:
						if (to_board_position_y == 0) from_piece->type = RED_KING; // если достигли другого конца, идем в дамки

					case BLACK_KING:
						middle_piece->type = EMPTY;

						to_piece->type = from_piece->type;
						from_piece->type = EMPTY;

						return true; break;

					default:
						return false; break;
				}

			break;

			case BLACK_CHECKER:
			case BLACK_KING:

				switch (middle_piece->type) // по центру
				{
					// если там оппонент, перепрыгиваем
					case RED_CHECKER:
						if (to_board_position_y == SIZE-1) from_piece->type = BLACK_KING; // если достигли другого конца, идем в дамки

					case RED_KING:
						middle_piece->type = EMPTY;

						to_piece->type = from_piece->type;
						from_piece->type = EMPTY;

						return true; break;

					default:
						return false; break;
				}

			break;
		}

		return false;
	}

	// иначе валидный 1/1 ход по диагонали
	if (from_piece->type == BLACK_CHECKER && to_board_position_y == SIZE-1) from_piece->type = BLACK_KING; // если достигли другого конца, идем в дамки
	if (from_piece->type == RED_CHECKER && to_board_position_y == 0) from_piece->type = RED_KING;

	to_piece->type = from_piece->type;
	from_piece->type = EMPTY;

	return true;
}

// чей ход?

bool this_colors_turn(checker_type type)
{
	return ((type == RED_CHECKER || type == RED_KING) && (is_reds_turn) || (type == BLACK_CHECKER || type == BLACK_KING) && (!is_reds_turn));
}

// начинаем или продолжаем создавать дерево minimax из доски

void construct_tree_from_board(Checker board[SIZE][SIZE])
{
	if(someone_won(game_board)) { reset_game_state(); return; }

	// начинаем дерево корневой нодой
	while (!game_tree[0].empty()) game_tree[0].pop_back();
	game_tree[0].push_back(TreeElement(board,NULL));
	pruned_count = 0;

	if (debug) std::cout << "Beginning construction at level 0 with " << game_tree[0].size() << " elements" << std::endl;

	// для каждой шашки каждой доске на каждом уровне проверяем доступные ходы
	for (int level = 1; level < MAX_SEARCH_DEPTH; ++level)
	{
		// clear current level (the one we're about to insert into)
		// очищаем текущий уровень (куда вставлять будем)
//		if (debug) std::cout << "Entering and clearing level " << (level) << ". Now clearing " << game_tree[level].size() << " existing records" << std::endl;

		while (!game_tree[level].empty()) game_tree[level].pop_back();

		// проходим все ноды из уровня выше
		for (unsigned int nodes_traversed = 0; nodes_traversed < game_tree[level-1].size(); ++nodes_traversed)
		{
			is_first_node = true;

			for (int i = 0; i < SIZE; ++i)
			{
				for (int j = 0; j < SIZE; ++j)
				{
					temp = game_tree[level-1][nodes_traversed].board[j][i];

					if (temp.type != EMPTY && temp.type != INVALID && (level % 2 == 0 ? temp.type == RED_CHECKER || temp.type == RED_KING : temp.type == BLACK_CHECKER || temp.type == BLACK_KING))
					{
						// стартуем с новой копией нашей доски
						copy_board(game_tree[level-1][nodes_traversed].board,temp_board);
//						if (debug && level % 2 == 1) print_checker(game_tree[level-1][nodes_traversed].board,i,j,"Checking branch moves from a");

						// перебираем все возможные ходы
						if (move_if_valid(temp_board,i,j,i+1,j+1,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, false, i, j, i+1, j+1); }
						if (move_if_valid(temp_board,i,j,i+1,j-1,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, false, i, j, i+1, j-1); }
						if (move_if_valid(temp_board,i,j,i-1,j+1,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, false, i, j, i-1, j+1); }
						if (move_if_valid(temp_board,i,j,i-1,j-1,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, false, i, j, i-1, j-1); }

						// делаем первый возможный ход из возможных прыжков
						if (move_if_valid(temp_board,i,j,i+2,j+2,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i+2, j+2); }
						if (move_if_valid(temp_board,i,j,i+2,j-2,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i+2, j-2); }
						if (move_if_valid(temp_board,i,j,i-2,j+2,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i-2, j+2); }
						if (move_if_valid(temp_board,i,j,i-2,j-2,true)) { insert_into_tree(level,game_tree[level], &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i-2, j-2); }

//						while(!double_jump_vector.empty()) // all double/triples/etc. jumps
//						{
//							double_jump_parent = double_jump_vector.back();
//							copy_board(double_jump_parent.board,temp_board);

//							if (debug) print_checker(double_jump_parent.board,double_jump_parent.from_x,double_jump_parent.from_y,"Found jump possible for a");

//							if (move_if_valid(temp_board,double_jump_parent.from_x,double_jump_parent.from_y,double_jump_parent.from_x+2,double_jump_parent.from_y+2,true)) { insert_into_tree(double_jump_vector, &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i+2, j+2); }
//							if (move_if_valid(temp_board,double_jump_parent.from_x,double_jump_parent.from_y,double_jump_parent.from_x+2,double_jump_parent.from_y-2,true)) { insert_into_tree(double_jump_vector, &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i+2, j-2); }
//							if (move_if_valid(temp_board,double_jump_parent.from_x,double_jump_parent.from_y,double_jump_parent.from_x-2,double_jump_parent.from_y+2,true)) { insert_into_tree(double_jump_vector, &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i-2, j+2); }
//							if (move_if_valid(temp_board,double_jump_parent.from_x,double_jump_parent.from_y,double_jump_parent.from_x-2,double_jump_parent.from_y-2,true)) { insert_into_tree(double_jump_vector, &game_tree[level-1][nodes_traversed], temp_board, true, i, j, i-2, j-2); }

							// push original state before multiple jumps
//							insert_into_tree(game_tree[level], &game_tree[level-1][nodes_traversed], double_jump_parent.board, true, i, j, double_jump_parent.to_x, double_jump_parent.to_y);
//							double_jump_vector.pop_back();
//						}
					}
				}
			}
		}
		if (debug) std::cout << "There are " << game_tree[level].size() << " nodes in this level (" << level << ")" << std::endl;
	}
	if (debug) std::cout << "Construction finished with " << pruned_count << " pruned" << std::endl;
}

// метод-помощник для вставки в дерево

void insert_into_tree(int level, std::vector<TreeElement> &tree, TreeElement *_parent, Checker board[SIZE][SIZE], bool jump, int from_x, int from_y, int to_x, int to_y)
{
	temp_element = TreeElement(board,_parent,jump,from_x,from_y,to_x,to_y);

//	if (debug && pruning_enabled) std::cout << (is_first_node ? "First node, " : "") << (level % 2 == 1 ? "Max node: " : "Min node: ") << temp_element.node_utility << (level % 2 == 1 ? " >= " : " <= ") << level_max_or_min[level] << " ? ";

	if (is_first_node)
	{
		is_first_node = false;
		tree.push_back(temp_element);
		level_max_or_min[level] = temp_element.node_utility;
//		if (debug) std::cout << " Added (first node)" << std::endl;
	}
	else if (pruning_enabled && !(level % 2 == 1 ? temp_element.node_utility >= level_max_or_min[level] : temp_element.node_utility <= level_max_or_min[level]))
	{
		++pruned_count;
//		if (debug) std::cout << " Pruned" << std::endl;
	}
	else
	{
		tree.push_back(temp_element);
		level_max_or_min[level] = temp_element.node_utility;
//		if (debug) std::cout << " Added" << std::endl;
	}

	copy_board(_parent->board,temp_board);

//	if (debug && ((tree.size() % 100000) == 99999)) std::cout << "Lowest level of our tree is now at " << tree.size() << " elements, we've pruned " << pruned_count << std::endl;
//	if (debug && _parent == &game_tree[0][0]) std::cout << "Got utility of " << comparison_utility << std::endl;
}


// глубокая копия доски в другую доску

void copy_board(Checker from_board[SIZE][SIZE], Checker to_board[SIZE][SIZE])
{
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			to_board[j][i].type = from_board[j][i].type;
		}
	}
}

// считаем выгодность каждого уровня и исходя из этого делаем лучший ход

void make_best_move()
{
	bool duplicates;
	int max_utility;
	TreeElement *best_parent;

	if (!game_tree[MAX_SEARCH_DEPTH-1].empty())
	{
		duplicates = false;
		max_utility = game_tree[MAX_SEARCH_DEPTH-1][0].path_utility;

		while(!best_elements.empty()) best_elements.pop_back();
		best_elements.push_back(0);

		for (unsigned int i = 1; i < game_tree[MAX_SEARCH_DEPTH-1].size(); ++i)
		{
			if (game_tree[MAX_SEARCH_DEPTH-1][i].path_utility > max_utility)
			{
				max_utility = game_tree[MAX_SEARCH_DEPTH-1][i].path_utility;
				while(!best_elements.empty()) best_elements.pop_back();
				best_elements.push_back(i);
				duplicates = false;
			}
			else if (game_tree[MAX_SEARCH_DEPTH-1][i].path_utility == max_utility)
			{
				duplicates = true;
				best_elements.push_back(i);
			}
//			if (debug) std::cout << "Utility at <" << (MAX_SEARCH_DEPTH-1) << "," << i << "> is " << game_tree[MAX_SEARCH_DEPTH-1][i].utility() << ", max is " << max_utility << (duplicates?" (with duplicates)":"") << std::endl;
		}

		srand((int)time(NULL));
		int random_choice = rand() % best_elements.size();

		best_parent = game_tree[MAX_SEARCH_DEPTH-1][best_elements[random_choice]].parent;
		if (debug) std::cout << "We chose element #" << random_choice << " out of " << (best_elements.size()-1) << " duplicates." << std::endl << "Now propagating up to first move in path." << std::endl;

		// возвращаемся наверх дерева
		while (best_parent->parent->parent != NULL) best_parent = best_parent->parent;

//		if (debug) print_checker(game_board,best_parent->get_from_x(),best_parent->get_from_y(),"Trying to move a ");
		enable_animation(best_parent->from_x,best_parent->from_y,best_parent->to_x,best_parent->to_y,1000.f);
	}
	else if (debug) std::cout << "There's no end node? How did that happen?" << std::endl;

	for (int i = 0; i < MAX_SEARCH_DEPTH; ++i)
	{
		while (!game_tree[i].empty())
		{
			game_tree[i].pop_back();
		}

//		if (debug) std::cout << "Deleted level " << i << ", there are now " << game_tree[i].size() << " entries" << std::endl;
	}
}

// show an animation of the computer moving

void enable_animation(int from_x, int from_y, int to_x, int to_y, float duration)
{
//	if (debug) { print_checker(game_board,from_x,from_y,"Trying to move a"); std::cout << " to <" << to_x << "," << to_y << ">"; }

	animated_from_x = from_x;
	animated_from_y = from_y;
	animated_to_x = to_x;
	animated_to_y = to_y;

	animation_duration = duration; // мсек

	piece_being_animated = true;

#ifdef _WIN32
	QueryPerformanceCounter(&animation_start);
#else
	gettimeofday(&animation_started, NULL);
#endif
}

// анимация закончилась

void disable_animation(void)
{
	if (!move_if_valid(game_board,animated_from_x,animated_from_y,animated_to_x,animated_to_y,true)) print_checker(game_board,animated_from_x,animated_from_y,"Failed to move a");
	piece_being_animated = false;
	glutPostRedisplay();
}

// выиграл ли кто-то?

bool someone_won(Checker board[SIZE][SIZE])
{
	bool red_found = false, black_found = false;

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			if (!red_found && board[j][i].type == RED_CHECKER || board[j][i].type == RED_KING) red_found = true;
			if (!black_found && board[j][i].type == BLACK_CHECKER || board[j][i].type == BLACK_KING) black_found = true;
			if (red_found && black_found) return false;
		}
	}

	return true;
}
