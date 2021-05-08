#include "gui.h"
#include "menu/menu.hpp"


void gui_run(
	struct mgb* mgb
) {
	(void)mgb;
}

void gui_render(
	struct mgb* mgb
) {
	// todo: select menu
	menu::Main(mgb);
}
