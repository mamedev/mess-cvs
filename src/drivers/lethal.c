/***************************************************************************

 Lethal Enforcers
 (c) 1992 Konami
 Driver by R. Belmont.

 This hardware is exceptionally weird - they have a bunch of chips intended
 for use with a 68000 hooked up to an 8-bit CPU.  So everything is bankswitched
 like crazy.

 LETHAL ENFORCERS
 KONAMI 1993

            84256         053245A
                191A03.A4     6116     191A04.A8 191A05.A10
                     054539             053244A   053244A
       Z80B  191A02.F4
                6116
                                          191A06.C9
                               7C185
                               7C185
                007644            5116
                054000            4464
                4464              4464
                191E01.U4         5116      054157  054157
                63C09EP           054156
       24MHZ                                 191A07.V8 191A08.V10
                                             191A09.V9 191A10.V10



---


Lethal Enforcers (c) Konami 1992
GX191 PWB353060A

Dump of USA program ROMs only.

Label	CRC32		Location	Code		Chip Type
  1   [72b843cc]	  F4		Z80		TMS 27C512 (64k)
  6   [1b6b8f16]	  U4		6309		ST 27C4001 (512k)

At offset 0x3FD03 in 6_usa.u4 is "08/17/92 21:38"

Run down of PCB:
Main CPU:  HD63C09EP
	OSC 24.00000MHz near 6309

Sound CPU:  Z80 (Zilog Z0840006PSC)
	OSC 18.43200MHz near Z80, 054968A & 054539

Konami Custom chips:

054986A (sound GLU)
054539  (sound)
054000  (collision/protection)
053244A (x2) (sprites)
053245A (sprites)
054156 (tilemaps)
054157 (x2) (tilemaps)
007324 (???)

All other ROMs surface mounted (not included):

Label	Printed*	Position
191 A03 Mask16M-8bit - Near 054986A & 054539 - Sound - Also labeled as 056046

191A04  Mask8M-16bit \ Near 053244A (x2) & 05245A - Tiles
191A05  Mask8M-16bit /
191 A06 Mask8M-16bit - Also labeled as 056049

191A07  Mask8M-16bitx4 \
191A08  Mask8M-16bitx4  | Near 054157 (x2) & 054156 - Sprites
191A09  Mask8M-16bitx4  |
191A10  Mask8M-16bitx4 /

* This info is printed/silk-screened on to the PCB for assembly information?


4 way Dip Switch

---------------------------------------------------
 DipSwitch Title   |   Function   | 1 | 2 | 3 | 4 |
---------------------------------------------------
   Sound Output    |    Stereo    |off|           |
                   |   Monaural   |on |           |
---------------------------------------------------
  Coin Mechanism   |   Common     |   |off|       |
                   | Independent  |   |on |       |
---------------------------------------------------
    Game Type      |   Street     |       |off|   |
                   |   Arcade     |       |on |   |
---------------------------------------------------
    Language*      |   English    |           |off|
                   |   Spanish    |           |on |
---------------------------------------------------
     Default Settings             |off|off|on |off|
---------------------------------------------------
 NOTE: Listed as "NOT USED" in UK / World Manual  |
---------------------------------------------------

Push Button Test Switch



***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/hd6309/hd6309.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"

VIDEO_START(lethalen);
VIDEO_UPDATE(lethalen);

static int init_eeprom_count;
static int cur_control2;
static data8_t le_ram48[0x800];

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/* read command */
	"011100",		/* write command */
	"0100100000000",	/* erase command */
	"0100000000000",	/* lock command */
	"0100110000000" 	/* unlock command */
};

static NVRAM_HANDLER( lethalen )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ8_HANDLER( control2_r )
{
	return 0x02 | EEPROM_read_bit() | (input_port_1_r(0) & 0xf0);
}

static WRITE8_HANDLER( control2_w )
{
	/* bit 0  is data */
	/* bit 1  is cs (active low) */
	/* bit 2  is clock (active high) */
	/* bit 4  bankswitches the 4800-4fff region: 0 = registers, 1 = RAM */

	cur_control2 = data;

	EEPROM_write_bit(cur_control2 & 0x01);
	EEPROM_set_cs_line((cur_control2 & 0x02) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((cur_control2 & 0x04) ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(lethalen_interrupt)
{
	if (K056832_is_IRQ_enabled(0)) cpunum_set_input_line(0, HD6309_IRQ_LINE, HOLD_LINE);
}

static WRITE8_HANDLER( sound_cmd_w )
{
	soundlatch_w(0, data);
}

static WRITE8_HANDLER( sound_irq_w )
{
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static READ8_HANDLER( sound_status_r )
{
	return 0xf;
}

static void sound_nmi(void)
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( le_bankswitch_w )
{
	data8_t *prgrom = (data8_t *)memory_region(REGION_CPU1)+0x10000;

	cpu_setbank(1, &prgrom[data * 0x2000]);
}

static READ8_HANDLER( le_4800_r )
{
	if (cur_control2 & 0x10)	// RAM enable
	{
		return le_ram48[offset];
	}

	switch (offset)
	{
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
			return K053244_r(offset-0x40);
			break;

		case 0xca:
			return sound_status_r(0);
			break;
	}

	return 0;
}

static WRITE8_HANDLER( le_4800_w )
{
	if (cur_control2 & 0x10)	// RAM enable
	{
		le_ram48[offset] = data;
		return;
	}

	switch (offset)
	{
		case 0xc6:
			sound_cmd_w(0, data);
			break;

		case 0xc7:
			sound_irq_w(0, data);
			break;

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
			K053244_w(offset-0x40, data);
			break;

		default:
			logerror("Unknown LE 48xx register write: %x to %x\n", data, offset);
			break;
	}
}

static ADDRESS_MAP_START( le_main, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_BANK1) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_RAM				// work RAM
	AM_RANGE(0x4000, 0x403f) AM_WRITE(K056832_w)
	AM_RANGE(0x4040, 0x404f) AM_WRITE(K056832_b_w)
	AM_RANGE(0x4080, 0x4080) AM_READ(MRA8_NOP)		// watchdog
	AM_RANGE(0x40a0, 0x40a0) AM_READNOP			// gun input related?
	AM_RANGE(0x40c4, 0x40c4) AM_WRITE(control2_w)
	AM_RANGE(0x40d4, 0x40d7) AM_READNOP			// gun inputs?
	AM_RANGE(0x40d8, 0x40d8) AM_READ(control2_r)
	AM_RANGE(0x40d9, 0x40d9) AM_READ(input_port_0_r)
	AM_RANGE(0x40db, 0x40db) AM_READNOP			// gun input related?
	AM_RANGE(0x40dc, 0x40dc) AM_WRITE(le_bankswitch_w)
	AM_RANGE(0x4800, 0x4fff) AM_READWRITE(le_4800_r, le_4800_w)	// bankswitched: RAM and registers
	AM_RANGE(0x5800, 0x5fff) AM_READWRITE(paletteram_r, paletteram_xBBBBBGGGGGRRRRR_swap_w) AM_BASE(&paletteram)	AM_RANGE(0x5000, 0x57ff) AM_READWRITE(K053245_r, K053245_w)
	AM_RANGE(0x6000, 0x67ff) AM_READWRITE(K056832_ram_code_lo_r, K056832_ram_code_lo_w)
	AM_RANGE(0x6800, 0x6fff) AM_READWRITE(K056832_ram_code_hi_r, K056832_ram_code_hi_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(K056832_ram_attr_lo_r, K056832_ram_attr_lo_w)
	AM_RANGE(0x7800, 0x7fff) AM_READWRITE(K056832_ram_attr_hi_r, K056832_ram_attr_hi_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK2) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( le_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xfa2f) AM_READWRITE(K054539_0_r, K054539_0_w)
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xfc02, 0xfc02) AM_READ(soundlatch_r)
ADDRESS_MAP_END

/* sound */

INPUT_PORTS_START( lethalen )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE(0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_DIPNAME( 0x10, 0x10, DEF_STR(Language) )
        PORT_DIPSETTING(    0x10, DEF_STR(English) )
        PORT_DIPSETTING(    0x00, DEF_STR(Spanish) )
	PORT_DIPNAME( 0x20, 0x00, "Game Type" )
	PORT_DIPSETTING(    0x20, "Street" )
	PORT_DIPSETTING(    0x00, "Arcade" )
	PORT_DIPNAME( 0x40, 0x40, "Coin Mechanism" )
	PORT_DIPSETTING(    0x40, "Common" )
	PORT_DIPSETTING(    0x00, "Independent" )
	PORT_DIPNAME( 0x0080, 0x0080, "Sound Output" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Mono ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Stereo ) )
INPUT_PORTS_END

static struct K054539interface k054539_interface =
{
	1,			/* 1 chip */
	48000,
	{ REGION_SOUND1 },
	{ { 100, 100 } },
	{ 0 },
	{ sound_nmi }
};

static MACHINE_INIT( lethalen )
{
	data8_t *prgrom = (data8_t *)memory_region(REGION_CPU1);

	cpu_setbank(1, &prgrom[0x10000]);
	cpu_setbank(2, &prgrom[0x48000]);
}

static MACHINE_DRIVER_START( lethalen )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", HD6309, 8000000)	// ???
	MDRV_CPU_PROGRAM_MAP(le_main, 0)
	MDRV_CPU_VBLANK_INT(lethalen_interrupt, 1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(le_sound, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(lethalen)

	MDRV_NVRAM_HANDLER(lethalen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)

	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(lethalen)
	MDRV_VIDEO_UPDATE(lethalen)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END

ROM_START( lethalen )	// US version UAE
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD( "191uae01.u4",    0x10000,  0x40000,  CRC(dca340e3) SHA1(8efbba0e3a459bcfe23c75c584bf3a4ce25148bb) )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "191a02.f4", 0x000000, 0x010000, CRC(72b843cc) SHA1(b44b2f039358c26fa792d740639b66a5c8bf78e7) )
	ROM_RELOAD(         0x010000, 0x010000 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD32_WORD( "191a08", 0x000002, 0x100000, CRC(555bd4db) SHA1(d2e55796b4ab2306ae549fa9e7288e41eaa8f3de) )
	ROM_LOAD32_WORD( "191a10", 0x000000, 0x100000, CRC(2fa9bf51) SHA1(1e4ec56b41dfd8744347a7b5799e3ebce0939adc) )
	ROM_LOAD32_WORD( "191a07", 0x100002, 0x100000, CRC(1dad184c) SHA1(b2c4a8e48084005056aef2c8eaccb3d2eca71b73) )
	ROM_LOAD32_WORD( "191a09", 0x100000, 0x100000, CRC(e2028531) SHA1(63ccce7855d829763e9e248a6c3eb6ea89ab17ee) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "191a04", 0x000000, 0x100000, CRC(5c3eeb2b) SHA1(33ea8b3968b78806334b5a0aab3a2c24e45c604e) )
	ROM_LOAD( "191a05", 0x100000, 0x100000, CRC(f2e3b58b) SHA1(0bbc2fe87a4fd00b5073a884bcfebcf9c2c402ad) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "191a03", 0x000000, 0x200000, CRC(9b13fbe8) SHA1(19b02dbd9d6da54045b0ba4dfe7b282c72745c9c))
ROM_END

ROM_START( lethalej )	// Japan version JAD
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD( "191jad01.u4",    0x10000,  0x40000, CRC(160a25c0) SHA1(1d3ed5a158e461a73c079fe24a8e9d5e2a87e126) )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "191a02.f4", 0x000000, 0x010000, CRC(72b843cc) SHA1(b44b2f039358c26fa792d740639b66a5c8bf78e7) )
	ROM_RELOAD(         0x010000, 0x010000 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD32_WORD( "191a08", 0x000002, 0x100000, CRC(555bd4db) SHA1(d2e55796b4ab2306ae549fa9e7288e41eaa8f3de) )
	ROM_LOAD32_WORD( "191a10", 0x000000, 0x100000, CRC(2fa9bf51) SHA1(1e4ec56b41dfd8744347a7b5799e3ebce0939adc) )
	ROM_LOAD32_WORD( "191a07", 0x100002, 0x100000, CRC(1dad184c) SHA1(b2c4a8e48084005056aef2c8eaccb3d2eca71b73) )
	ROM_LOAD32_WORD( "191a09", 0x100000, 0x100000, CRC(e2028531) SHA1(63ccce7855d829763e9e248a6c3eb6ea89ab17ee) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "191a04", 0x000000, 0x100000, CRC(5c3eeb2b) SHA1(33ea8b3968b78806334b5a0aab3a2c24e45c604e) )
	ROM_LOAD( "191a05", 0x100000, 0x100000, CRC(f2e3b58b) SHA1(0bbc2fe87a4fd00b5073a884bcfebcf9c2c402ad) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "191a03", 0x000000, 0x200000, CRC(9b13fbe8) SHA1(19b02dbd9d6da54045b0ba4dfe7b282c72745c9c))
ROM_END

static DRIVER_INIT( lethalen )
{
	konami_rom_deinterleave_2(REGION_GFX2);

	state_save_register_int("LE", 0, "control2", &cur_control2);
	state_save_register_UINT8("LE", 0, "Ram48", le_ram48, 0x800);
}

GAMEX( 1992, lethalen, 0,        lethalen, lethalen, lethalen, ORIENTATION_FLIP_Y, "Konami", "Lethal Enforcers (US ver UAE)", GAME_NOT_WORKING)
GAMEX( 1992, lethalej, lethalen, lethalen, lethalen, lethalen, ORIENTATION_FLIP_X, "Konami", "Lethal Enforcers (Japan ver JAD)", GAME_NOT_WORKING)
