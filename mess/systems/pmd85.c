/*******************************************************************************

PMD-85 driver by Krzysztof Strzecha

What's new:
-----------


15.03.2002	Added drivers for: PMD-85.2, PMD-85.2A, PMD-85.2B and Didaktik
		Alfa (PMD-85.1 clone). Keyboard finished. Natural keyboard added.
		Memory system rewritten. I/O system rewritten. Support for Basic
		ROM module added. Video emulation rewritten.
30.11.2002	Memory mapping improved.
06.07.2002	Preliminary driver.

Notes on emulation status and to do list:
-----------------------------------------
1. Tape and V.24 (8251)
2. Support for .pmd tape
3. 8253 (needed by at least one game 'Hlipa')
4. PMD-85.3 and Mato drivers
5. Flash video attribute
6. External interfaces connectors (K2-K5)
7. Speaker
8. LEDs

PMD-85 technical information
============================

Memory map:
-----------

	PMD-85.1, PMD-85.2
	------------------

	start-up map (cleared by the first I/O write operation done by the CPU):
	0000-0fff ROM mirror #1
	1000-1fff not mapped
	2000-2fff ROM mirror #2
	3000-3fff not mapped
	4000-7fff Video RAM mirror #1
	8000-8fff ROM
	9000-9fff not mapped
	a000-afff ROM mirror #3
	b000-bfff not mapped
	c000-ffff Video RAM

	normal map:
	0000-7fff RAM
	8000-8fff ROM
	9000-9fff not mapped
	a000-afff ROM mirror #1
	b000-bfff not mapped
	c000-ffff Video RAM

	Didaktik Alfa (PMD-85.1 clone)
	------------------------------

	start-up map (cleared by the first I/O write operation done by the CPU):
	0000-0fff ROM mirror
	1000-33ff BASIC mirror
	3400-3fff not mapped
	4000-7fff Video RAM mirror
	8000-8fff ROM
	9000-b3ff BASIC
	b400-bfff not mapped
	c000-ffff Video RAM

	normal map:
	0000-7fff RAM
	8000-8fff ROM
	9000-b3ff BASIC
	b400-bfff not mapped
	c000-ffff Video RAM

	PMD-85.2A, PMD-85.2B
	--------------------

	start-up map (cleared by the first I/O write operation done by the CPU):
	0000-0fff ROM mirror #1
	1000-1fff RAM #2 mirror
	2000-2fff ROM mirror #2
	3000-3fff RAM #3 mirror
	4000-7fff Video RAM mirror #1
	8000-8fff ROM
	9000-9fff RAM #2
	a000-afff ROM mirror #3
	b000-bfff RAM #3
	c000-ffff Video RAM

	normal map:
	0000-7fff RAM #1
	8000-8fff ROM
	9000-9fff RAM #2
	a000-afff ROM mirror #1
	b000-bfff RAM #3
	c000-ffff Video RAM

I/O ports
---------

	I/O board
	---------
	1xxx11aa	external interfaces connector (K2)
					
	0xxx11aa	I/O board interfaces
		000111aa	8251 (casette recorder, V24)
		010011aa	8255 (GPIO/0, GPIO/1)
		010111aa	8253
		011111aa	8255 (IMS-2)

	Motherboard
	-----------
	1xxx01aa	8255 (keyboard, speaker. LEDS)

	ROM Module
	----------
	1xxx10aa	8255 (ROM reading)
	ROM module is not supported by Didaktik Alfa.

*******************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/i8085/i8085.h"
#include "vidhrdw/generic.h"
#include "includes/pmd85.h"

/* I/O ports */

ADDRESS_MAP_START( pmd85_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_READ( pmd85_io_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( pmd85_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_WRITE( pmd85_io_w )
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( pmd85_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x1000, 0x1fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x2000, 0x2fff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0x3000, 0x3fff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK5, MWA8_BANK5)
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(MRA8_BANK6, MWA8_BANK6)
	AM_RANGE(0x9000, 0x9fff) AM_READWRITE(MRA8_BANK7, MWA8_BANK7)
	AM_RANGE(0xa000, 0xafff) AM_READWRITE(MRA8_BANK8, MWA8_BANK8)
	AM_RANGE(0xb000, 0xbfff) AM_READWRITE(MRA8_BANK9, MWA8_BANK9)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK10, MWA8_BANK10)
ADDRESS_MAP_END

ADDRESS_MAP_START( alfa_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x1000, 0x33ff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x3400, 0x3fff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(MRA8_BANK5, MWA8_BANK5)
	AM_RANGE(0x9000, 0xb3ff) AM_READWRITE(MRA8_BANK6, MWA8_BANK6)
	AM_RANGE(0xb400, 0xbfff) AM_READWRITE(MRA8_BANK7, MWA8_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK8, MWA8_BANK8)
ADDRESS_MAP_END

/* keyboard input */

#define PMD85_KEYBOARD_MAIN											\
	PORT_START /* port 0x00 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K0",	KEYCODE_ESC,	CODE_NONE,	UCHAR_MAMEKEY(F1))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "1 !",	KEYCODE_1,	CODE_NONE,	'1',	'!')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "Q",	KEYCODE_Q,	CODE_NONE,	'q')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "A",	KEYCODE_A,	CODE_NONE,	'a')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Space",	KEYCODE_SPACE,CODE_NONE,	' ')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x01 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K1",	KEYCODE_F1,	CODE_NONE,	UCHAR_MAMEKEY(F2))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "2 \"",	KEYCODE_2,	CODE_NONE,	'1',	'\"')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "W",	KEYCODE_W,	CODE_NONE,	'w')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "S",	KEYCODE_S,	CODE_NONE,	's')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Y",	KEYCODE_Z,	CODE_NONE,	'z')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x02 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K2",	KEYCODE_F2,	CODE_NONE,	UCHAR_MAMEKEY(F3))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "3 #",	KEYCODE_3,	CODE_NONE,	'3',	'#')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "E",	KEYCODE_E,	CODE_NONE,	'e')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "D",	KEYCODE_D,	CODE_NONE,	'd')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "X",	KEYCODE_X,	CODE_NONE,	'x')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x03 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K3",	KEYCODE_F3,	CODE_NONE,	UCHAR_MAMEKEY(F4))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "4 $",	KEYCODE_4,	CODE_NONE,	'4',	'$')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "R",	KEYCODE_R,	CODE_NONE,	'r')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "F",	KEYCODE_F,	CODE_NONE,	'f')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "C",	KEYCODE_C,	CODE_NONE,	'c')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x04 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K4",	KEYCODE_F4,	CODE_NONE,	UCHAR_MAMEKEY(F5))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "5 %",	KEYCODE_5,	CODE_NONE,	'5',	'%')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "T",	KEYCODE_T,	CODE_NONE,	't')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "G",	KEYCODE_G,	CODE_NONE,	'g')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "V",	KEYCODE_V,	CODE_NONE,	'v')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x05 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K5",	KEYCODE_F5,	CODE_NONE,	UCHAR_MAMEKEY(F6))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "6 &",	KEYCODE_6,	CODE_NONE,	'6',	'&')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "Z",	KEYCODE_Y,	CODE_NONE,	'y')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "H",	KEYCODE_H,	CODE_NONE,	'h')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "B",	KEYCODE_B,	CODE_NONE,	'b')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x06 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K6",	KEYCODE_F6,	CODE_NONE,	UCHAR_MAMEKEY(F7))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "7 \'",	KEYCODE_7,	CODE_NONE,	'7',	'\'')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "U",	KEYCODE_U,	CODE_NONE,	'u')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "J",	KEYCODE_J,	CODE_NONE,	'j')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "N",	KEYCODE_N,	CODE_NONE,	'n')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x07 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K7",	KEYCODE_F7,	CODE_NONE,	UCHAR_MAMEKEY(F8))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "8 (",	KEYCODE_8,	CODE_NONE,	'8',	'(')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "I",	KEYCODE_I,	CODE_NONE,	'i')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "K",	KEYCODE_K,	CODE_NONE,	'k')			\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "M",	KEYCODE_M,	CODE_NONE,	'm')			\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x08 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K8",	KEYCODE_F8,	CODE_NONE,	UCHAR_MAMEKEY(F9))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "9 )",	KEYCODE_9,	CODE_NONE,	'9',	')')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "O",	KEYCODE_O,	CODE_NONE,	'0')			\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "L",	KEYCODE_L,	CODE_NONE,	'l')			\
		PORT_KEY2(0x10, IP_ACTIVE_LOW, ", <",	KEYCODE_COMMA,	CODE_NONE,	',',	'<')		\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x09 */										\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K9",	KEYCODE_F9,	CODE_NONE,	UCHAR_MAMEKEY(F10))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "0 -",	KEYCODE_0,	CODE_NONE,	'0',	'-')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "P",	KEYCODE_P,	CODE_NONE,	'p')			\
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "; +",	KEYCODE_COLON,	CODE_NONE,	';',	'+')		\
		PORT_KEY2(0x10, IP_ACTIVE_LOW, ". >",	KEYCODE_STOP,	CODE_NONE,	'.',	'>')		\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)							\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)							\
	PORT_START /* port 0x0a */											\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K10",	KEYCODE_F10,		CODE_NONE,	UCHAR_MAMEKEY(F11))	\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "_ =",	KEYCODE_MINUS,		CODE_NONE,	'_',	'=')		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "@",	KEYCODE_OPENBRACE,	CODE_NONE,	'@')			\
		PORT_KEY2(0x08, IP_ACTIVE_LOW, ": *",	KEYCODE_QUOTE,		CODE_NONE,	':',	'*')		\
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "/ ?",	KEYCODE_SLASH,		CODE_NONE,	'/',	'?')		\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)								\
	PORT_START /* port 0x0b */											\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "K11",	KEYCODE_F11,		CODE_NONE,	UCHAR_MAMEKEY(F12))	\
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "Blank",	KEYCODE_EQUALS,		CODE_NONE,	UCHAR_MAMEKEY(ESC))	\
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "\\ ^",	KEYCODE_CLOSEBRACE,	CODE_NONE,	'\\',	'^')		\
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "[ ]",	KEYCODE_BACKSLASH,	CODE_NONE,	'[',	']')		\
		PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)								\
	PORT_START /* port 0x0c */														\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "WRK",		KEYCODE_INSERT,	CODE_NONE,	UCHAR_MAMEKEY(RCONTROL))			\
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "INS PTL",	KEYCODE_DEL,	CODE_NONE,	UCHAR_MAMEKEY(INSERT),	UCHAR_MAMEKEY(TAB))	\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "<-",		KEYCODE_LEFT,	CODE_NONE,	UCHAR_MAMEKEY(LEFT))				\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "|<-",		KEYCODE_LALT,	CODE_NONE,	UCHAR_MAMEKEY(LALT))				\
		PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)											\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)											\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)											\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)											\
	PORT_START /* port 0x0d */											\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "C-D",	KEYCODE_HOME,	CODE_NONE,	UCHAR_MAMEKEY(CAPSLOCK))	\
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "DEL",	KEYCODE_END,	CODE_NONE,	UCHAR_MAMEKEY(DEL))		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "^\\",	KEYCODE_UP,	CODE_NONE,	UCHAR_MAMEKEY(HOME))		\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "END",	KEYCODE_DOWN,	CODE_NONE,	UCHAR_MAMEKEY(END))		\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "EOL1",	KEYCODE_ENTER,	CODE_NONE,	UCHAR_MAMEKEY(ENTER))		\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)								\
	PORT_START /* port 0x0e */											\
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "CLR",	KEYCODE_PGUP,	CODE_NONE,	UCHAR_MAMEKEY(PGUP))		\
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "RCL",	KEYCODE_PGDN,	CODE_NONE,	UCHAR_MAMEKEY(PGDN))		\
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "->",	KEYCODE_RIGHT,	CODE_NONE,	UCHAR_MAMEKEY(RIGHT))		\
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "->|",	KEYCODE_RALT,	CODE_NONE,	UCHAR_MAMEKEY(RALT))		\
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "EOL2",	KEYCODE_TAB,	CODE_NONE,	UCHAR_MAMEKEY(ENTER_PAD))	\
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)								\
	PORT_START /* port 0x0f */											\
		PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)								\
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "Shift",	KEYCODE_LSHIFT,		CODE_NONE,	UCHAR_MAMEKEY(LSHIFT))	\
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "Shift",	KEYCODE_RSHIFT,		CODE_NONE,	UCHAR_MAMEKEY(RSHIFT))	\
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "Stop",	KEYCODE_LCONTROL,	CODE_NONE,	UCHAR_MAMEKEY(LCONTROL))\
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)

#define PMD85_KEYBOARD_RESET												\
	PORT_START /* port 0x10 */											\
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_RESETCPU, "RST", KEYCODE_BACKSPACE, IP_JOY_NONE )

#define PMD85_DIPSWITCHES						\
	PORT_START /* port 0x11 */					\
		PORT_CONFNAME( 0x01, 0x00, "Basic ROM Module" )		\
			PORT_CONFSETTING( 0x00, DEF_STR( Off ) )	\
			PORT_CONFSETTING( 0x01, DEF_STR( On ) )		\
		PORT_CONFNAME( 0x02, 0x00, "Tape/V.24" )		\
			PORT_CONFSETTING( 0x00, "Tape" )		\
			PORT_CONFSETTING( 0x02, "V.24" )

#define ALFA_DIPSWITCHES						\
	PORT_START /* port 0x11 */					\
		PORT_CONFNAME( 0x02, 0x00, "Tape/V.24" )		\
			PORT_CONFSETTING( 0x00, "Tape" )		\
			PORT_CONFSETTING( 0x02, "V.24" )

INPUT_PORTS_START (pmd85)
	PMD85_KEYBOARD_MAIN
	PMD85_KEYBOARD_RESET
	PMD85_DIPSWITCHES
INPUT_PORTS_END

INPUT_PORTS_START (alfa)
	PMD85_KEYBOARD_MAIN
	PMD85_KEYBOARD_RESET
	ALFA_DIPSWITCHES
INPUT_PORTS_END

/* machine definition */
static MACHINE_DRIVER_START( pmd85 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080, 2000000)
	MDRV_CPU_PROGRAM_MAP(pmd85_mem, 0)
	MDRV_CPU_IO_MAP(pmd85_readport, pmd85_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( pmd85 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(288, 256)
	MDRV_VISIBLE_AREA(0, 288-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(sizeof (pmd85_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (pmd85_colortable))
	MDRV_PALETTE_INIT( pmd85 )

	MDRV_VIDEO_START( pmd85 )
	MDRV_VIDEO_UPDATE( pmd85 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( alfa )
	MDRV_IMPORT_FROM( pmd85 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(alfa_mem, 0)
MACHINE_DRIVER_END

ROM_START(pmd851)
	ROM_REGION(0x11000,REGION_CPU1,0)
	ROM_LOAD("pmd85-1.bin", 0x10000, 0x1000, CRC(9bc5e6ec))
	ROM_REGION(0x2400,REGION_USER1,0)
	ROM_LOAD_OPTIONAL("pmd85-1.bas", 0x0000, 0x2400, CRC(4fc37d45))
ROM_END

ROM_START(pmd852)
	ROM_REGION(0x11000,REGION_CPU1,0)
	ROM_LOAD("pmd85-2.bin", 0x10000, 0x1000, CRC(c093474a))
	ROM_REGION(0x2400,REGION_USER1,0)
	ROM_LOAD_OPTIONAL("pmd85-2.bas", 0x0000, 0x2400, CRC(fc4a3ebf))
ROM_END

ROM_START(pmd852a)
	ROM_REGION(0x11000,REGION_CPU1,0)
	ROM_LOAD("pmd85-2a.bin", 0x10000, 0x1000, CRC(5a9a961b))
	ROM_REGION(0x2400,REGION_USER1,0)
	ROM_LOAD_OPTIONAL("pmd85-2a.bas", 0x0000, 0x2400, CRC(6ff379ad))
ROM_END

ROM_START(pmd852b)
	ROM_REGION(0x11000,REGION_CPU1,0)
	ROM_LOAD("pmd85-2a.bin", 0x10000, 0x1000, CRC(5a9a961b))
	ROM_REGION(0x2400,REGION_USER1,0)
	ROM_LOAD_OPTIONAL("pmd85-2a.bas", 0x0000, 0x2400, CRC(6ff379ad))
ROM_END

ROM_START(alfa)
	ROM_REGION(0x13400,REGION_CPU1,0)
	ROM_LOAD("alfa.bin", 0x10000, 0x1000, CRC(e425eedb))
	ROM_LOAD("alfa.bas", 0x11000, 0x2400, CRC(9a73bfd2))
ROM_END

SYSTEM_CONFIG_START(pmd85)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END


/*    YEAR  NAME     PARENT  COMPAT	MACHINE  INPUT  INIT      CONFIG COMPANY  FULLNAME */
COMP( 1989, pmd851,  0,      0,		pmd85,   pmd85, pmd851,   pmd85, "Tesla", "PMD-85.1" )
COMP( 1989, pmd852,  pmd851, 0,		pmd85,   pmd85, pmd851,   pmd85, "Tesla", "PMD-85.2" )
COMP( 1989, pmd852a, pmd851, 0,		pmd85,   pmd85, pmd852a,  pmd85, "Tesla", "PMD-85.2A" )
COMP( 1989, pmd852b, pmd851, 0,		pmd85,   pmd85, pmd852a,  pmd85, "Tesla", "PMD-85.2B" )
COMP( 1989, alfa,    pmd851, 0,		alfa,    alfa,  alfa,     pmd85, "Didaktik", "Alfa" )

