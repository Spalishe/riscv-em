#pragma once

#include <cstdint>

// HID Keyboard handle
struct hid_keyboard;

// HID Mouse handle
struct hid_mouse;

void hid_keyboard_press(hid_keyboard* kb, uint8_t key);

// Release a keyboard key
void hid_keyboard_release(hid_keyboard* kb, uint8_t key);

// Press mouse buttons
void hid_mouse_press(hid_mouse* mouse, uint8_t btns);

// Release mouse buttons
void hid_mouse_release(hid_mouse* mouse, uint8_t btns);

// Scroll mouse wheel (Positive offset goes downwards)
void hid_mouse_scroll(hid_mouse* mouse, int32_t offset);

// Set tablet resolution, must be called before hid_mouse_place()
void hid_mouse_resolution(hid_mouse* mouse, uint32_t x, uint32_t y);

// Relative mouse movement
void hid_mouse_move(hid_mouse* mouse, int32_t x, int32_t y);

// Absolute mouse movement (Tablet mode for host cursor integration)
// Set tablet resolution with hid_mouse_resolution() beforehand
void hid_mouse_place(hid_mouse* mouse, int32_t x, int32_t y);

/*
 * Mouse definitions
 */

#define HID_BTN_NONE    0x0
#define HID_BTN_LEFT    0x1 //!< Left mouse button
#define HID_BTN_RIGHT   0x2 //!< Right mouse button
#define HID_BTN_MIDDLE  0x4 //!< Middle mouse button (Scrollwheel)

#define HID_SCROLL_UP   -1  //!< Scroll one unit upward
#define HID_SCROLL_DOWN 1   //!< Scroll one unit downward

/*
 * Keyboard keycode definitions
 */

#define HID_KEY_NONE 0x00

// Keyboard errors (Non-physical keys)
#define HID_KEY_ERR_ROLLOVER  0x01
#define HID_KEY_ERR_POSTFAIL  0x02
#define HID_KEY_ERR_UNDEFINED 0x03

// Typing keys
#define HID_KEY_A 0x04
#define HID_KEY_B 0x05
#define HID_KEY_C 0x06
#define HID_KEY_D 0x07
#define HID_KEY_E 0x08
#define HID_KEY_F 0x09
#define HID_KEY_G 0x0a
#define HID_KEY_H 0x0b
#define HID_KEY_I 0x0c
#define HID_KEY_J 0x0d
#define HID_KEY_K 0x0e
#define HID_KEY_L 0x0f
#define HID_KEY_M 0x10
#define HID_KEY_N 0x11
#define HID_KEY_O 0x12
#define HID_KEY_P 0x13
#define HID_KEY_Q 0x14
#define HID_KEY_R 0x15
#define HID_KEY_S 0x16
#define HID_KEY_U 0x18
#define HID_KEY_V 0x19
#define HID_KEY_W 0x1a
#define HID_KEY_X 0x1b
#define HID_KEY_Y 0x1c
#define HID_KEY_Z 0x1d

// Number keys
#define HID_KEY_1 0x1e
#define HID_KEY_2 0x1f
#define HID_KEY_3 0x20
#define HID_KEY_4 0x21
#define HID_KEY_5 0x22
#define HID_KEY_6 0x23
#define HID_KEY_7 0x24
#define HID_KEY_8 0x25
#define HID_KEY_9 0x26
#define HID_KEY_0 0x27

// Control keys
#define HID_KEY_ENTER      0x28
#define HID_KEY_ESC        0x29
#define HID_KEY_BACKSPACE  0x2a
#define uint8_tAB        0x2b
#define HID_KEY_SPACE      0x2c
#define HID_KEY_MINUS      0x2d
#define HID_KEY_EQUAL      0x2e
#define HID_KEY_LEFTBRACE  0x2f //!< Button [ {
#define HID_KEY_RIGHTBRACE 0x30 //!< Button ] }
#define HID_KEY_BACKSLASH  0x31
#define HID_KEY_HASHTILDE  0x32 //!< Button # ~ (Huh? Never seen one.)
#define HID_KEY_SEMICOLON  0x33 //!< Button ; :
#define HID_KEY_APOSTROPHE 0x34 //!< Button ' "
#define HID_KEY_GRAVE      0x35 //!< Button ` ~ (For dummies: Quake console button)
#define HID_KEY_COMMA      0x36 //!< Button , <
#define HID_KEY_DOT        0x37 //!< Button . >
#define HID_KEY_SLASH      0x38
#define HID_KEY_CAPSLOCK   0x39

// Function keys
#define HID_KEY_F1  0x3a
#define HID_KEY_F2  0x3b
#define HID_KEY_F3  0x3c
#define HID_KEY_F4  0x3d
#define HID_KEY_F5  0x3e
#define HID_KEY_F6  0x3f
#define HID_KEY_F7  0x40
#define HID_KEY_F8  0x41
#define HID_KEY_F9  0x42
#define HID_KEY_F10 0x43
#define HID_KEY_F11 0x44
#define HID_KEY_F12 0x45

// Editing keys
#define HID_KEY_SYSRQ      0x46 //!< Print Screen (REISUB, anyone?)
#define HID_KEY_SCROLLLOCK 0x47
#define HID_KEY_PAUSE      0x48
#define HID_KEY_INSERT     0x49
#define HID_KEY_HOME       0x4a
#define HID_KEY_PAGEUP     0x4b
#define HID_KEY_DELETE     0x4c
#define HID_KEY_END        0x4d
#define HID_KEY_PAGEDOWN   0x4e
#define HID_KEY_RIGHT      0x4f //!< Right Arrow
#define HID_KEY_LEFT       0x50 //!< Left Arrow
#define HID_KEY_DOWN       0x51 //!< Down Arrow
#define HID_KEY_UP         0x52 //!< Up Arrow

// Numpad keys
#define HID_KEY_NUMLOCK    0x53
#define HID_KEY_KPSLASH    0x54
#define HID_KEY_KPASTERISK 0x55 //!< Button *
#define HID_KEY_KPMINUS    0x56
#define HID_KEY_KPPLUS     0x57
#define HID_KEY_KPENTER    0x58
#define HID_KEY_KP1        0x59
#define HID_KEY_KP2        0x5a
#define HID_KEY_KP3        0x5b
#define HID_KEY_KP4        0x5c
#define HID_KEY_KP5        0x5d
#define HID_KEY_KP6        0x5e
#define HID_KEY_KP7        0x5f
#define HID_KEY_KP8        0x60
#define HID_KEY_KP9        0x61
#define HID_KEY_KP0        0x62
#define HID_KEY_KPDOT      0x63

// Non-US keyboard keys
#define HID_KEY_102ND      0x64 //!< Non-US \ and |, also <> key on German-like keyboards
#define HID_KEY_COMPOSE    0x65 //!< Compose key
#define HID_KEY_POWER      0x66 //!< Poweroff key
#define HID_KEY_KPEQUAL    0x67 //!< Keypad =

// Function keys (F13 - F24)
#define HID_KEY_F13 0x68
#define HID_KEY_F14 0x69
#define HID_KEY_F15 0x6a
#define HID_KEY_F16 0x6b
#define HID_KEY_F17 0x6c
#define HID_KEY_F18 0x6d
#define HID_KEY_F19 0x6e
#define HID_KEY_F20 0x6f
#define HID_KEY_F21 0x70
#define HID_KEY_F22 0x71
#define HID_KEY_F23 0x72
#define HID_KEY_F24 0x73

// Non-US Media/special keys
#define HID_KEY_OPEN       0x74 //!< Execute
#define HID_KEY_HELP       0x75
#define HID_KEY_PROPS      0x76 //!< Context menu key (Near right Alt) - Linux evdev naming
#define HID_KEY_MENU       0x76 //!< ^ Context menu key too, different naming
#define HID_KEY_FRONT      0x77 //!< Select key
#define HID_KEY_STOP       0x78
#define HID_KEY_AGAIN      0x79
#define HID_KEY_UNDO       0x7a
#define HID_KEY_CUT        0x7b
#define HID_KEY_COPY       0x7c
#define HID_KEY_PASTE      0x7d
#define HID_KEY_FIND       0x7e
#define HID_KEY_MUTE       0x7f
#define HID_KEY_VOLUMEUP   0x80
#define HID_KEY_VOLUMEDOWN 0x81
#define HID_KEY_KPCOMMA    0x85 //!< Keypad Comma (Brazilian keypad period key?)

// International keys
#define HID_KEY_RO               0x87 //!< International1 (Japanese Ro, \\ key)
#define HID_KEY_KATAKANAHIRAGANA 0x88 //!< International2 (Japanese Katakana/Hiragana, second key right to spacebar)
#define HID_KEY_YEN              0x89 //!< International3 (Japanese Yen)
#define HID_KEY_HENKAN           0x8a //!< International4 (Japanese Henkan, key right to spacebar)
#define HID_KEY_MUHENKAN         0x8b //!< International5 (Japanese Muhenkan, key left to spacebar)
#define HID_KEY_KPJPCOMMA        0x8c //!< International6 (Japanese Comma? See HID spec...)

// LANG keys
#define HID_KEY_HANGEUL        0x90 //!< LANG1 (Korean Hangul/English toggle key)
#define HID_KEY_HANJA          0x91 //!< LANG2 (Korean Hanja control key)
#define HID_KEY_KATAKANA       0x92 //!< LANG3 (Japanese Katakana key)
#define HID_KEY_HIRAGANA       0x93 //!< LANG4 (Japanese Hiragana key)
#define HID_KEY_ZENKAKUHANKAKU 0x94 //!< LANG5 (Japanese Zenkaku/Hankaku key)

// Additional keypad keys
#define HID_KEY_KPLEFTPAREN  0xb6 //!< Keypad (
#define HID_KEY_KPRIGHTPAREN 0xb7 //!< Keypad )

// Modifier keys
#define HID_KEY_LEFTCTRL   0xe0
#define HID_KEY_LEFTSHIFT  0xe1
#define HID_KEY_LEFTALT    0xe2
#define HID_KEY_LEFTMETA   0xe3 //!< The one with the ugly Windows icon
#define HID_KEY_RIGHTCTRL  0xe4
#define HID_KEY_RIGHTSHIFT 0xe5
#define HID_KEY_RIGHTALT   0xe6
#define HID_KEY_RIGHTMETA  0xe7

// Media keys
#define HID_KEY_MEDIA_PLAYPAUSE    0xe8
#define HID_KEY_MEDIA_STOPCD       0xe9
#define HID_KEY_MEDIA_PREVIOUSSONG 0xea
#define HID_KEY_MEDIA_NEXTSONG     0xeb
#define HID_KEY_MEDIA_EJECTCD      0xec
#define HID_KEY_MEDIA_VOLUMEUP     0xed
#define HID_KEY_MEDIA_VOLUMEDOWN   0xee
#define HID_KEY_MEDIA_MUTE         0xef
#define HID_KEY_MEDIA_WWW          0xf0
#define HID_KEY_MEDIA_BACK         0xf1
#define HID_KEY_MEDIA_FORWARD      0xf2
#define HID_KEY_MEDIA_STOP         0xf3
#define HID_KEY_MEDIA_FIND         0xf4
#define HID_KEY_MEDIA_SCROLLUP     0xf5
#define HID_KEY_MEDIA_SCROLLDOWN   0xf6
#define HID_KEY_MEDIA_EDIT         0xf7
#define HID_KEY_MEDIA_SLEEP        0xf8
#define HID_KEY_MEDIA_COFFEE       0xf9
#define HID_KEY_MEDIA_REFRESH      0xfa
#define HID_KEY_MEDIA_CALC         0xfb
