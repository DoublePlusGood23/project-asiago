#include <modules/screen/screen.h>

#include <modules/screen/uefi.h>
#include <feta.h>

#define DEFAULT_FOREGROUND_COLOR EFI_WHITE
#define DEFAULT_BACKGROUND_COLOR EFI_BLACK

// ---- terminal ----

void screen_terminal_init() {
	uefi_terminal_init();
	screen_terminal_resetForegroundColor();
	screen_terminal_resetBackgroundColor();
	screen_terminal_clear();
}

void screen_terminal_clear() {
	uefi_terminal_clear();
}

void screen_terminal_writeString(String data) {
	uefi_terminal_writeString(data);
}

void screen_terminal_writeString(strchar data) {
	strchar string[] = { data, 0 };
	screen_terminal_writeString((String) string);
}
uint64 screen_terminal_cursorColumn() {
	return uefi_terminal_cursorColumn();
}
uint64 screen_terminal_cursorRow() {
	return uefi_terminal_cursorRow();
}
void screen_terminal_setCursorColumn(uinteger column) {
	uefi_terminal_setCursorColumn(column);
}
void screen_terminal_setCursorRow(uinteger row) {
	uefi_terminal_setCursorRow(row);
}

void screen_terminal_setForegroundColor(UINTN color) {
	uefi_terminal_setForegroundColor(color);
}
void screen_terminal_resetForegroundColor() {
	screen_terminal_setForegroundColor(DEFAULT_FOREGROUND_COLOR);
}

void screen_terminal_setBackgroundColor(UINTN color) {
	uefi_terminal_setBackgroundColor(color);
}
void screen_terminal_resetBackgroundColor() {
	uefi_terminal_setBackgroundColor(DEFAULT_BACKGROUND_COLOR);
}

// ---- graphics ----

void screen_graphics_init() {
	//vga_graphics_initialize();
}
void screen_graphics_rectangle(uint32 x, uint32 y, uint32 width, uint32 height,
		uint8 red, uint8 green, uint8 blue) {
	CUNUSED(x);
	CUNUSED(y);
	CUNUSED(width);
	CUNUSED(height);
	CUNUSED(red);
	CUNUSED(green);
	CUNUSED(blue);
	NIMPL;
//vga_graphics_rectangle(x, y, width, height, red, green, blue);
}
void screen_graphics_pixel(uint32 x, uint32 y, uint8 red, uint8 green,
		uint8 blue) {
	CUNUSED(x);
	CUNUSED(y);
	CUNUSED(red);
	CUNUSED(green);
	CUNUSED(blue);
	NIMPL;
//vga_graphics_pixel(x, y, red, green, blue);
}
void screen_graphics_flip() {
//vga_graphics_flip();
	NIMPL;
}

void screen_graphicsMode() {
	//vga_graphicsMode();
}

// ---- general ----

void screen_init() {
	screen_terminal_init();
	screen_graphics_init();
}
