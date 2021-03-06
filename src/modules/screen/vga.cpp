#include <modules/screen/vga.h>

#include <int.h>
#include <bool.h>

#include <kernel/ports.h>
#include <utils/utils.h>

// TODO externalize this
static uint8 TAB_SIZE = 2;

// ---- terminal ----

static size_t terminalRow;
static size_t terminalColumn;
static uint8 terminalColor;
static uint16* terminalBuffer;

static uint16 entry(char c, uint8 color) {
	uint16 c16 = c;
	uint16 color16 = color;
	return c16 | color16 << 8;
}

static void putEntryAt(char c, uint8 color, size_t x, size_t y) {
	const size_t index = y * VGA_TEXT_WIDTH + x;
	terminalBuffer[index] = entry(c, color);
}

static void shiftUp() {
	for (unsigned int i = VGA_TEXT_WIDTH; i < VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT;
			i += VGA_TEXT_WIDTH) {
		memcpy((uint8*) &terminalBuffer[i - VGA_TEXT_WIDTH],
				(uint8*) &terminalBuffer[i], VGA_TEXT_WIDTH * 2);
	}
}

static void updateCursor() {
	unsigned short position = (terminalRow * VGA_TEXT_WIDTH) + terminalColumn;

	// cursor LOW port to vga INDEX register
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char) (position & 0xFF));
	// cursor HIGH port to vga INDEX register
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char) ((position >> 8) & 0xFF));
}

static void backup() {
	if (--terminalColumn == (unsigned) -1) {
		terminalColumn = VGA_TEXT_WIDTH - 1;
		if (--terminalRow == (unsigned) -1) {
			terminalRow = VGA_TEXT_HEIGHT - 1;
		}
	}
}

static void putChar(char c) {
	if (c == '\n') {
		// new line
		terminalColumn = VGA_TEXT_WIDTH - 1;
		putChar(' ');
		return;
	} else if (c == 8) {
		// backspace
		backup();
		putChar(' ');
		backup();
	} else if (c == 9) {
		// tab
		for (int i = 0; i < TAB_SIZE; i++) { // FIXME backspacing will only delete part of the tab, not a real tab
			putChar(' ');
		}
		return;
	} else {
		// regular characters
		putEntryAt(c, terminalColor, terminalColumn, terminalRow);
		if (++terminalColumn == VGA_TEXT_WIDTH) {
			terminalColumn = 0;
			if (++terminalRow == VGA_TEXT_HEIGHT) {
				shiftUp();
				terminalRow--;
				for (unsigned int column = 0; column < VGA_TEXT_WIDTH;
						column++) {
					putEntryAt(' ', terminalColor, column, terminalRow);
				}
			}
		}
	}
	updateCursor();
}

// public API

uint8 vga_terminal_makeColor(VGATextColor fg, VGATextColor bg) {
	return fg | bg << 4;
}

void vga_terminal_backgroundColor(uint8 color) {
	terminalColor = color;
}

void vga_terminal_writeString(const char* data) {
	size datalen = stringlength((char*) data);
	for (size i = 0; i < datalen; i++) {
		char c = data[i];
		putChar(c);
	}
}

void vga_terminal_initialize() {
	terminalRow = 0;
	terminalColumn = 0;
	terminalColor = vga_terminal_makeColor(COLOR_LIGHT_GREY, COLOR_BLACK);
	terminalBuffer = (uint16*) 0xB8000;
	for (size y = 0; y < VGA_TEXT_HEIGHT; y++) {
		for (size x = 0; x < VGA_TEXT_WIDTH; x++) {
			const size index = y * VGA_TEXT_WIDTH + x;
			terminalBuffer[index] = entry(' ', terminalColor);
		}
	}
}

// ---- graphics ----

static uint8* graphicsBuffer;
static uint8 graphicsDoubleBuffer[VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT];

static void setColor(uint32 index, uint8 red, uint8 green, uint8 blue) {
	uint8 r = red / 64;
	uint8 g = green / 64;
	uint8 b = blue / 64;

	uint8 color = 0;

	color = changeBit(color, 0, getBit(b, 1));
	color = changeBit(color, 1, getBit(g, 1));
	color = changeBit(color, 2, getBit(r, 1));

	color = changeBit(color, 3, getBit(b, 0));
	color = changeBit(color, 4, getBit(g, 0));
	color = changeBit(color, 5, getBit(r, 0));

	graphicsDoubleBuffer[index] = color;
}

static void vsync() {
	/* wait until any previous retrace has ended */
	do {
	} while (inb(0x3DA) & 8);

	/* wait until a new retrace has just begun */
	do {
	} while (!(inb(0x3DA) & 8));
}

// public API

void vga_graphics_initialize() {
	graphicsBuffer = (uint8*) 0xA0000;
}

void vga_graphics_flip() {
	vsync();

	memcpy(graphicsBuffer, graphicsDoubleBuffer,
	VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT);
}

bool vga_graphics_pixel(uint32 x, uint32 y, uint8 red, uint8 green,
		uint8 blue) {
	if (x > VGA_GRAPHICS_WIDTH)
		return false;
	if (y > VGA_GRAPHICS_HEIGHT)
		return false;

	setColor(y * VGA_GRAPHICS_WIDTH + x, red, green, blue);

	return true;
}

bool vga_graphics_rectangle(uint32 x, uint32 y, uint32 width, uint32 height,
		uint8 red, uint8 green, uint8 blue) {
	if (x + width > VGA_GRAPHICS_WIDTH)
		return false;
	if (y + height > VGA_GRAPHICS_HEIGHT)
		return false;

	for (uint16 xp = x; xp < x + width; xp++) {
		for (uint8 yp = y; yp < y + height; yp++) {
			if (!vga_graphics_pixel(xp, yp, red, green, blue)) {
				return false;
			}
		}
	}

	//vga_graphics_flip(); // TODO Make this an option...maybe?

	return true;
}

// TODO make the stuff below make sense

#define	VGA_AC_INDEX		0x3C0
#define	VGA_AC_WRITE		0x3C0
#define	VGA_AC_READ		0x3C1
#define	VGA_MISC_WRITE		0x3C2
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5
#define	VGA_DAC_READ_INDEX	0x3C7
#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9
#define	VGA_MISC_READ		0x3CC
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF
/*			COLOR emulation		MONO emulation */
#define VGA_CRTC_INDEX		0x3D4		/* 0x3B4 */
#define VGA_CRTC_DATA		0x3D5		/* 0x3B5 */
#define	VGA_INSTAT_READ		0x3DA

#define	VGA_NUM_SEQ_REGS	5
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		9
#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_REGS		(1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
				VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

#define VGA_320_200_256 vga_320x200x256

unsigned char vga_320x200x256[] = {
/* MISC */
0x63,
/* SEQ */
0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF,
		/* GC */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,
		/* AC */
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
		0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00 };

static void write_regs(unsigned char *regs) {
	unsigned i;

	/* write MISCELLANEOUS reg */
	outb(VGA_MISC_WRITE, *regs);
	regs++;
	/* write SEQUENCER regs */
	for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
		outb(VGA_SEQ_INDEX, i);
		outb(VGA_SEQ_DATA, *regs);
		regs++;
	}
	/* unlock CRTC registers */
	outb(VGA_CRTC_INDEX, 0x03);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
	outb(VGA_CRTC_INDEX, 0x11);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
	/* make sure they remain unlocked */
	regs[0x03] |= 0x80;
	regs[0x11] &= ~0x80;
	/* write CRTC regs */
	for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
		outb(VGA_CRTC_INDEX, i);
		outb(VGA_CRTC_DATA, *regs);
		regs++;
	}
	/* write GRAPHICS CONTROLLER regs */
	for (i = 0; i < VGA_NUM_GC_REGS; i++) {
		outb(VGA_GC_INDEX, i);
		outb(VGA_GC_DATA, *regs);
		regs++;
	}
	/* write ATTRIBUTE CONTROLLER regs */
	for (i = 0; i < VGA_NUM_AC_REGS; i++) {
		(void) inb(VGA_INSTAT_READ);
		outb(VGA_AC_INDEX, i);
		outb(VGA_AC_WRITE, *regs);
		regs++;
	}
	/* lock 16-color palette and unblank display */
	(void) inb(VGA_INSTAT_READ);
	outb(VGA_AC_INDEX, 0x20);
}

void vga_graphicsMode() {
	write_regs(VGA_320_200_256);

	vga_graphics_rectangle(0, 0, VGA_GRAPHICS_WIDTH, VGA_GRAPHICS_HEIGHT, 0, 0,
			0);
}
