/***************************************************************************

	systems/apple2gs.c

	Apple IIgs
	
    TODO:
    - Implement Apple IIgs sound
    - Fix spurrious interrupt problem
    - Fix 5.25" disks
    - Optimize video code
    - More RAM configurations

***************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"
#include "includes/apple2gs.h"
#include "machine/sonydriv.h"


static struct GfxLayout apple2gs_text_layout =
{
	14,8,		/* 14*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },   /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxLayout apple2gs_dbltext_layout =
{
	7,8,		/* 7*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },    /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxDecodeInfo apple2gs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &apple2gs_text_layout, 0, 2 },
	{ REGION_GFX1, 0x0000, &apple2gs_dbltext_layout, 0, 2 },
	{ -1 } /* end of array */
};



static const unsigned char apple2gs_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xD0, 0x00, 0x30,	/* Dark Red */
	0x00, 0x00, 0x90,	/* Dark Blue */
	0xD0, 0x20, 0xD0,	/* Purple */
	0x00, 0x70, 0x20,	/* Dark Green */
	0x50, 0x50, 0x50,	/* Dark Grey */
	0x20, 0x20, 0xF0,	/* Medium Blue */
	0x60, 0xA0, 0xF0,	/* Light Blue */
	0x80, 0x50, 0x00,	/* Brown */
	0xF0, 0x60, 0x00,	/* Orange */
	0xA0, 0xA0, 0xA0,	/* Light Grey */
	0xF0, 0x90, 0x80,	/* Pink */
	0x10, 0xD0, 0x00,	/* Light Green */
	0xF0, 0xF0, 0x00,	/* Yellow */
	0x40, 0xF0, 0x90,	/* Aquamarine */
	0xF0, 0xF0, 0xF0	/* White */
};

MACHINE_DRIVER_EXTERN( apple2e );
INPUT_PORTS_EXTERN( apple2ep );

INPUT_PORTS_START( apple2gs )
	PORT_INCLUDE( apple2ep )

	PORT_START_TAG("adb_mouse_x")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_1_BUTTON2) PORT_NAME("Mouse Button 1")

	PORT_START_TAG("adb_mouse_y")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_1_BUTTON1) PORT_NAME("Mouse Button 0")

INPUT_PORTS_END



/* Initialize the palette */
static PALETTE_INIT( apple2gs )
{
	extern PALETTE_INIT( apple2 );
	palette_init_apple2(colortable, color_prom);
	palette_set_colors(0, apple2gs_palette, sizeof(apple2gs_palette) / 3);
}

static MACHINE_DRIVER_START( apple2gs )
	MDRV_IMPORT_FROM( apple2e )
	MDRV_CPU_REPLACE("main", G65816, 1021800)

	MDRV_CPU_VBLANK_INT(apple2gs_interrupt, 200/8)

	MDRV_SCREEN_SIZE(640, 200)
	MDRV_VISIBLE_AREA(0,639,0,199)
	MDRV_PALETTE_LENGTH( 16+256 )
	MDRV_GFXDECODE( apple2gs_gfxdecodeinfo )

	MDRV_PALETTE_INIT( apple2gs )
	MDRV_VIDEO_START( apple2gs )
	MDRV_VIDEO_UPDATE( apple2gs )

	MDRV_NVRAM_HANDLER( apple2gs )
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2gs)
	ROM_REGION(0x1000,REGION_GFX1,0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("rom03", 0x0000, 0x40000, CRC(de7ddf29) SHA1(bc32bc0e8902946663998f56aea52be597d9e361))
ROM_END

ROM_START(apple2g1)
	ROM_REGION(0x1000,REGION_GFX1,0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("rom01", 0x0000, 0x20000, CRC(42f124b0) SHA1(e4fc7560b69d062cb2da5b1ffbe11cd1ca03cc37))
ROM_END



/* -----------------------------------------------------------------------
 * Floppy disk devices
 * ----------------------------------------------------------------------- */

static const char *apple2gs_floppy35_getname(const struct IODevice *dev,
	int id, char *buf, size_t bufsize)
{
	snprintf(buf, bufsize, "Slot 5 Disk #%d", id + 1);
	return buf;
}




static const char *apple2gs_floppy525_getname(const struct IODevice *dev,
	int id, char *buf, size_t bufsize)
{
	snprintf(buf, bufsize, "Slot 6 Disk #%d", id + 1);
	return buf;
}



static void apple2gs_floppy35_getinfo(struct IODevice *dev)
{
	/* 3.5" floppy */
	sonydriv_device_getinfo(dev, SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K | SONY_FLOPPY_SUPPORT2IMG);
	dev->name = apple2gs_floppy35_getname;
}



static void apple2gs_floppy525_getinfo(struct IODevice *dev)
{
	/* 5.25" floppy */
	floppy_device_getinfo(dev, floppyoptions_apple2);
	dev->count = 2;
	dev->name = apple2gs_floppy525_getname;
	dev->tag = APDISK_DEVTAG;
}



/* ----------------------------------------------------------------------- */

SYSTEM_CONFIG_START(apple2gs)
	CONFIG_DEVICE(apple2gs_floppy35_getinfo)
	CONFIG_DEVICE(apple2gs_floppy525_getinfo)

	CONFIG_QUEUE_CHARS			( AY3600 )
	CONFIG_ACCEPT_CHAR			( AY3600 )
	CONFIG_RAM_DEFAULT			(2 * 1024 * 1024)
SYSTEM_CONFIG_END

COMPX( 1989, apple2gs, 0,        apple2,	apple2gs, apple2gs,   apple2gs, apple2gs,	"Apple Computer", "Apple IIgs (ROM03)",			GAME_NOT_WORKING )
COMPX( 1987, apple2g1, apple2gs, 0,			apple2gs, apple2gs,   apple2gs, apple2gs,	"Apple Computer", "Apple IIgs (ROM01)",			GAME_NOT_WORKING )
