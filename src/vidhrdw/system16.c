/***************************************************************************

Sega System 16 Video Hardware

Known issues:
- better abstraction of tilemap hardware is needed; a lot of this mess could and should
	be consolidated further
- many games have ROM patches - why?  let's emulate protection when possible
- several games fail their RAM self-test because of hacks in the drivers
- many registers are suspiciously plucked from working RAM
- several games have nvram according to the self test, but we aren't yet saving it
- many games suffer from sys16_refreshenable register not being mapped
- hangon road isn't displayed(!) emulation bug?
- road-rendering routines need to be cleaned up or at least better described
- logical sprite height computation isn't quite right - garbage pixels are drawn
- screen orientation support for sprite drawing
- sprite drawing is unoptimized
- end-of-sprite marker support will fix some glitches
- shadow and partial shadow sprite support
- to achieve sprite-tilemap orthogonality we must draw sprites from front to back;
	this will also allow us to avoid processing the same shadowed pixel twice.

The System16 video hardware consists of:

- Road Layer (present only for certain games)

- Two scrolling tilemap layers

- Two alternate scrolling tilemap layers, normally invisible.

- A fixed tilemap layer, normally used for score, lives display

Each scrolling layer (foreground, background) is an arrangement
of 4 pages selected from 16 available pages, laid out as follows:

	Page0  Page1
	Page2  Page3

Each page is an arrangement of 8x8 tiles, 64 tiles wide, and 32 tiles high.

Layers normally scroll as a whole, with xscroll and yscroll.

However, layers can also be configured with screenwise rowscroll, if the most significant
bit of xscroll is set.  When a layer is in this mode, the splittable is used.

When rowscroll is in effect, the most significant bit of rowscroll selects between the
default layer and an alternate layer associated with it.

The foreground layer's tiles may be flagged as high-priority; this is used to mask
sprites, i.e. the grass in Altered Beast.

The background layer's tiles may also be flagged as high-priority.  In this case,
it's really a transparency_pen effect rather.  Aurail uses this.

Most games map Video Registers in textram as follows:

type0:
most games

type1:
alexkidd,fantzone,shinobl,hangon
mjleague

others:
	shangon, shdancbl,
	dduxbl,eswat,
	passsht,passht4b
	quartet,quartet2,
	tetris,tturfbl,wb3bl

sys16_textram:
type1		type0			function
---------------------------------------
0x74f		0x740			sys16_fg_page
0x74e		0x741			sys16_bg_page
			0x742			sys16_fg2_page
			0x743			sys16_bg2_page

0x792		0x748			sys16_fg_scrolly
0x793		0x749			sys16_bg_scrolly
			0x74a			sys16_fg2_scrolly
			0x74b			sys16_bg2_scrolly

0x7fc		0x74c			sys16_fg_scrollx
0x7fd		0x74d			sys16_bg_scrollx
			0x74e			sys16_fg2_scrollx
			0x74f			sys16_bg2_scrollx

			0x7c0..0x7df	sys18_splittab_fg_x
			0x7e0..0x7ff	sys18_splittab_bg_x

***************************************************************************/
#include "driver.h"
#include "machine/system16.h"

/*
static void debug_draw( struct mame_bitmap *bitmap, int x, int y, unsigned int data ){
	int digit;
	for( digit=0; digit<4; digit++ ){
		drawgfx( bitmap, Machine->uifont,
			"0123456789abcdef"[data>>12],
			0,
			0,0,
			x+digit*6,y,
			&Machine->visible_area,TRANSPARENCY_NONE,0);
		data = (data<<4)&0xffff;
	}
}

static void debug_vreg( struct mame_bitmap *bitmap ){
	int g = 0x740;
	int i;

	if( keyboard_pressed( KEYCODE_Q ) ) g+=0x10;
	if( keyboard_pressed( KEYCODE_W ) ) g+=0x20;
	if( keyboard_pressed( KEYCODE_E ) ) g+=0x40;
	if( keyboard_pressed( KEYCODE_R ) ) g+=0x80;

	for( i=0; i<16; i++ ){
		debug_draw( bitmap, 8,8*i,sys16_textram[g+i] );
	}
}
*/

/* callback to poll video registers */
void (* sys16_update_proc)( void );

data16_t *sys16_tileram;
data16_t *sys16_textram;
data16_t *sys16_spriteram;
data16_t *sys16_roadram;

static int num_sprites;

#define MAXCOLOURS 0x2000 /* 8192 */

#ifdef TRANSPARENT_SHADOWS
#define ShadowColorsShift 8
UINT16 shade_table[MAXCOLOURS];
#endif

int sys16_sh_shadowpal;
int sys16_MaxShadowColors;

/* video driver constants (potentially different for each game) */
int sys16_gr_bitmap_width;
int (*sys16_spritesystem)( struct sys16_sprite_attributes *sprite, const UINT16 *source, int bJustGetColor );
int *sys16_obj_bank;
int sys16_sprxoffset;
int sys16_bgxoffset;
int sys16_fgxoffset;
int sys16_textmode;
int sys16_textlayer_lo_min;
int sys16_textlayer_lo_max;
int sys16_textlayer_hi_min;
int sys16_textlayer_hi_max;
int sys16_dactype;
int sys16_bg1_trans; // alien syn + sys18
int sys16_bg_priority_mode;
int sys16_fg_priority_mode;
int sys16_bg_priority_value;
int sys16_fg_priority_value;
int sys16_18_mode;
int sys16_spritelist_end;
int sys16_tilebank_switch;
int sys16_rowscroll_scroll;
int sys16_quartet_title_kludge;

/* video registers */
int sys16_tile_bank1;
int sys16_tile_bank0;
int sys16_refreshenable;

int sys16_bg_scrollx, sys16_bg_scrolly;
int sys16_bg2_scrollx, sys16_bg2_scrolly;
int sys16_fg_scrollx, sys16_fg_scrolly;
int sys16_fg2_scrollx, sys16_fg2_scrolly;

int sys16_bg_page[4];
int sys16_bg2_page[4];
int sys16_fg_page[4];
int sys16_fg2_page[4];

int sys18_bg2_active;
int sys18_fg2_active;
data16_t *sys18_splittab_bg_x;
data16_t *sys18_splittab_bg_y;
data16_t *sys18_splittab_fg_x;
data16_t *sys18_splittab_fg_y;

data16_t *sys16_gr_ver;
data16_t *sys16_gr_hor;
data16_t *sys16_gr_pal;
data16_t *sys16_gr_flip;
int sys16_gr_palette;
int sys16_gr_palette_default;
unsigned char sys16_gr_colorflip[2][4];
data16_t *sys16_gr_second_road;

static struct tilemap *background, *foreground, *text_layer;
static struct tilemap *background2, *foreground2;
static int old_bg_page[4],old_fg_page[4], old_tile_bank1, old_tile_bank0;
static int old_bg2_page[4],old_fg2_page[4];

/***************************************************************************/

READ16_HANDLER( sys16_textram_r ){
	return sys16_textram[offset];
}

READ16_HANDLER( sys16_tileram_r ){
	return sys16_tileram[offset];
}

/***************************************************************************/

/*
	We mark the priority buffer as follows:
		text	(0xf)
		fg (hi) (0x7)
		fg (lo) (0x3)
		bg (hi) (0x1)
		bg (lo) (0x0)

	Each sprite has 4 levels of priority, specifying where they are placed between bg(lo) and text.
*/

static void draw_sprite16(
	struct mame_bitmap *bitmap,
	const unsigned char *addr, int pitch,
	const pen_t *paldata,
	int x0, int y0, int screen_width, int screen_height,
	int width, int height,
	int flipx, int flipy,
	int priority )
{
	int dx,dy;
	priority = 1<<priority;

	if( flipy ){
		dy = -1;
		y0 += screen_height-1;
	}
	else {
		dy = 1;
	}

	if( flipx ){
		dx = -1;
		x0 += screen_width-1;
	}
	else {
		dx = 1;
	}

	{
		int sy = y0;
		int y;
		int ycount = 0;
		for( y=0; y<height; y++ ){
			ycount += screen_height;
			while( ycount>=height ){
				if( sy>=0 && sy<bitmap->height ){
					const UINT8 *source = addr;
					UINT16 *dest = (UINT16 *)bitmap->line[sy];
					UINT8 *pri = priority_bitmap->line[sy];

					int sx = x0;
					int x;
					int xcount = 0;
					for( x=0; x<width; x+=2 ){
						UINT8 data = *source++; /* next 2 pixels */

						// breaks outrun, since those sprites are drawn from right to left
						//if( data==0x0f ) break;

						xcount += screen_width;
						while( xcount>=width )
						{
							int pen = data>>4;
							if( pen!=0x0 && pen!=0xf && sx>=0 && sx<bitmap->width ){
								if( (pri[sx]&priority)==0 ){
									dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
						xcount += screen_width;
						while( xcount>=width )
						{
							int pen = data&0xf;
							if(  pen!=0x0 && pen!=0xf && sx>=0 && sx<bitmap->width ){
								if( (pri[sx]&priority)==0 ){
									dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
					}
				}
				ycount -= height;
				sy+=dy;
			}
			addr += pitch;
		}
	}
}

static void draw_sprite8(
	struct mame_bitmap *bitmap,
	const unsigned char *addr, int pitch,
	const pen_t *paldata,
	int x0, int y0, int screen_width, int screen_height,
	int width, int height,
	int flipx, int flipy,
	int priority )
{
	int dx,dy;
/* test code */
//	if( keyboard_pressed( KEYCODE_A ) && priority==0 ) return;
//	if( keyboard_pressed( KEYCODE_S ) && priority==1 ) return;
//	if( keyboard_pressed( KEYCODE_D ) && priority==2 ) return;
//	if( keyboard_pressed( KEYCODE_F ) && priority==3 ) return;

	priority = 1<<priority;

	if( flipy ){
		dy = -1;
		y0 += screen_height-1;
	}
	else {
		dy = 1;
	}

	if( flipx ){
		dx = -1;
		x0 += screen_width-1;
	}
	else {
		dx = 1;
	}

	{
		int sy = y0;
		int y;
		int ycount = 0;
		for( y=0; y<height; y++ ){
			ycount += screen_height;
			while( ycount>=height ){
				if( sy>=0 && sy<bitmap->height ){
					const UINT8 *source = addr;
					UINT8 *dest = bitmap->line[sy];
					UINT8 *pri = priority_bitmap->line[sy];
					int sx = x0;
					int x;
					int xcount = 0;
					for( x=0; x<width; x+=2 ){
						UINT8 data = *source++; /* next 2 pixels */

						// breaks outrun
						//if( data==0x0f ) break;

						xcount += screen_width;
						while( xcount>=width )
						{
							int pen = data>>4;
							if( pen!=0x0 && pen!=0xf && sx>=0 && sx<bitmap->width ){
								if( (pri[sx]&priority)==0 ){
									dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
						xcount += screen_width;
						while( xcount>=width )
						{
							int pen = data&0xf;
							if(  pen!=0x0 && pen!=0xf && sx>=0 && sx<bitmap->width ){
								if( (pri[sx]&priority)==0 ){
									dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
					}
				}
				ycount -= height;
				sy+=dy;
			}
			addr += pitch;
		}
	}
}

static void draw_sprite(
	struct mame_bitmap *bitmap,
	const UINT8 *addr, int pitch,
	const pen_t *paldata,
	int x0, int y0, int screen_width, int screen_height,
	int width, int height,
	int flipx, int flipy,
	int priority )
{
	if( bitmap->depth==16 ){
		draw_sprite16(bitmap,addr,pitch,paldata,x0,y0,screen_width,screen_height,
			width,height,flipx,flipy,priority);
	}
	else {
		draw_sprite8(bitmap,addr,pitch,paldata,x0,y0,screen_width,screen_height,
			width,height,flipx,flipy,priority);
	}
}

static void draw_sprites( struct mame_bitmap *bitmap, int b3d ){
	const pen_t *base_pal = Machine->gfx[0]->colortable;
	const unsigned char *base_gfx = memory_region(REGION_GFX2);

	struct sys16_sprite_attributes sprite;
	const data16_t *source = sys16_spriteram;
	int i;
	memset( &sprite, 0x00, sizeof(sprite) );
	for( i=0; i<num_sprites; i++ ){
		sprite.flags = 0;
		if( sys16_spritesystem( &sprite, source,0 ) ) return; /* end-of-spritelist */
		if( sprite.flags & SYS16_SPR_VISIBLE ){
			int xpos = sprite.x;
			int ypos = sprite.y;
			int gfx = sprite.gfx;
			int screen_width;
			int width;
			int logical_height;
			int pitch = sprite.pitch;
			int flipy = pitch&0x80;
			int flipx = sprite.flags&SYS16_SPR_FLIPX;

			width = pitch&0x7f;
			if( pitch&0x80 ) width = 0x80-width;
			width *= 4;

			if( sys16_spritesystem==sys16_sprite_sharrier ){
				logical_height = ((sprite.screen_height)<<4|0xf)*(0x400+sprite.zoomy)/0x4000;
			}
			else if( b3d ){ // outrun/aburner
				logical_height = ((sprite.screen_height<<4)|0xf)*sprite.zoomy/0x2000;
			}
			else {
				logical_height = sprite.screen_height*(0x400+sprite.zoomy)/0x400;
			}

			if( flipx ) gfx += 2; else gfx += width/2;
			if( flipy ) gfx -= logical_height*width/2;

			pitch = width/2;
			if( width==0 ){
				width = 512;// used by fantasy zone for laser
			}

			if( b3d ){ /* outrun, afterburner, ... */
				screen_width = width*0x200/sprite.zoomx;
				if( sprite.flags & SYS16_SPR_DRAW_TO_TOP ){
					ypos -= sprite.screen_height;
					flipy = !flipy;
				}
				if( sprite.flags & SYS16_SPR_DRAW_TO_LEFT ){
					xpos -= screen_width;
					flipx = !flipx;
				}
			}
			else {
				screen_width = width*(0x800-sprite.zoomx)/0x800;
			}

			draw_sprite(
				bitmap,
				base_gfx + gfx, pitch,
				base_pal + sprite.color*16,
				xpos, ypos, screen_width, sprite.screen_height,
				width, logical_height,
				flipx, flipy,
				sprite.priority
			);
		}
		source+=8;
	}
}

/***************************************************************************/

UINT32 sys16_bg_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	int page = 0;
	if( row<32 ){ /* top */
		if( col<64 ) page = 0; else page = 1;
	}
	else { /* bottom */
		if( col<64 ) page = 2; else page = 3;
	}
	row = row%32;
	col = col%64;
	return page*64*32+row*64+col;
}

UINT32 sys16_text_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	return row*64+col+(64-40);
}

/***************************************************************************/

WRITE16_HANDLER( sys16_paletteram_w ){
	data16_t oldword = paletteram16[offset];
	data16_t newword;
	COMBINE_DATA( &paletteram16[offset] );
	newword = paletteram16[offset];
	if( oldword!=newword ){ /* we can do this, because we initialize palette RAM to all black in vh_start */
		/*	   byte 0    byte 1 */
		/*	GBGR BBBB GGGG RRRR */
		/*	5444 3210 3210 3210 */
		UINT8 r = (newword & 0x00f)<<1;
		UINT8 g = (newword & 0x0f0)>>2;
		UINT8 b = (newword & 0xf00)>>7;
		if( sys16_dactype == 0 ){ /* we should really use two distinct paletteram_w handlers */
			/* dac_type == 0 (from GCS file) */
			if (newword&0x1000) r|=1;
			if (newword&0x2000) g|=2;
			if (newword&0x8000) g|=1;
			if (newword&0x4000) b|=1;
		}
		else if( sys16_dactype == 1 ){
			/* dac_type == 1 (from GCS file) Shinobi Only*/
			if (newword&0x1000) r|=1;
			if (newword&0x4000) g|=2;
			if (newword&0x8000) g|=1;
			if (newword&0x2000) b|=1;
		}

#ifndef TRANSPARENT_SHADOWS
		palette_set_color( offset,
				(r << 3) | (r >> 2), /* 5 bits red */
				(g << 2) | (g >> 4), /* 6 bits green */
				(b << 3) | (b >> 2) /* 5 bits blue */
			);
#else
		if (Machine->scrbitmap->depth == 8){ /* 8 bit shadows */
			palette_set_color( offset,
					(r << 3) | (r >> 3), /* 5 bits red */
					(g << 2) | (g >> 4), /* 6 bits green */
					(b << 3) | (b >> 3) /* 5 bits blue */
				);
		}
		else {
			r=(r << 3) | (r >> 2); /* 5 bits red */
			g=(g << 2) | (g >> 4); /* 6 bits green */
			b=(b << 3) | (b >> 2); /* 5 bits blue */

			palette_set_color( offset,r,g,b);

			/* shadow color */
			r= r * 160 / 256;
			g= g * 160 / 256;
			b= b * 160 / 256;

			palette_set_color( offset+Machine->drv->total_colors/2,r,g,b);
		}
#endif
	}
}

static void update_page( void ){
	int all_dirty = 0;
	int i,offset;
	if( old_tile_bank1 != sys16_tile_bank1 ){
		all_dirty = 1;
		old_tile_bank1 = sys16_tile_bank1;
	}
	if( old_tile_bank0 != sys16_tile_bank0 ){
		all_dirty = 1;
		old_tile_bank0 = sys16_tile_bank0;
		tilemap_mark_all_tiles_dirty( text_layer );
	}
	if( all_dirty ){
		tilemap_mark_all_tiles_dirty( background );
		tilemap_mark_all_tiles_dirty( foreground );
		if( sys16_18_mode ){
			tilemap_mark_all_tiles_dirty( background2 );
			tilemap_mark_all_tiles_dirty( foreground2 );
		}
	}
	else {
		for(i=0;i<4;i++){
			int page0 = 64*32*i;
			if( old_bg_page[i]!=sys16_bg_page[i] ){
				old_bg_page[i] = sys16_bg_page[i];
				for( offset = page0; offset<page0+64*32; offset++ ){
					tilemap_mark_tile_dirty( background, offset );
				}
			}
			if( old_fg_page[i]!=sys16_fg_page[i] ){
				old_fg_page[i] = sys16_fg_page[i];
				for( offset = page0; offset<page0+64*32; offset++ ){
					tilemap_mark_tile_dirty( foreground, offset );
				}
			}
			if( sys16_18_mode ){
				if( old_bg2_page[i]!=sys16_bg2_page[i] ){
					old_bg2_page[i] = sys16_bg2_page[i];
					for( offset = page0; offset<page0+64*32; offset++ ){
						tilemap_mark_tile_dirty( background2, offset );
					}
				}
				if( old_fg2_page[i]!=sys16_fg2_page[i] ){
					old_fg2_page[i] = sys16_fg2_page[i];
					for( offset = page0; offset<page0+64*32; offset++ ){
						tilemap_mark_tile_dirty( foreground2, offset );
					}
				}
			}
		}
	}
}

static void get_bg_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_bg_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&sys16_tilebank_switch)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}

	switch(sys16_bg_priority_mode) {
	case 1: // Alien Syndrome
		tile_info.priority = (data&0x8000)?1:0;
		break;
	case 2: // Body Slam / wrestwar
		tile_info.priority = ((data&0xff00) >= sys16_bg_priority_value)?1:0;
		break;
	case 3: // sys18 games
		if( data&0x8000 ){
			tile_info.priority = 2;
		}
		else {
			tile_info.priority = ((data&0xff00) >= sys16_bg_priority_value)?1:0;
		}
		break;
	}
}

static void get_fg_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_fg_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&sys16_tilebank_switch)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	switch(sys16_fg_priority_mode){
	case 1: // alien syndrome
		tile_info.priority = (data&0x8000)?1:0;
		break;

	case 3:
		tile_info.priority = ((data&0xff00) >= sys16_fg_priority_value)?1:0;
		break;

	default:
		if( sys16_fg_priority_mode>=0 ){
			tile_info.priority = (data&0x8000)?1:0;
		}
		break;
	}
}

static void get_bg2_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_bg2_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	tile_info.priority = 0;
}

static void get_fg2_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_fg2_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	if((data&0xff00) >= sys16_fg_priority_value) tile_info.priority = 1;
	else tile_info.priority = 0;
}

WRITE16_HANDLER( sys16_tileram_w ){
	data16_t oldword = sys16_tileram[offset];
	COMBINE_DATA( &sys16_tileram[offset] );
	if( oldword != sys16_tileram[offset] ){
		int page = offset/(64*32);
		offset = offset%(64*32);

		if( sys16_bg_page[0]==page ) tilemap_mark_tile_dirty( background, offset+64*32*0 );
		if( sys16_bg_page[1]==page ) tilemap_mark_tile_dirty( background, offset+64*32*1 );
		if( sys16_bg_page[2]==page ) tilemap_mark_tile_dirty( background, offset+64*32*2 );
		if( sys16_bg_page[3]==page ) tilemap_mark_tile_dirty( background, offset+64*32*3 );

		if( sys16_fg_page[0]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*0 );
		if( sys16_fg_page[1]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*1 );
		if( sys16_fg_page[2]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*2 );
		if( sys16_fg_page[3]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*3 );

		if( sys16_18_mode ){
			if( sys16_bg2_page[0]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*0 );
			if( sys16_bg2_page[1]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*1 );
			if( sys16_bg2_page[2]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*2 );
			if( sys16_bg2_page[3]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*3 );

			if( sys16_fg2_page[0]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*0 );
			if( sys16_fg2_page[1]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*1 );
			if( sys16_fg2_page[2]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*2 );
			if( sys16_fg2_page[3]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*3 );
		}
	}
}

/***************************************************************************/

static void get_text_tile_info( int offset ){
	const data16_t *source = sys16_textram;
	int tile_number = source[offset];
	int pri = tile_number >> 8;
	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				(tile_number&0x1ff) + sys16_tile_bank0 * 0x1000,
				512+384+((tile_number>>9)&0x7),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				(tile_number&0x1ff) + sys16_tile_bank0 * 0x1000,
				(tile_number>>9)%8,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				(tile_number&0xff)  + sys16_tile_bank0 * 0x1000,
				(tile_number>>8)%8,
				0)
	}
	if(pri>=sys16_textlayer_lo_min && pri<=sys16_textlayer_lo_max)
		tile_info.priority = 1;
	if(pri>=sys16_textlayer_hi_min && pri<=sys16_textlayer_hi_max)
		tile_info.priority = 0;
}

WRITE16_HANDLER( sys16_textram_w ){
	int oldword = sys16_textram[offset];
	COMBINE_DATA( &sys16_textram[offset] );
	if( oldword!=sys16_textram[offset] ){
		tilemap_mark_tile_dirty( text_layer, offset );
	}
}

/***************************************************************************/

void sys16_vh_stop( void ){
#if 0
	FILE *f;
	int i;

	f = fopen( "textram.txt","w" );
	if( f ){
		const UINT16 *source = sys16_textram;
		for( i=0; i<0x800; i++ ){
			if( (i&0x1f)==0 ) fprintf( f, "\n %04x: ", i );
			fprintf( f, "%04x ", source[i] );
		}
		fclose( f );
	}

	f = fopen( "spriteram.txt","w" );
	if( f ){
		const UINT16 *source = sys16_spriteram;
		for( i=0; i<0x800; i++ ){
			if( (i&0x7)==0 ) fprintf( f, "\n %04x: ", i );
			fprintf( f, "%04x ", source[i] );
		}
		fclose( f );
	}
#endif
}

int sys16_vh_start( void ){
	static int bank_default[16] = {
		0x0,0x1,0x2,0x3,
		0x4,0x5,0x6,0x7,
		0x8,0x9,0xa,0xb,
		0xc,0xd,0xe,0xf
	};
	sys16_obj_bank = bank_default;

	if( !sys16_bg1_trans )
		background = tilemap_create(
			get_bg_tile_info,
			sys16_bg_map,
			TILEMAP_OPAQUE,
			8,8,
			64*2,32*2 );
	else
		background = tilemap_create(
			get_bg_tile_info,
			sys16_bg_map,
			TILEMAP_TRANSPARENT,
			8,8,
			64*2,32*2 );

	foreground = tilemap_create(
		get_fg_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	text_layer = tilemap_create(
		get_text_tile_info,
		sys16_text_map,
		TILEMAP_TRANSPARENT,
		8,8,
		40,28 );

	num_sprites = 128*2; /* only 128 for most games; aburner uses 256 */

#ifdef TRANSPARENT_SHADOWS
	sprite_set_shade_table(shade_table);
#endif

	if( background && foreground && text_layer ){
		/* initialize all entries to black - needed for Golden Axe*/
		int i;
		for( i=0; i<Machine->drv->total_colors; i++ ){
			palette_set_color( i, 0,0,0 );
		}
#ifdef TRANSPARENT_SHADOWS
		if (Machine->scrbitmap->depth == 8) /* 8 bit shadows */
		{
			int j,color;
			for(j = 0, i = Machine->drv->total_colors/2;j<sys16_MaxShadowColors;i++,j++)
			{
				color=j * 160 / (sys16_MaxShadowColors-1);
				color=color | 0x04;
				palette_set_color(i, color, color, color);
			}
		}
		if(sys16_MaxShadowColors==32)
			sys16_MaxShadowColors_Shift = ShadowColorsShift;
		else if(sys16_MaxShadowColors==16)
			sys16_MaxShadowColors_Shift = ShadowColorsShift+1;

#endif

		if(sys16_bg1_trans) tilemap_set_transparent_pen( background, 0 );
		tilemap_set_transparent_pen( foreground, 0 );
		tilemap_set_transparent_pen( text_layer, 0 );

		sys16_tile_bank0 = 0;
		sys16_tile_bank1 = 1;

		sys16_fg_scrollx = 0;
		sys16_fg_scrolly = 0;

		sys16_bg_scrollx = 0;
		sys16_bg_scrolly = 0;

		sys16_refreshenable = 1;

		/* common defaults */
		sys16_update_proc = 0;
		sys16_spritesystem = sys16_sprite_shinobi;
		sys16_sprxoffset = -0xb8;
		sys16_textmode = 0;
		sys16_bgxoffset = 0;
		sys16_dactype = 0;
		sys16_bg_priority_mode=0;
		sys16_fg_priority_mode=0;
		sys16_spritelist_end=0xffff;
		sys16_tilebank_switch=0x1000;

		// Defaults for sys16 games
		sys16_textlayer_lo_min=0;
		sys16_textlayer_lo_max=0x7f;
		sys16_textlayer_hi_min=0x80;
		sys16_textlayer_hi_max=0xff;

		sys16_18_mode=0;

#ifdef GAMMA_ADJUST
		{
			static float sys16_orig_gamma=0;
			static float sys16_set_gamma=0;
			float cur_gamma=osd_get_gamma();

			if(sys16_orig_gamma == 0)
			{
				sys16_orig_gamma = cur_gamma;
				sys16_set_gamma = cur_gamma - 0.35;
				if (sys16_set_gamma < 0.5) sys16_set_gamma = 0.5;
				if (sys16_set_gamma > 2.0) sys16_set_gamma = 2.0;
				osd_set_gamma(sys16_set_gamma);
			}
			else
			{
				if(sys16_orig_gamma == cur_gamma)
				{
					osd_set_gamma(sys16_set_gamma);
				}
			}
		}
#endif
		return 0;
	}
	return 1;
}

int sys16_hangon_vh_start( void ){
	int ret;
	sys16_bg1_trans=1;
	ret = sys16_vh_start();
	if(ret) return 1;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_bg_priority_mode=-1;
	sys16_bg_priority_value=0x1800;
	sys16_fg_priority_value=0x2000;
	return 0;
}

int sys18_vh_start( void ){
	sys16_bg1_trans=1;

	background2 = tilemap_create(
		get_bg2_tile_info,
		sys16_bg_map,
		TILEMAP_OPAQUE,
		8,8,
		64*2,32*2 );

	foreground2 = tilemap_create(
		get_fg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	if( background2 && foreground2 ){
		if( sys16_vh_start()==0 ){
			tilemap_set_transparent_pen( foreground2, 0 );

			if(sys18_splittab_fg_x){
				tilemap_set_scroll_rows( foreground , 64 );
				tilemap_set_scroll_rows( foreground2 , 64 );
			}
			if(sys18_splittab_bg_x){
				tilemap_set_scroll_rows( background , 64 );
				tilemap_set_scroll_rows( background2 , 64 );
			}

			sys16_textlayer_lo_min=0;
			sys16_textlayer_lo_max=0x1f;
			sys16_textlayer_hi_min=0x20;
			sys16_textlayer_hi_max=0xff;

			sys16_18_mode=1;
			sys16_bg_priority_mode=3;
			sys16_fg_priority_mode=3;
			sys16_bg_priority_value=0x1800;
			sys16_fg_priority_value=0x2000;

			return 0;
		}
	}
	return 1;
}

/***************************************************************************/

#ifdef TRANSPARENT_SHADOWS
static void build_shadow_table(void)
{
	int i,size;
	int color_start=Machine->drv->total_colors/2;
	/* build the shading lookup table */
	if (Machine->scrbitmap->depth == 8) /* 8 bit shadows */
	{
		if(sys16_MaxShadowColors == 0) return;
		for (i = 0; i < 256; i++)
		{
			unsigned char r, g, b;
			int y;
			osd_get_pen(i, &r, &g, &b);
			y = (r * 10 + g * 18 + b * 4) >> sys16_MaxShadowColors_Shift;
			shade_table[i] = Machine->pens[color_start + y];
		}
		for(i=0;i<sys16_MaxShadowColors;i++)
		{
			shade_table[Machine->pens[color_start + i]]=Machine->pens[color_start + i];
		}
	}
	else
	{
		if(sys16_MaxShadowColors != 0)
		{
			size=Machine->drv->total_colors/2;
			for(i=0;i<size;i++)
			{
				shade_table[Machine->pens[i]]=Machine->pens[size + i];
				shade_table[Machine->pens[size+i]]=Machine->pens[size + i];
			}
		}
		else
		{
			size=Machine->drv->total_colors;
			for(i=0;i<size;i++)
			{
				shade_table[Machine->pens[i]]=Machine->pens[i];
			}
		}
	}
}
#else
#define build_shadow_table()
#endif

static void sys16_vh_refresh_helper( void ){
	if( sys18_splittab_bg_x ){
		if( (sys16_bg_scrollx&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_rows( background , 1 );
			tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_rows( background , 64 );
			offset = 32+((sys16_bg_scrolly&0x1f8) >> 3);

			for( i=0; i<29; i++ ){
				scroll = sys18_splittab_bg_x[i];
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
			}
		}
	}
	else {
		tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
	}

	if( sys18_splittab_bg_y ){
		if( (sys16_bg_scrolly&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_cols( background , 1 );
			tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_cols( background , 128 );
			offset = 127-((sys16_bg_scrollx&0x3f8) >> 3)-40+2;

			for( i=0;i<41;i++ ){
				scroll = sys18_splittab_bg_y[(i+24)>>1];
				tilemap_set_scrolly( background , (i+offset)&0x7f, -256+(scroll&0x3ff) );
			}
		}
	}
	else {
		tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
	}

	if( sys18_splittab_fg_x ){
		if( (sys16_fg_scrollx&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_rows( foreground , 1 );
			tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_rows( foreground , 64 );
			offset = 32+((sys16_fg_scrolly&0x1f8) >> 3);

			for(i=0;i<29;i++){
				scroll = sys18_splittab_fg_x[i];
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
			}
		}
	}
	else {
		tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
	}

	if( sys18_splittab_fg_y ){
		if( (sys16_fg_scrolly&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_cols( foreground , 1 );
			tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_cols( foreground , 128 );
			offset = 127-((sys16_fg_scrollx&0x3f8) >> 3)-40+2;

			for( i=0; i<41; i++ ){
				scroll = sys18_splittab_fg_y[(i+24)>>1];
				tilemap_set_scrolly( foreground , (i+offset)&0x7f, -256+(scroll&0x3ff) );
			}
		}
	}
	else {
		tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );
	}
}

static void sys18_vh_screenrefresh_helper( void ){
	int i;
	if( sys18_splittab_bg_x ){ // screenwise rowscroll?
		int offset,offset2, scroll,scroll2,orig_scroll;

		offset = 32+((sys16_bg_scrolly&0x1f8) >> 3); // 0x00..0x3f
		offset2 = 32+((sys16_bg2_scrolly&0x1f8) >> 3); // 0x00..0x3f

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_bg_x[i];
			if((sys16_bg_scrollx  &0xff00) != 0x8000) scroll = sys16_bg_scrollx;
			if((sys16_bg2_scrollx &0xff00) != 0x8000) scroll2 = sys16_bg2_scrollx;

			if(orig_scroll&0x8000){ // background2
				tilemap_set_scrollx( background , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_bgxoffset );
			}
			else{ // background
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}
	else {
		tilemap_set_scrollx( background , 0, -320-(sys16_bg_scrollx&0x3ff)+sys16_bgxoffset );
		tilemap_set_scrollx( background2, 0, -320-(sys16_bg2_scrollx&0x3ff)+sys16_bgxoffset );
	}

	tilemap_set_scrolly( background , 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( background2, 0, -256+sys16_bg2_scrolly );

	if( sys18_splittab_fg_x ){
		int offset,offset2, scroll,scroll2,orig_scroll;

		offset = 32+((sys16_fg_scrolly&0x1f8) >> 3);
		offset2 = 32+((sys16_fg2_scrolly&0x1f8) >> 3);

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_fg_x[i];
			if( (sys16_fg_scrollx &0xff00) != 0x8000 ) scroll = sys16_fg_scrollx;

			if( (sys16_fg2_scrollx &0xff00) != 0x8000 ) scroll2 = sys16_fg2_scrollx;

			if( orig_scroll&0x8000 ){
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( foreground2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_fgxoffset );
			}
			else {
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
				tilemap_set_scrollx( foreground2 , (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}
	else {
		tilemap_set_scrollx( foreground , 0, -320-(sys16_fg_scrollx&0x3ff)+sys16_fgxoffset );
		tilemap_set_scrollx( foreground2, 0, -320-(sys16_fg2_scrollx&0x3ff)+sys16_fgxoffset );
	}


	tilemap_set_scrolly( foreground , 0, -256+sys16_fg_scrolly );
	tilemap_set_scrolly( foreground2, 0, -256+sys16_fg2_scrolly );

	tilemap_set_enable( background2, sys18_bg2_active );
	tilemap_set_enable( foreground2, sys18_fg2_active );
}

void sys16_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh){
	if (!sys16_refreshenable) return;

	if( sys16_update_proc ) sys16_update_proc();
	update_page();
	sys16_vh_refresh_helper(); /* set scroll registers */

	fillbitmap(priority_bitmap,0,NULL);

	build_shadow_table();

	tilemap_draw( bitmap, background, TILEMAP_IGNORE_TRANSPARENCY, 0x00 );
	if(sys16_bg_priority_mode) tilemap_draw( bitmap, background, TILEMAP_IGNORE_TRANSPARENCY | 1, 0x00 );
//	sprite_draw(sprite_list,3); // needed for Aurail
	if( sys16_bg_priority_mode==2 ) tilemap_draw( bitmap, background, 1, 0x01 );// body slam (& wrestwar??)
//	sprite_draw(sprite_list,2);
	else if( sys16_bg_priority_mode==1 ) tilemap_draw( bitmap, background, 1, 0x03 );// alien syndrome / aurail
	tilemap_draw( bitmap, foreground, 0, 0x03 );
//	sprite_draw(sprite_list,1);
	tilemap_draw( bitmap, foreground, 1, 0x07 );
	if( sys16_textlayer_lo_max!=0 ) tilemap_draw( bitmap, text_layer, 1, 7 );// needed for Body Slam
//	sprite_draw(sprite_list,0);
	tilemap_draw( bitmap, text_layer, 0, 0xf );

	draw_sprites( bitmap,0 );
}

void sys18_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh){
	if (!sys16_refreshenable) return;
	if( sys16_update_proc ) sys16_update_proc();
	update_page();
	sys18_vh_screenrefresh_helper(); /* set scroll registers */

	build_shadow_table();

	if(sys18_bg2_active)
		tilemap_draw( bitmap, background2, 0, 0 );
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	tilemap_draw( bitmap, background, TILEMAP_IGNORE_TRANSPARENCY, 0 );
	tilemap_draw( bitmap, background, TILEMAP_IGNORE_TRANSPARENCY | 1, 0 );	//??
	tilemap_draw( bitmap, background, TILEMAP_IGNORE_TRANSPARENCY | 2, 0 );	//??

//	sprite_draw(sprite_list,3);
	tilemap_draw( bitmap, background, 1, 0x1 );
//	sprite_draw(sprite_list,2);
	tilemap_draw( bitmap, background, 2, 0x3 );

	if(sys18_fg2_active) tilemap_draw( bitmap, foreground2, 0, 0x3 );
	tilemap_draw( bitmap, foreground, 0, 0x3 );
//	sprite_draw(sprite_list,1);
	if(sys18_fg2_active) tilemap_draw( bitmap, foreground2, 1, 0x7 );
	tilemap_draw( bitmap, foreground, 1, 0x7 );

	tilemap_draw( bitmap, text_layer, 1, 0x7 );
//	sprite_draw(sprite_list,0);
	tilemap_draw( bitmap, text_layer, 0, 0xf );

	draw_sprites( bitmap, 0 );
}


static void render_gr(struct mame_bitmap *bitmap,int priority){
	/* the road is a 4 color bitmap */
	int i,j;
	UINT8 *data = memory_region(REGION_GFX3);
	UINT8 *source;
	UINT8 *line;
	UINT16 *line16;
	UINT32 *line32;
	UINT16 *data_ver=sys16_gr_ver;
	UINT32 ver_data,hor_pos;
	UINT16 colors[5];
	UINT32 fastfill;
	int colorflip;
	int yflip=0, ypos;
	int dx=1,xoff=0;

	pen_t *paldata1 = Machine->gfx[0]->colortable + sys16_gr_palette;
	pen_t *paldata2 = Machine->gfx[0]->colortable + sys16_gr_palette_default;

#if 0
if( keyboard_pressed( KEYCODE_S ) ){
	FILE *f;
	int i;
	char fname[64];
	static int fcount = 0;
	while( keyboard_pressed( KEYCODE_S ) ){}
	sprintf( fname, "road%d.txt",fcount );
	fcount++;
	f = fopen( fname,"w" );
	if( f ){
		const UINT16 *source = sys16_gr_ver;
		for( i=0; i<0x1000; i++ ){
			if( (i&0x1f)==0 ) fprintf( f, "\n %04x: ", i );
			fprintf( f, "%04x ", source[i] );
		}
		fclose( f );
	}
}
#endif

	priority=priority << 10;

	if (Machine->scrbitmap->depth == 16) /* 16 bit */
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY ){
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=0;i<224;i++){
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data=*data_ver;
				if((ver_data & 0x400) == priority)
				{
					colors[0] = paldata1[ sys16_gr_pal[(ver_data)&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200)
					{
						// fill line
						for(j=0;j<320;j++)
						{
							line16=(UINT16 *)bitmap->line[j]+ypos;
							*line16=colors[0];
						}
					}
					else
					{
						// copy line
						ver_data=ver_data & 0x00ff;
						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						ver_data = ver_data << sys16_gr_bitmap_width;

						if(hor_pos & 0xf000)
						{
							// reverse
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else
						{
							// normal
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						source = data + hor_pos + ver_data + 18 + 8;

						for(j=0;j<320;j++)
						{
							line16=(UINT16 *)bitmap->line[xoff+j*dx]+ypos;
							*line16 = colors[*source++];
						}
					}
				}
				data_ver++;
			}
		}
		else
		{ /* 16 bpp, normal screen orientation */
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=0;i<224;i++){ /* with each scanline */
				if( yflip ) ypos=223-i; else ypos=i;
				ver_data= *data_ver; /* scanline parameters */
				/*
					gr_ver:
						---- -x-- ---- ----	priority
						---- --x- ---- ---- ?
						---- ---x ---- ---- ?
						---- ---- xxxx xxxx ypos (source bitmap)

					gr_flip:
						---- ---- ---- x--- flip colors

					gr_hor:
						xxxx xxxx xxxx xxxx	xscroll

					gr_pal:
						---- ---- xxxx xxxx palette
				*/
				if( (ver_data & 0x400) == priority ){
					colors[0] = paldata1[ sys16_gr_pal[ver_data&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200){
						line16 = (UINT16 *)bitmap->line[ypos]; /* dest for drawing */
						for(j=0;j<320;j++){
							*line16++=colors[0]; /* opaque fill with background color */
						}
					}
					else {
						// copy line
						line16 = (UINT16 *)bitmap->line[ypos]+xoff; /* dest for drawing */
						ver_data &= 0xff;

						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;
						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						if( hor_pos & 0xf000 ){ // reverse (precalculated)
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else { // normal
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						ver_data <<= sys16_gr_bitmap_width;
						source = data + hor_pos + ver_data + 18 + 8;

						for(j=0;j<320;j++){
							*line16 = colors[*source++];
							line16+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
	else /* 8 bit */
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=0;i<224;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x400) == priority)
				{
					colors[0] = paldata1[ sys16_gr_pal[ver_data&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200)
					{
						// fill line
						for(j=0;j<320;j++)
						{
							((UINT8 *)bitmap->line[j])[ypos]=colors[0];
						}
					}
					else
					{
						// copy line
						ver_data=ver_data & 0x00ff;
						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;
						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						ver_data = ver_data << sys16_gr_bitmap_width;

						if(hor_pos & 0xf000)
						{
							// reverse
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else
						{
							// normal
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						source = data + hor_pos + ver_data + 18 + 8;

						for(j=0;j<320;j++)
						{
							((UINT8 *)bitmap->line[xoff+j*dx])[ypos] = colors[*source++];
						}
					}
				}
				data_ver++;
			}
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=0;i<224;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x400) == priority)
				{
					colors[0] = paldata1[ sys16_gr_pal[ver_data&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200)
					{
						// fill line
						line32 = (UINT32 *)bitmap->line[ypos];
						fastfill = colors[0] + (colors[0] << 8) + (colors[0] << 16) + (colors[0] << 24);
						for(j=0;j<320;j+=4)
						{
							*line32++ = fastfill;
						}
					}
					else
					{
						// copy line
						line = ((UINT8 *)bitmap->line[ypos])+xoff;
						ver_data=ver_data & 0x00ff;
						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						ver_data = ver_data << sys16_gr_bitmap_width;

						if(hor_pos & 0xf000)
						{
							// reverse
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else
						{
							// normal
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						source = data + hor_pos + ver_data + 18 + 8;

						for(j=0;j<320;j++)
						{
							*line = colors[*source++];
							line+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
}

void sys16_hangon_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh){
	if (!sys16_refreshenable) return;
	if( sys16_update_proc ) sys16_update_proc();
	update_page();

	tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
	tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
	tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );

	fillbitmap(priority_bitmap,0,NULL);

	build_shadow_table();

	render_gr(bitmap,0); /* sky */
	tilemap_draw( bitmap, background, 0, 0 );
	tilemap_draw( bitmap, foreground, 0, 0 );
	render_gr(bitmap,1); /* floor */
	tilemap_draw( bitmap, text_layer, 0, 0xf );

	draw_sprites( bitmap, 0 );
}

static void render_grv2(struct mame_bitmap *bitmap,int priority)
{
	int i,j;
	UINT8 *data = memory_region(REGION_GFX3);
	UINT8 *source,*source2,*temp;
	UINT8 *line;
	UINT16 *line16;
	UINT32 *line32;
	UINT16 *data_ver=sys16_gr_ver;
	UINT32 ver_data,hor_pos,hor_pos2;
	UINT16 colors[5];
	UINT32 fastfill;
	int colorflip,colorflip_info;
	int yflip=0,ypos;
	int dx=1,xoff=0;

	int second_road = sys16_gr_second_road[0];

	pen_t *paldata1 = Machine->gfx[0]->colortable + sys16_gr_palette;
	pen_t *paldata2 = Machine->gfx[0]->colortable + sys16_gr_palette_default;

	priority=priority << 11;

	if (Machine->scrbitmap->depth == 16) /* 16 bit */
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=0;i<224;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data = *data_ver;
				if((ver_data & 0x800) == priority)
				{

					if(ver_data & 0x800) /* disable */
					{
						colors[0] = paldata1[ ver_data&0x3f ];
						// fill line
						for(j=0;j<320;j++)
						{
							line16=(UINT16 *)bitmap->line[j]+ypos;
							*line16=colors[0];
						}
					}
					else
					{
						// copy line
						ver_data=ver_data & 0x01ff;		//???
						colorflip_info = sys16_gr_flip[ver_data];

						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];		//??

						colorflip = (colorflip_info >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];

						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];

						ver_data=ver_data>>1;
						if( ver_data != 0 )
						{
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}
						source  = data + ((hor_pos +0x200) & 0x7ff) + 0x300 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 0x300 + ver_data + 8;

						switch(second_road)
						{
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}

						source2++;

						for(j=0;j<320;j++)
						{
							line16=(UINT16 *)bitmap->line[xoff+j*dx]+ypos;
							if(*source2 <= *source)
								*line16 = colors[*source];
							else
								*line16 = colors[*source2];
							source++;
							source2++;
						}
					}
				}
				data_ver++;
			}
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=0;i<224;i++){
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x800) == priority){
					if(ver_data & 0x800){
						colors[0] = paldata1[ ver_data&0x3f ];
						// fill line
						line16 = (UINT16 *)bitmap->line[ypos];
						for(j=0;j<320;j++){
							*line16++ = colors[0];
						}
					}
					else {
						// copy line
						line16 = (UINT16 *)bitmap->line[ypos]+xoff;
						ver_data &= 0x01ff;		//???
						colorflip_info = sys16_gr_flip[ver_data];
						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];		//??
						colorflip = (colorflip_info >> 3) & 1;
						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];
						ver_data=ver_data>>1;
						if( ver_data != 0 ){
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}
						source  = data + ((hor_pos +0x200) & 0x7ff) + 768 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 768 + ver_data + 8;
						switch(second_road){
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}
						source2++;
						for(j=0;j<320;j++){
							if(*source2 <= *source) *line16 = colors[*source]; else *line16 = colors[*source2];
							source++;
							source2++;
							line16+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
	else
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=0;i<224;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x800) == priority)
				{

					if(ver_data & 0x800)
					{
						colors[0] = paldata1[ ver_data&0x3f ];
						// fill line
						for(j=0;j<320;j++)
						{
							((UINT8 *)bitmap->line[j])[ypos]=colors[0];
						}
					}
					else
					{
						// copy line
						ver_data=ver_data & 0x01ff;		//???
						colorflip_info = sys16_gr_flip[ver_data];

						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];		//??

						colorflip = (colorflip_info >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];

						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];

						ver_data=ver_data>>1;
						if( ver_data != 0 )
						{
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}

						source  = data + ((hor_pos +0x200) & 0x7ff) + 768 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 768 + ver_data + 8;

						switch(second_road)
						{
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}

						source2++;

						for(j=0;j<320;j++)
						{
							if(*source2 <= *source)
								((UINT8 *)bitmap->line[xoff+j*dx])[ypos] = colors[*source];
							else
								((UINT8 *)bitmap->line[xoff+j*dx])[ypos] = colors[*source2];
							source++;
							source2++;
						}
					}
				}
				data_ver++;
			}
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=0;i<224;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x800) == priority)
				{

					if(ver_data & 0x800)
					{
						colors[0] = paldata1[ ver_data&0x3f ];
						// fill line
						line32 = (UINT32 *)bitmap->line[ypos];
						fastfill = colors[0] + (colors[0] << 8) + (colors[0] << 16) + (colors[0] << 24);
						for(j=0;j<320;j+=4)
						{
							*line32++ = fastfill;
						}
					}
					else
					{
						// copy line
						line = ((UINT8 *)bitmap->line[ypos])+xoff;
						ver_data=ver_data & 0x01ff;		//???
						colorflip_info = sys16_gr_flip[ver_data];

						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];		//??

						colorflip = (colorflip_info >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];

						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];

						ver_data=ver_data>>1;
						if( ver_data != 0 )
						{
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}

						source  = data + ((hor_pos +0x200) & 0x7ff) + 768 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 768 + ver_data + 8;

						switch(second_road)
						{
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}

						source2++;

						for(j=0;j<320;j++)
						{
							if(*source2 <= *source)
								*line = colors[*source];
							else
								*line = colors[*source2];
							source++;
							source2++;
							line+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
}


int sys16_outrun_vh_start( void ){
	int ret;
	sys16_bg1_trans=1;
	ret = sys16_vh_start();
	if(ret) return 1;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_bg_priority_mode=-1;
	sys16_bg_priority_value=0x1800;
	sys16_fg_priority_value=0x2000;
	return 0;
}

void sys16_outrun_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh){
	if( sys16_refreshenable ){
		if( sys16_update_proc ) sys16_update_proc();
		update_page();

		tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
		tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );

		tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
		tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );

		build_shadow_table();

		render_grv2(bitmap,1);
		tilemap_draw( bitmap, background, 0, 0 );
		tilemap_draw( bitmap, foreground, 0, 0 );
		render_grv2(bitmap,0);

		draw_sprites( bitmap, 1 );

		tilemap_draw( bitmap, text_layer, 0, 0 );
	}
}

/***************************************************************************/

static UINT8 *aburner_backdrop;

UINT8 *aburner_unpack_backdrop( const UINT8 *baseaddr ){
	UINT8 *result = malloc(512*256*2);
	if( result ){
		int page;
		for( page=0; page<2; page++ ){
			UINT8 *dest = result + 512*256*page;
			const UINT8 *source = baseaddr + 0x8000*page;
			int y;
			for( y=0; y<256; y++ ){
				int x;
				for( x=0; x<512; x++ ){
					int data0 = source[x/8];
					int data1 = source[x/8 + 0x4000];
					int bit = 0x80>>(x&7);
					int pen = 0;
					if( data0 & bit ) pen+=1;
					if( data1 & bit ) pen+=2;
					dest[x] = pen;
				}

				{
					int edge_color = dest[0];
					for( x=0; x<512; x++ ){
						if( dest[x]==edge_color ) dest[x] = 4; else break;
					}
					edge_color = dest[511];
					for( x=511; x>=0; x-- ){
						if( dest[x]==edge_color ) dest[x] = 4; else break;
					}
				}

				source += 0x40;
				dest += 512;
			}
		}
	}
	return result;
}

int sys16_aburner_vh_start( void ){
	int ret;

	aburner_backdrop = aburner_unpack_backdrop( memory_region(REGION_GFX3) );

	sys16_bg1_trans=1;
	ret = sys16_vh_start();
	if(ret) return 1;

	foreground2 = tilemap_create(
		get_fg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	background2 = tilemap_create(
		get_bg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	if( foreground2 && background2 ){
		ret = sys16_vh_start();
		if(ret) return 1;
		tilemap_set_transparent_pen( foreground2, 0 );
		sys16_18_mode = 1;

		tilemap_set_scroll_rows( foreground , 64 );
		tilemap_set_scroll_rows( foreground2 , 64 );
		tilemap_set_scroll_rows( background , 64 );
		tilemap_set_scroll_rows( background2 , 64 );

		return 0;
	}
	return 1;
}

void sys16_aburner_vh_stop( void ){
	free( aburner_backdrop );
	sys16_vh_stop();
}

static void aburner_draw_road( struct mame_bitmap *bitmap ){
	/*
		sys16_roadram[0x1000]:
			0x04: flying (sky/horizon)
			0x43: (flying->landing)
			0xc3: runway landing
			0xe3: (landing -> flying)
			0x03: rocky canyon

			Thunderblade: 0x04, 0xfe
	*/

	/*	Palette:
	**		0x1700	ground(8), road(4), road(4)
	**		0x1720	sky(16)
	**		0x1730	road edge(2)
	**		0x1780	background color(16)
	*/

	const UINT16 *vreg = sys16_roadram;
	/*	0x000..0x0ff: 0x800: disable; 0x100: enable
		0x100..0x1ff: color/line_select
		0x200..0x2ff: xscroll
		0x400..0x4ff: 0x5b0?
		0x600..0x6ff: flip colors
	*/
	int page = sys16_roadram[0x1000];
	int sy;

	for( sy=0; sy<bitmap->height; sy++ ){
		UINT16 *dest = (UINT16 *)bitmap->line[sy]; /* assume 16bpp */
		int sx;
		UINT16 line = vreg[0x100+sy];

		if( page&4 ){ /* flying */
			int xscroll = vreg[0x200+sy] - 0x552;
			UINT16 sky = Machine->pens[0x1720];
			UINT16 ground = Machine->pens[0x1700];
			for( sx=0; sx<bitmap->width; sx++ ){
				int temp = xscroll+sx;
				if( temp<0 ){
					*dest++ = sky;
				}
				else if( temp < 0x200 ){
					*dest++ = ground;
				}
				else {
					*dest++ = sky;
				}
			}
		}
		else if( line&0x800 ){
			/* opaque fill; the least significant nibble selects color */
			unsigned short color = Machine->pens[0x1780+(line&0xf)];
			for( sx=0; sx<bitmap->width; sx++ ){
				*dest++ = color;
			}
		}
		else if( page&0xc0 ){ /* road */
			const UINT8 *source = aburner_backdrop+(line&0xff)*512 + 512*256*(page&1);
			UINT16 xscroll = (512-320)/2;
			// 040d 04b0 0552: normal: sky,horizon,sea

			UINT16 flip = vreg[0x600+sy];
			int clut[5];
			{
				int road_color = 0x1708+(flip&0x1);
				clut[0] = Machine->pens[road_color];
				clut[1] = Machine->pens[road_color+2];
				clut[2] = Machine->pens[road_color+4];
				clut[3] = Machine->pens[road_color+6];
				clut[4] = Machine->pens[(flip&0x100)?0x1730:0x1731]; /* edge of road */
			}
			for( sx=0; sx<bitmap->width; sx++ ){
				int xpos = (sx + xscroll)&0x1ff;
				*dest++ = clut[source[xpos]];
			}
		}
		else { /* rocky canyon */
			UINT16 flip = vreg[0x600+sy];
			unsigned short color = Machine->pens[(flip&0x100)?0x1730:0x1731];
			for( sx=0; sx<bitmap->width; sx++ ){
				*dest++ = color;
			}
		}
	}
#if 0
	if( keyboard_pressed( KEYCODE_S ) ){ /* debug */
		FILE *f;
		int i;
		char fname[64];
		static int fcount = 0;
		while( keyboard_pressed( KEYCODE_S ) ){}
		sprintf( fname, "road%d.txt",fcount );
		fcount++;
		f = fopen( fname,"w" );
		if( f ){
			const UINT16 *source = sys16_roadram;
			for( i=0; i<0x1000; i++ ){
				if( (i&0x1f)==0 ) fprintf( f, "\n %04x: ", i );
				fprintf( f, "%04x ", source[i] );
			}
			fclose( f );
		}
	}
#endif
}

static void sys16_aburner_vh_screenrefresh_helper( void ){
	const data16_t *vreg = &sys16_textram[0x740];
	int i;

	{
		UINT16 data = vreg[0];
		sys16_fg_page[0] = data>>12;
		sys16_fg_page[1] = (data>>8)&0xf;
		sys16_fg_page[2] = (data>>4)&0xf;
		sys16_fg_page[3] = data&0xf;
		sys16_fg_scrolly = vreg[8];
		sys16_fg_scrollx = vreg[12];
	}

	{
		UINT16 data = vreg[0+1];
		sys16_bg_page[0] = data>>12;
		sys16_bg_page[1] = (data>>8)&0xf;
		sys16_bg_page[2] = (data>>4)&0xf;
		sys16_bg_page[3] = data&0xf;
		sys16_bg_scrolly = vreg[8+1];
		sys16_bg_scrollx = vreg[12+1];
	}

	{
		UINT16 data = vreg[0+2];
		sys16_fg2_page[0] = data>>12;
		sys16_fg2_page[1] = (data>>8)&0xf;
		sys16_fg2_page[2] = (data>>4)&0xf;
		sys16_fg2_page[3] = data&0xf;
		sys16_fg2_scrolly = vreg[8+2];
		sys16_fg2_scrollx = vreg[12+2];
	}

	{
		UINT16 data = vreg[0+3];
		sys16_bg2_page[0] = data>>12;
		sys16_bg2_page[1] = (data>>8)&0xf;
		sys16_bg2_page[2] = (data>>4)&0xf;
		sys16_bg2_page[3] = data&0xf;
		sys16_bg2_scrolly = vreg[8+3];
		sys16_bg2_scrollx = vreg[12+3];
	}

	sys18_splittab_fg_x = &sys16_textram[0x7c0];
	sys18_splittab_bg_x = &sys16_textram[0x7e0];

	{
		int offset,offset2, scroll,scroll2,orig_scroll;
		offset  = 32+((sys16_bg_scrolly >>3)&0x3f ); // screenwise rowscroll
		offset2 = 32+((sys16_bg2_scrolly>>3)&0x3f ); // screenwise rowscroll

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_bg_x[i];
			if((sys16_bg_scrollx  &0xff00) != 0x8000) scroll = sys16_bg_scrollx;
			if((sys16_bg2_scrollx &0xff00) != 0x8000) scroll2 = sys16_bg2_scrollx;

			if( orig_scroll&0x8000 ){ // background2
				tilemap_set_scrollx( background , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_bgxoffset );
			}
			else{ // background1
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}

	tilemap_set_scrolly( background , 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( background2, 0, -256+sys16_bg2_scrolly );

	{
		int offset,offset2, scroll,scroll2,orig_scroll;
		offset  = 32+((sys16_fg_scrolly >>3)&0x3f ); // screenwise rowscroll
		offset2 = 32+((sys16_fg2_scrolly>>3)&0x3f ); // screenwise rowscroll

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_fg_x[i];
			if( (sys16_fg_scrollx &0xff00) != 0x8000 ) scroll = sys16_fg_scrollx;

			if( (sys16_fg2_scrollx &0xff00) != 0x8000 ) scroll2 = sys16_fg2_scrollx;

			if( orig_scroll&0x8000 ){ // foreground2
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( foreground2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_fgxoffset );
			}
			else { // foreground
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
				tilemap_set_scrollx( foreground2 , (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}

	tilemap_set_scrolly( foreground , 0, -256+sys16_fg_scrolly );
	tilemap_set_scrolly( foreground2, 0, -256+sys16_fg2_scrolly );
}

void sys16_aburner_vh_screenrefresh( struct mame_bitmap *bitmap, int full_refresh ){
	sys16_aburner_vh_screenrefresh_helper();
	update_page();

	fillbitmap(priority_bitmap,0,NULL);

	aburner_draw_road( bitmap );

//	tilemap_draw( bitmap, background2, 0, 7 );
//	tilemap_draw( bitmap, background2, 1, 7 );

	/* speed indicator, high score header */
	tilemap_draw( bitmap, background, 0, 7 );
	tilemap_draw( bitmap, background, 1, 7 );

	/* radar view */
	tilemap_draw( bitmap, foreground2, 0, 7 );
	tilemap_draw( bitmap, foreground2, 1, 7 );

	/* hand, scores */
	tilemap_draw( bitmap, foreground, 0, 7 );
	tilemap_draw( bitmap, foreground, 1, 7 );

	tilemap_draw( bitmap, text_layer, 0, 7 );
	draw_sprites( bitmap, 1 );

//	debug_draw( bitmap, 8,8,sys16_roadram[0x1000] );
}
