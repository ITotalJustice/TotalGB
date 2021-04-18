#include "frontend/platforms/video/allegro5/allegro5_video.hpp"
#include "core/gb.h"

#ifdef MGB_ALLEGRO5_VIDEO

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/keycodes.h>

#include <cstdint>
#include <cstring>


namespace mgb::platform::video::allegro5 {

Allegro5::~Allegro5() {
	if (this->queue) {
		al_destroy_event_queue(this->queue);
	}
	
	if (this->bitmap) {
		al_destroy_bitmap(this->bitmap);
	}

	if (this->display) {
		al_destroy_display(this->display);
	}

	if (al_is_joystick_installed()) {
		al_uninstall_joystick();
	}

	if (al_is_keyboard_installed()) {
		al_uninstall_keyboard();
	}

	if (al_is_mouse_installed()) {
		al_uninstall_mouse();
	}

	if (al_is_image_addon_initialized()) {
		al_shutdown_image_addon();
	}
}

auto Allegro5::SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	// this seems safe to call as many times as needed
	// as it just sets up an atexit()!
    if (!al_init()) {
    	printf("[AL_AUDIO] failed to init audio\n");
    	return false;
    }

	if (!al_install_mouse()) {
		printf("[ALLEGRO5-VIDEO] failed to install mouse\n");
		return false;
	}

    if (!al_install_keyboard()) {
		printf("[ALLEGRO5-VIDEO] failed to install keyboard\n");
		return false;
    }

    if (!al_install_joystick()) {
		printf("[ALLEGRO5-VIDEO] failed to install joystick\n");
		return false;
    }

    if (!al_init_image_addon()) {
		printf("[ALLEGRO5-VIDEO] failed to init image addon\n");
    	return false;	
    }

    int display_flags = ALLEGRO_RESIZABLE;

    al_set_new_display_flags(display_flags);

	this->display = al_create_display(
		vid_info.w, vid_info.h
	);

	if (!this->display) {
		printf("[ALLEGRO5-VIDEO] failed to create a display\n");
		return false;
	}

	// change bitmap format to BGR555
	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_BGR_555);

	// now create the bitmap
	this->bitmap = al_create_bitmap(
		game_info.w, game_info.h
	);

	if (!this->bitmap) {
		printf("[ALLEGRO5-VIDEO] failed to create bitmap\n");
		return false;
	}

	this->queue = al_create_event_queue();

    al_register_event_source(this->queue, al_get_keyboard_event_source()); 
    al_register_event_source(this->queue, al_get_display_event_source(display));

    return true;
}

auto Allegro5::UpdateGameTexture(GameTextureData data) -> void {
	auto locked_region = al_lock_bitmap(
    	this->bitmap,
    	ALLEGRO_PIXEL_FORMAT_BGR_555,
    	ALLEGRO_LOCK_WRITEONLY
    );

	constexpr auto size = 160 * 144 * 2;

	std::memcpy(
		((uint8_t*)(locked_region->data)) - (size + locked_region->pitch),
    	data.pixels,
    	size
	);

    al_unlock_bitmap(this->bitmap);
}

auto Allegro5::RenderDisplay() -> void {
	al_clear_to_color(al_map_rgb_f(0, 0, 0));

    al_draw_scaled_bitmap(
    	this->bitmap,
    	0, 0, 160, 144,
    	0, 0, 160 * 4, 144 * 4,
    	ALLEGRO_FLIP_VERTICAL
    );
    
    al_flip_display();
}

auto Allegro5::PollEvents() -> void {
	const auto OnJoystickEvent = [this](auto type, ALLEGRO_JOYSTICK_EVENT& e) -> void {
		switch (type) {
			case ALLEGRO_EVENT_JOYSTICK_AXIS:
				break;

   			case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
   				break;

   			case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
   				break;

   			case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
   				break;
		}
	};

	const auto OnKeyEvent = [this](auto type, ALLEGRO_KEYBOARD_EVENT& e) -> void {
		struct KeyMapEntry {
		    int key;
		    GB_Button button;
		};

		constexpr std::array<KeyMapEntry, 8> key_map{{
		    { ALLEGRO_KEY_X, GB_BUTTON_A },
		    { ALLEGRO_KEY_Z, GB_BUTTON_B },
		    { ALLEGRO_KEY_ENTER, GB_BUTTON_START },
		    { ALLEGRO_KEY_SPACE, GB_BUTTON_SELECT },
		    { ALLEGRO_KEY_DOWN, GB_BUTTON_DOWN },
		    { ALLEGRO_KEY_UP, GB_BUTTON_UP },
		    { ALLEGRO_KEY_LEFT, GB_BUTTON_LEFT },
		    { ALLEGRO_KEY_RIGHT, GB_BUTTON_RIGHT },
		}};

		const bool kdown = type == ALLEGRO_EVENT_KEY_DOWN;

		// first check if any of the mapped keys were pressed...
	    for (auto [key, button] : key_map) {
	        if (key == e.keycode) {
	            GB_set_buttons(this->callback.GetCore(), button, kdown);
	            return;
	        }
	    }

	    // usually best to respond once the key is released
	    if (kdown) {
	        return;
	    }

		switch (e.keycode) {
			case ALLEGRO_KEY_O:
				this->callback.FilePicker();
				break;

		// these are hotkeys to toggle layers of the gb core
	    // such as bg, win, obj...
	        case ALLEGRO_KEY_0:
	            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_ALL);
	            break;

	        case ALLEGRO_KEY_1:
	            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_BG);
	            break;

	        case ALLEGRO_KEY_2:
	            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_WIN);
	            break;

	        case ALLEGRO_KEY_3:
	            GB_set_render_palette_layer_config(this->callback.GetCore(), GB_RENDER_LAYER_CONFIG_OBJ);
	            break;

	    // these are for savestates
	        case ALLEGRO_KEY_F1:
	            this->callback.SaveState();
	            break;

	        case ALLEGRO_KEY_F2:
	            this->callback.LoadState();
	            break;
		}
	};

	const auto OnMouseEvent = [this](auto type, ALLEGRO_MOUSE_EVENT& e) -> void {
		switch (type) {
			case ALLEGRO_EVENT_MOUSE_AXES:
				break;

			case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
				break;

			case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
				break;

			case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
				break;

			case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
				break;

			case ALLEGRO_EVENT_MOUSE_WARPED:
				break;
		}
	};

	const auto OnTimerEvent = [this](auto type, ALLEGRO_TIMER_EVENT& e) -> void {

	};

	const auto OnDisplayEvent = [this](auto type, ALLEGRO_DISPLAY_EVENT& e) -> void {
		switch (type) {
			case ALLEGRO_EVENT_DISPLAY_EXPOSE:
				break;

			case ALLEGRO_EVENT_DISPLAY_RESIZE:
				break;

			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				this->callback.OnQuit();
				break;

			case ALLEGRO_EVENT_DISPLAY_LOST:
				break;

			case ALLEGRO_EVENT_DISPLAY_FOUND:
				break;

			case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
				break;

			case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
				break;

			case ALLEGRO_EVENT_DISPLAY_ORIENTATION:
				break;

			case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
				break;

			case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
				break;

			case ALLEGRO_EVENT_DISPLAY_CONNECTED:
				break;

			case ALLEGRO_EVENT_DISPLAY_DISCONNECTED:
				break;
		}
	};

	const auto OnTouchEvent = [this](auto type, ALLEGRO_TOUCH_EVENT& e) -> void {
		switch (type) {
			case ALLEGRO_EVENT_TOUCH_BEGIN:
				break;

			case ALLEGRO_EVENT_TOUCH_END:
				break;

			case ALLEGRO_EVENT_TOUCH_MOVE:
				break;

			case ALLEGRO_EVENT_TOUCH_CANCEL:
				break;
		}
	};


	ALLEGRO_EVENT e;
	
	while (al_get_next_event(this->queue, &e)) {
		switch (e.type) {
   			case ALLEGRO_EVENT_JOYSTICK_AXIS:
   			case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
   			case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
   			case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
   				OnJoystickEvent(e.type, e.joystick);
   				break;

			case ALLEGRO_EVENT_KEY_DOWN:
			// case ALLEGRO_EVENT_KEY_CHAR:
			case ALLEGRO_EVENT_KEY_UP:
				OnKeyEvent(e.type, e.keyboard);
				break;

			case ALLEGRO_EVENT_MOUSE_AXES:
			case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
			case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
			case ALLEGRO_EVENT_MOUSE_WARPED:
				OnMouseEvent(e.type, e.mouse);
				break;

			case ALLEGRO_EVENT_TIMER:
				OnTimerEvent(e.type, e.timer);
				break;

			case ALLEGRO_EVENT_DISPLAY_EXPOSE:
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
			case ALLEGRO_EVENT_DISPLAY_LOST:
			case ALLEGRO_EVENT_DISPLAY_FOUND:
			case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
			case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			case ALLEGRO_EVENT_DISPLAY_ORIENTATION:
			case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
			case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
			case ALLEGRO_EVENT_DISPLAY_CONNECTED:
			case ALLEGRO_EVENT_DISPLAY_DISCONNECTED:
				OnDisplayEvent(e.type, e.display);
				break;

			case ALLEGRO_EVENT_TOUCH_BEGIN:
			case ALLEGRO_EVENT_TOUCH_END:
			case ALLEGRO_EVENT_TOUCH_MOVE:
			case ALLEGRO_EVENT_TOUCH_CANCEL:
				OnTouchEvent(e.type, e.touch);
				break;
		}
	}
}

} // namespace mgb::platform::video::allegro5

#endif // MGB_ALLEGRO5_VIDEO
