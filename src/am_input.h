enum am_key {
    AM_KEY_UNKNOWN,

    AM_KEY_A,
    AM_KEY_B,
    AM_KEY_C,
    AM_KEY_D,
    AM_KEY_E,
    AM_KEY_F,
    AM_KEY_G,
    AM_KEY_H,
    AM_KEY_I,
    AM_KEY_J,
    AM_KEY_K,
    AM_KEY_L,
    AM_KEY_M,
    AM_KEY_N,
    AM_KEY_O,
    AM_KEY_P,
    AM_KEY_Q,
    AM_KEY_R,
    AM_KEY_S,
    AM_KEY_T,
    AM_KEY_U,
    AM_KEY_V,
    AM_KEY_W,
    AM_KEY_X,
    AM_KEY_Y,
    AM_KEY_Z,

    AM_KEY_1,
    AM_KEY_2,
    AM_KEY_3,
    AM_KEY_4,
    AM_KEY_5,
    AM_KEY_6,
    AM_KEY_7,
    AM_KEY_8,
    AM_KEY_9,
    AM_KEY_0,

    AM_KEY_ENTER,
    AM_KEY_ESCAPE,
    AM_KEY_BACKSPACE,
    AM_KEY_TAB,
    AM_KEY_SPACE,

    AM_KEY_MINUS,
    AM_KEY_EQUALS,
    AM_KEY_LEFTBRACKET,
    AM_KEY_RIGHTBRACKET,
    AM_KEY_BACKSLASH,
    AM_KEY_SEMICOLON,
    AM_KEY_QUOTE,
    AM_KEY_BACKQUOTE,
    AM_KEY_COMMA,
    AM_KEY_PERIOD,
    AM_KEY_SLASH,

    AM_KEY_CAPSLOCK,

    AM_KEY_F1,
    AM_KEY_F2,
    AM_KEY_F3,
    AM_KEY_F4,
    AM_KEY_F5,
    AM_KEY_F6,
    AM_KEY_F7,
    AM_KEY_F8,
    AM_KEY_F9,
    AM_KEY_F10,
    AM_KEY_F11,
    AM_KEY_F12,

    AM_KEY_PRINTSCREEN,
    AM_KEY_SCROLLLOCK,
    AM_KEY_PAUSE,
    AM_KEY_INSERT,
    AM_KEY_HOME,
    AM_KEY_PAGEUP,
    AM_KEY_DELETE,
    AM_KEY_END,
    AM_KEY_PAGEDOWN,
    AM_KEY_RIGHT,
    AM_KEY_LEFT,
    AM_KEY_DOWN,
    AM_KEY_UP,

    AM_KEY_KP_DIVIDE,
    AM_KEY_KP_MULTIPLY,
    AM_KEY_KP_MINUS,
    AM_KEY_KP_PLUS,
    AM_KEY_KP_ENTER,
    AM_KEY_KP_1,
    AM_KEY_KP_2,
    AM_KEY_KP_3,
    AM_KEY_KP_4,
    AM_KEY_KP_5,
    AM_KEY_KP_6,
    AM_KEY_KP_7,
    AM_KEY_KP_8,
    AM_KEY_KP_9,
    AM_KEY_KP_0,
    AM_KEY_KP_PERIOD,

    AM_KEY_LCTRL,
    AM_KEY_LSHIFT,
    AM_KEY_LALT,
    AM_KEY_LGUI,
    AM_KEY_RCTRL,
    AM_KEY_RSHIFT,
    AM_KEY_RALT,
    AM_KEY_RGUI,

    AM_NUM_KEYS
};

enum am_gamepad_button {
    AM_GAMEPAD_UP,
    AM_GAMEPAD_DOWN,
    AM_GAMEPAD_LEFT,
    AM_GAMEPAD_RIGHT,
    AM_GAMEPAD_START,
    AM_GAMEPAD_BACK,
    AM_GAMEPAD_LSTICK_BUTTON,
    AM_GAMEPAD_RSTICK_BUTTON,
    AM_GAMEPAD_LB,
    AM_GAMEPAD_RB,
    AM_GAMEPAD_HOME,
    AM_GAMEPAD_A,
    AM_GAMEPAD_B,
    AM_GAMEPAD_X,
    AM_GAMEPAD_Y,
    AM_GAMEPAD_NUM_BUTTONS
};

enum am_gamepad_axis {
    AM_GAMEPAD_LSTICK_X,
    AM_GAMEPAD_LSTICK_Y,
    AM_GAMEPAD_RSTICK_X,
    AM_GAMEPAD_RSTICK_Y,
    AM_GAMEPAD_AM,
    AM_GAMEPAD_RT,
    AM_GAMEPAD_NUM_AXES
};

enum am_mouse_button {
    AM_MOUSE_BUTTON_LEFT,
    AM_MOUSE_BUTTON_RIGHT,
    AM_MOUSE_BUTTON_MIDDLE,
};

const char *am_key_name(am_key key);

/*
TODO
void am_handle_key_down(lua_State *L, am_native_window *nwin, am_key key);
void am_handle_key_up(lua_State *L, am_native_window *nwin, am_key key);
void am_handle_mouse_move(lua_State *L, am_native_window *nwin, float x, float y);
void am_handle_mouse_down(lua_State *L, am_native_window *nwin, am_mouse_button button);
void am_handle_mouse_up(lua_State *L, am_native_window *nwin, am_mouse_button button);
*/
