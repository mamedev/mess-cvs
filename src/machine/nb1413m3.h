/***************************************************************************

	Machine Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 1999/11/05 - 

***************************************************************************/

// Note: If you change the #defines, all sources using this header file must be re-compiled.

#define NB1413M3_NONE		0
#define NB1413M3_JOKERMJN	1
#define NB1413M3_JNGNIGHT	2
#define NB1413M3_JNGLADY	3
#define NB1413M3_NIGHTGL	4
#define NB1413M3_NIGHTGLS	5
#define NB1413M3_SWEETGAL	6
#define NB1413M3_PASTELGL	7
#define NB1413M3_CRYSTAL	8
#define NB1413M3_CRYSTAL2	9
#define NB1413M3_NIGHTLOV	10
#define NB1413M3_CITYLOVE	11
#define NB1413M3_SECOLOVE	12
#define NB1413M3_HOUSEMNQ	13
#define NB1413M3_HOUSEMN2	14
#define NB1413M3_LIVEGAL	15
#define NB1413M3_BIJOKKOY	16
#define NB1413M3_IEMOTO		17
#define NB1413M3_SEIHA		18
#define NB1413M3_SEIHAM		19
#define NB1413M3_HYHOO		20
#define NB1413M3_HYHOO2		21
#define NB1413M3_SWINGGAL	22
#define NB1413M3_BIJOKKOG	23
#define NB1413M3_OJOUSAN	24
#define NB1413M3_KORINAI	25
#define NB1413M3_MJCAMERA	26
#define NB1413M3_OTONANO	27
#define NB1413M3_MJSIKAKU	28
#define NB1413M3_MSJIKEN	29
#define NB1413M3_HANAMOMO	30
#define NB1413M3_TELMAHJN	31
#define NB1413M3_GIONBANA	32
#define NB1413M3_SCANDAL	33
#define NB1413M3_SCANDALM	34
#define NB1413M3_MGMEN89	35
#define NB1413M3_OHPYEPEE	36
#define NB1413M3_TOUGENK	37
#define NB1413M3_MJUCHUU	38
#define NB1413M3_MJFOCUS	39
#define NB1413M3_MJFOCUSM	40
#define NB1413M3_PEEPSHOW	41
#define NB1413M3_GALKOKU	42
#define NB1413M3_MJNANPAS	43
#define NB1413M3_BANANADR	44
#define NB1413M3_GALKAIKA	45
#define NB1413M3_MCONTEST	46
#define NB1413M3_TOKIMBSJ	47
#define NB1413M3_TOKYOGAL	48
#define NB1413M3_TRIPLEW1	49
#define NB1413M3_NTOPSTAR	50
#define NB1413M3_MLADYHTR	51
#define NB1413M3_PSTADIUM	52
#define NB1413M3_TRIPLEW2	53
#define NB1413M3_CLUB90S	54
#define NB1413M3_CHINMOKU	55
#define NB1413M3_VANILLA	56
#define NB1413M3_MJLSTORY	57
#define NB1413M3_QMHAYAKU	58
#define NB1413M3_MAIKO		59
#define NB1413M3_HANAOJI	60
#define NB1413M3_KAGUYA		61
#define NB1413M3_APPAREL	62
#define NB1413M3_AV2MJ1		63

#define NB1413M3_VCR_NOP	0x00
#define NB1413M3_VCR_POWER	0x01
#define NB1413M3_VCR_STOP	0x02
#define NB1413M3_VCR_REWIND	0x04
#define NB1413M3_VCR_PLAY	0x08
#define NB1413M3_VCR_FFORWARD	0x10
#define NB1413M3_VCR_PAUSE	0x20

#define NBMJCTRL_PORT1 \
	PORT_START	/* (3) PORT 1-1 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT2 \
	PORT_START	/* (4) PORT 1-2 */ \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT3 \
	PORT_START	/* (5) PORT 1-3 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT4 \
	PORT_START	/* (6) PORT 1-4 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT5 \
	PORT_START	/* (7) PORT 1-5 */ \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT6 \
	PORT_START	/* (6) PORT 2-1 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Kan", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 M", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 I", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 E", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 A", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT7 \
	PORT_START	/* (7) PORT 2-2 */ \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Bet", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Reach", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 N", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 J", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 F", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 B", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT8 \
	PORT_START	/* (8) PORT 2-3 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Ron", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Chi", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 K", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 G", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 C", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT9 \
	PORT_START	/* (9) PORT 2-4 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Pon", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 L", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 H", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 D", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NBMJCTRL_PORT10 \
	PORT_START	/* (10) PORT 2-5 */ \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Small", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Big", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Flip", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Double Up", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Take Score", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Last Chance", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

extern void nb1413m3_init_machine(void);
extern void nb1413m3_nmi_clock_w(int data);
extern int nb1413m3_interrupt(void);
extern void nb1413m3_nvram_handler(void *file, int read_or_write);
extern int nb1413m3_sndrom_r(int offset);
extern void nb1413m3_sndrombank1_w(int data);
extern void nb1413m3_sndrombank2_w(int data);
extern int nb1413m3_gfxrom_r(int offset);
extern void nb1413m3_gfxrombank_w(int data);
extern void nb1413m3_gfxradr_l_w(int data);
extern void nb1413m3_gfxradr_h_w(int data);
extern void nb1413m3_inputportsel_w(int data);
extern int nb1413m3_inputport0_r(void);
extern int nb1413m3_inputport1_r(void);
extern int nb1413m3_inputport2_r(void);
extern int nb1413m3_inputport3_r(void);
extern int nb1413m3_dipsw1_r(void);
extern int nb1413m3_dipsw2_r(void);
extern void nb1413m3_outcoin_w(int data);
extern void nb1413m3_vcrctrl_w(int data);

extern int nb1413m3_type;
extern int nb1413m3_int_count;
extern int nb1413m3_sndrombank1;
extern int nb1413m3_sndrombank2;
extern int nb1413m3_busyctr;
extern int nb1413m3_busyflag;
extern int nb1413m3_inputport;
extern unsigned char *nb1413m3_nvram;
extern size_t nb1413m3_nvram_size;
