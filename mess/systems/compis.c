/******************************************************************************

	systems/compis.c
	machine driver

	Per Ola Ingvarsson
	Tomas Karlsson

	Hardware:
		- Intel 80186 CPU 8MHz, integrated DMA(8237?), PIC(8259?), PIT(8253?)
                - Intel 80130 OSP Operating system processor (PIC 8259, PIT 8254)
		- Intel 8274 MPSC Multi-protocol serial communications controller (NEC 7201)
		- Intel 8255 PPI Programmable peripheral interface 
		- Intel 8253 PIT Programmable interval timer
		- Intel 8251 USART Universal synchronous asynchronous receiver transmitter
		- National 58174 Real-time clock (compatible with 58274)
	Peripheral:
		- Intel 82720 GDC Graphic display processor (NEC uPD 7220)
		- Intel 8272 FDC Floppy disk controller (Intel iSBX-218A)
		- Western Digital WD1002-05 Winchester controller

	Memory map:

	00000-3FFFF	RAM	LMCS (Low Memory Chip Select)
	40000-4FFFF	RAM	MMCS 0 (Midrange Memory Chip Select)
	50000-5FFFF	RAM	MMCS 1 (Midrange Memory Chip Select)
	60000-6FFFF	RAM	MMCS 2 (Midrange Memory Chip Select)
	70000-7FFFF	RAM	MMCS 3 (Midrange Memory Chip Select)
	80000-EFFFF	NOP
	F0000-FFFFF	ROM	UMCS (Upper Memory Chip Select)

 ******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/i82720.h"
#include "includes/compis.h"
#include "devices/mflopimg.h"
#include "devices/printer.h"
#include "formats/cpis_dsk.h"
#include "cpuintrf.h"

static ADDRESS_MAP_START( compis_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x3ffff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x40000, 0x4ffff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x50000, 0x5ffff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x60000, 0x6ffff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x70000, 0x7ffff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x80000, 0xeffff) AM_READ( MRA8_NOP )
	AM_RANGE( 0xf0000, 0xfffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( compis_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x3ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x40000, 0x4ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x50000, 0x5ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x60000, 0x6ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x70000, 0x7ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x80000, 0xeffff) AM_WRITE( MWA8_NOP )
	AM_RANGE( 0xf0000, 0xfffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( compis_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x0001, 0x0008) AM_READ( compis_ppi_r )	/* PPI 8255			*/
	AM_RANGE( 0x0080, 0x0087) AM_READ( compis_pit_r )	/* PIT 8253			*/
	AM_RANGE( 0x0100, 0x011a) AM_READ( compis_rtc_r ) 	/* RTC 58174			*/
	AM_RANGE( 0x0280, 0x0282) AM_READ( compis_osp_pic_r ) /* PIC 8259 (80150/80130)	*/
//  { 0x0288, 0x028e, compis_osp_pit_r },	/* PIT 8254 (80150/80130)	*/
	AM_RANGE( 0x0311, 0x031f) AM_READ( compis_usart_r )	/* USART 8251 Keyboard		*/
	AM_RANGE( 0x0330, 0x033e) AM_READ( compis_gdc_r )	/* GDC 82720 PCS6:6		*/
	AM_RANGE( 0x0340, 0x0342) AM_READ( compis_fdc_r )	/* iSBX0 (J8) FDC 8272		*/
	AM_RANGE( 0x0351, 0x0351) AM_READ( compis_fdc_dack_r)	/* iSBX0 (J8) DMA ACK		*/
	AM_RANGE( 0xff00, 0xffff) AM_READ( i186_internal_port_r)/* CPU 80186			*/
//{ 0x0100, 0x017e, compis_null_r },	/* RTC				*/
//{ 0x0180, 0x01ff, compis_null_r },	/* PCS3?			*/
//{ 0x0200, 0x027f, compis_null_r },	/* Reserved			*/
//{ 0x0280, 0x02ff, compis_null_r },	/* 80150 not used?		*/
//{ 0x0300, 0x0300, compis_null_r },	/* Cassette  motor		*/
//{ 0x0301, 0x030f, compis_null_r}, 	/* DMA ACK Graphics		*/
//{ 0x0310, 0x031e, compis_null_r },	/* SCC 8274 Int Ack		*/
//{ 0x0320, 0x0320, compis_null_r },	/* SCC 8274 Serial port		*/
//{ 0x0321, 0x032f, compis_null_r },	/* DMA Terminate		*/
//{ 0x0331, 0x033f, compis_null_r },	/* DMA Terminate		*/
//{ 0x0341, 0x034f, compis_null_r },	/* J8 CS1 (16-bit)		*/
//{ 0x0350, 0x035e, compis_null_r },	/* J8 CS1 (8-bit)		*/
//{ 0x0360, 0x036e, compis_null_r },	/* J9 CS0 (8/16-bit)		*/
//{ 0x0361, 0x036f, compis_null_r },	/* J9 CS1 (16-bit)		*/
//{ 0x0370, 0x037e, compis_null_r },	/* J9 CS1 (8-bit)		*/
//{ 0x0371, 0x037f, compis_null_r },	/* J9 CS1 (8-bit)		*/
//{ 0xff20, 0xffff, compis_null_r },	/* CPU 80186			*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( compis_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x0001, 0x0008) AM_WRITE( compis_ppi_w )	/* PPI 8255			*/
	AM_RANGE( 0x0080, 0x0087) AM_WRITE( compis_pit_w )	/* PIT 8253			*/
	AM_RANGE( 0x0108, 0x011c) AM_WRITE( compis_rtc_w )	/* RTC 58174			*/
	AM_RANGE( 0x0280, 0x0282) AM_WRITE( compis_osp_pic_w )	/* PIC 8259 (80150/80130)	*/
// { 0x0288, 0x028e, compis_osp_pit_w },	/* PIT 8254 (80150/80130)	*/
	AM_RANGE( 0x0311, 0x031f) AM_WRITE( compis_usart_w )	/* USART 8251 Keyboard		*/
	AM_RANGE( 0x0330, 0x033e) AM_WRITE( compis_gdc_w )	/* GDC 82720 PCS6:6		*/
	AM_RANGE( 0x0340, 0x0342) AM_WRITE( compis_fdc_w )	/* FDC 8272			*/
	AM_RANGE( 0xff00, 0xffff) AM_WRITE( i186_internal_port_w)/* CPU 80186			*/
ADDRESS_MAP_END

/* COMPIS Keyboard */
INPUT_PORTS_START (compis)
	PORT_START /* 0 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "Esc", KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  ", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 &", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 /", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 (", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 )", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 =", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "+ ?", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "� `", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE )

	PORT_START /* 1 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "� �", KEYCODE_OPENBRACE, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "� �", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D", KEYCODE_D, IP_JOY_NONE )

	PORT_START /* 2 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "� �", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "� �", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "'' *", KEYCODE_TILDE, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "LSHIFT", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "< >", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE )

	PORT_START /* 3 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, ", ;", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, ". :", KEYCODE_STOP, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  ", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "RSHIFT", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "LSSHIFT", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "LCTRL", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "RCTRL", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "RSSHIFT", KEYCODE_RALT, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "INPASSA", KEYCODE_INSERT, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "S�K", KEYCODE_PRTSCR, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD, "UTPL�NA", KEYCODE_DEL, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD, "START-STOP", KEYCODE_PAUSE, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE )

	PORT_START /* 4 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "AVBRYT", KEYCODE_SCRLOCK, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOME", KEYCODE_HOME, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "TABL", KEYCODE_PGUP, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "TABR", KEYCODE_PGDN, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "COMPIS !", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "COMPIS ?", KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "COMPIS |", KEYCODE_F5, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "COMPIS S", KEYCODE_NUMLOCK, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 7", KEYCODE_7_PAD, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 8", KEYCODE_8_PAD, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 9", KEYCODE_9_PAD, IP_JOY_NONE )

	PORT_START /* 5 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 4", KEYCODE_4_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 5", KEYCODE_5_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 6", KEYCODE_6_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 1", KEYCODE_1_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 2", KEYCODE_2_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 3", KEYCODE_3_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 0", KEYCODE_0_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 00", KEYCODE_SLASH_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP 000", KEYCODE_ASTERISK, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP ENTER", KEYCODE_ENTER_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP ,", KEYCODE_DEL_PAD, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP -", KEYCODE_MINUS_PAD, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD, "KP +", KEYCODE_PLUS_PAD, IP_JOY_NONE )

	PORT_START /* 6 */
	PORT_DIPNAME( 0x18, 0x00, "S8 Test mode")
	PORT_DIPSETTING( 0x00, "Normal" )
	PORT_DIPSETTING( 0x08, "Remote" )
	PORT_DIPSETTING( 0x10, "Stand alone" )
	PORT_DIPSETTING( 0x18, "Reserved" )

	PORT_START /* 7 */
	PORT_DIPNAME( 0x01, 0x00, "iSBX-218A DMA")
	PORT_DIPSETTING( 0x01, "Enabled" )
  PORT_DIPSETTING( 0x00, "Disabled" )
INPUT_PORTS_END

static unsigned i86_address_mask = 0x000fffff;

static const compis_gdc_interface i82720_interface =
{
	GDC_MODE_HRG,
	0x8000
};

static MACHINE_DRIVER_START( compis )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", I186, 8000000)	/* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(compis_readmem, compis_writemem)
	MDRV_CPU_IO_MAP(compis_readport, compis_writeport)
	MDRV_CPU_VBLANK_INT(compis_vblank_int, 1)
	MDRV_CPU_CONFIG(i86_address_mask)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT(compis)

	MDRV_COMPISGDC( &i82720_interface )

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (compis)
     ROM_REGION (0x100000, REGION_CPU1, 0)
     ROM_LOAD ("compis.rom", 0xf0000, 0x10000, CRC(89877688) SHA1(7daa1762f24e05472eafc025879da90fe61d0225))
ROM_END

SYSTEM_CONFIG_START(compis)
	CONFIG_DEVICE_PRINTER	(1)
	CONFIG_DEVICE_FLOPPY	(2,	compis)
SYSTEM_CONFIG_END

/*   YEAR	NAME		PARENT	COMPAT MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
COMP(1985,	compis,		0,		0,     compis,	compis,	compis,	compis,	"Telenova", "Compis" )
