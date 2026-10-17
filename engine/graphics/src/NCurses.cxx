
// NCurses.cpp
// ###### -

// include header
#include "NCurses.hxx"

namespace vibe { namespace gfx {
// constructor
NCurses::NCurses() {}

// destructor
NCurses::~NCurses() {}

// init stuff
void NCurses::init() {
	// wrap init
	initscr();
}

void NCurses::init_flush() {
	// wrap initial flush
	intrflush(stdscr, FALSE);
}

void NCurses::init_keypad() {
	// wrap init key
	keypad(stdscr, TRUE);
}

WINDOW* NCurses::new_window(int x, int y, int width, int height) {
	// wrap new win
	return newwin(height, width, y, x);
}

void NCurses::create_box(WINDOW* win, int x, int y) {
	// wrap box with different name
	box(win, y, x);
}

void NCurses::rectangle(WINDOW* win, int x1, int y1, int x2, int y2) {
	mvwhline(win, y1, x1, 0, x2 - x1);
	mvwhline(win, y2, x1, 0, x2 - x1);
	mvwvline(win, y1, x1, 0, y2 - y1);
	mvwvline(win, y1, x2, 0, y2 - y1);
	mvwaddch(win, y1, x1, ACS_ULCORNER);
	mvwaddch(win, y2, x1, ACS_LLCORNER);
	mvwaddch(win, y1, x2, ACS_URCORNER);
	mvwaddch(win, y2, x2, ACS_LRCORNER);
}

void NCurses::refresh_screen() {
	// wrap refresh
	refresh();
}

int NCurses::get_input() {
	// wrap get ch
	return getch();
}

void NCurses::move_window(WINDOW* win, int x, int y) {
	// wrap move win
	mvwin(win, y, x);
}

void NCurses::print(std::string str) {
	// wrap
	printw(str.c_str());
}

void NCurses::move_window_print(WINDOW* win, int x, int y, std::string str) {
	// wrap win move print
	mvwprintw(win, y, x, str.c_str());
}

void NCurses::init_color() {
	start_color();
}

void NCurses::add_color(int num, int color1, int color2) {
	init_pair(num, color1, color2);
}

void NCurses::color_on(int num) {
	attron(COLOR_PAIR(num));
}

void NCurses::win_bg_color(WINDOW* win, int num) {

	wbkgd(win, COLOR_PAIR(num));
}

void NCurses::window_refresh(WINDOW* win) {
	// wrap refresh
	wrefresh(win);
}

void NCurses::clear_screen() {
	// wrap clear
	clear();
}

void NCurses::destroy() {
	// wrap end
	endwin();
};

// destructor
// setters

// getters

// static

// factory
ShNCursesPr NCurses::create() {
	return std::make_shared<NCurses>();
}
}} // namespace vibe::gfx
