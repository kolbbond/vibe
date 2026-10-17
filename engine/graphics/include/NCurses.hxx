// ncurses wrapper

// NCurses.hh

#pragma once

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <ncurses.h>
namespace vibe { namespace gfx {
typedef std::shared_ptr<class NCurses> ShNCursesPr;

class NCurses {

	// properties
protected:
	WINDOW* window_;

	// need a list of windows points...
	std::vector<WINDOW*> windows_;

	int num_windows_;

	// methods
public:
	// constructor
	NCurses();

	// init stuff
	void init();
	void init_flush();
	void init_keypad();

	WINDOW* new_window(int, int, int, int);

	void add_window(WINDOW*);

	void create_box(WINDOW*, int, int);
	void rectangle(WINDOW* win,int x1, int y1, int x2, int y2);

	int get_input();

	void init_color();

	void create_color(int, int, int);
	void color_on(int);
	void add_color(int, int, int);

	void win_bg_color(WINDOW* win, int num);

	void move_window(WINDOW*, int, int);

	void print(std::string);
	void move_window_print(WINDOW*, int, int, std::string);

	void refresh_screen();
	void window_refresh(WINDOW*);

	void clear_screen();

	void destroy();

	// destructor
	~NCurses();

	// factory
	static ShNCursesPr create();
};

}} // namespace vibe::gfx
