#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1403.h"

static struct {
	UINT8 reg[0x100];
} pc1403_lcd;

const struct { int x, y; } pos[]={
    { 152,67 }, // pc1403
    { 155,69 } // pc1403h
};

static int DOWN=67, RIGHT=152;

int pc1403_vh_start(void)
{
    if (strcmp(Machine->gamedrv->name,"pc1403h")==0) {
	DOWN=pos[1].y;
	RIGHT=pos[1].x;
    }
    return pocketc_vh_start();
}


READ_HANDLER(pc1403_lcd_read)
{
    UINT8 data=pc1403_lcd.reg[offset];
    logerror ("lcd read %.4x %.2x\n",offset, data);
    return data;
}

WRITE_HANDLER(pc1403_lcd_write)
{
    logerror ("lcd write %.4x %.2x\n",offset, data);
    pc1403_lcd.reg[offset]=data;
}

static const POCKETC_FIGURE	line={ /* simple line */
	"11111",
	"11111",
	"11111e" 
};
static const POCKETC_FIGURE busy={ 
	"11  1 1  11 1 1",
	"1 1 1 1 1   1 1",
	"11  1 1  1  1 1",
	"1 1 1 1   1  1",
	"11   1  11   1e"
}, def={ 
	"11  111 111",
	"1 1 1   1",
	"1 1 111 11",
	"1 1 1   1",
	"11  111 1e" 
}, shift={
	" 11 1 1 1 111 111",
	"1   1 1 1 1    1",
	" 1  111 1 11   1",
	"  1 1 1 1 1    1",
	"11  1 1 1 1    1e" 
}, hyp={
	"1 1 1 1 11",
	"1 1 1 1 1 1",
	"111 1 1 11",
	"1 1  1  1",
	"1 1  1  1e" 
}, de={
	"11  111",
	"1 1 1",
	"1 1 111",
	"1 1 1",
	"11  111e"
}, g={
	" 11",
	"1",
	"1 1",
	"1 1",
	" 11e" 
}, rad={
	"11   1  11",
	"1 1 1 1 1 1",
	"11  111 1 1",
	"1 1 1 1 1 1",
	"1 1 1 1 11e"
}, braces={
	" 1 1",
	"1   1",
	"1   1",
	"1   1",
	" 1 1e"
}, m={
	"1   1",
	"11 11",
	"1 1 1",
	"1   1",
	"1   1e"
}, e={
	"111",
	"1",
	"111",
	"1",
	"111e" 
}, run={ 
	"11  1 1 1  1",
	"1 1 1 1 11 1",
	"11  1 1 1 11",
	"1 1 1 1 1  1",
	"1 1  1  1  1e" 
}, pro={ 
	"11  11   1  ",
	"1 1 1 1 1 1",
	"11  11  1 1",
	"1   1 1 1 1",
	"1   1 1  1e" 
}, japan={ 
	"  1  1  11   1  1  1",
	"  1 1 1 1 1 1 1 11 1",
	"  1 111 11  111 1 11",
	"1 1 1 1 1   1 1 1  1",
	" 1  1 1 1   1 1 1  1e" 
}, sml={ 
	" 11 1 1 1",
	"1   111 1",
	" 1  1 1 1",
	"  1 1 1 1",
	"11  1 1 111e" 
}, rsv={ 
	"11   11 1   1",
	"1 1 1   1   1",
	"11   1   1 1",
	"1 1   1  1 1",
	"1 1 11    1e" 
};

void pc1403_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
    int x, y, i, j;
    int color[2];
    /* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = Machine->pens[pocketc_colortable[CONTRAST][0]];
    color[1] = Machine->pens[pocketc_colortable[CONTRAST][1]];
    
    if (full_refresh)
    {
        osd_mark_dirty (0, 0, bitmap->width, bitmap->height);
    }
    if (pocketc_backdrop)
        copybitmap (bitmap, pocketc_backdrop->artwork, 0, 0, 0, 0, NULL, 
		    TRANSPARENCY_NONE, 0);
    else
	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);
    
    for (x=RIGHT,y=DOWN,i=0; i<6*5;x+=2) {
	for (j=0; j<5;j++,i++,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }
    for (i=9*5; i<12*5;x+=2) {
	for (j=0; j<5;j++,i++,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }
    for (i=6*5; i<9*5;x+=2) {
	for (j=0; j<5;j++,i++,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }
    for (i=0x7b-3*5; i>0x7b-6*5;x+=2) {
	for (j=0; j<5;j++,i--,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }
    for (i=0x7b; i>0x7b-3*5;x+=2) {
	for (j=0; j<5;j++,i--,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }
    for (i=0x7b-6*5; i>0x7b-12*5;x+=2) {
	for (j=0; j<5;j++,i--,x+=2)
	    drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
		    x,y,
		    0, TRANSPARENCY_NONE,0);
    }

    pocketc_draw_special(bitmap,RIGHT,DOWN-13,busy,
			 pc1403_lcd.reg[0x3d]&1?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+18,DOWN-13,def,
			 pc1403_lcd.reg[0x3d]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+43,DOWN-13,shift,
			 pc1403_lcd.reg[0x3d]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+63,DOWN-13,hyp,
			 pc1403_lcd.reg[0x3d]&8?color[1]:color[0]);
    
    pocketc_draw_special(bitmap,RIGHT+100,DOWN-13,sml, // position, looking?
			 pc1403_lcd.reg[0x3c]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+140,DOWN-13,japan, // position, looking?
			 pc1403_lcd.reg[0x3c]&1?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+191,DOWN-13,de,
			 pc1403_lcd.reg[0x7c]&0x20?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+199,DOWN-13,g,
			 pc1403_lcd.reg[0x7c]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+203,DOWN-13,rad,
			 pc1403_lcd.reg[0x7c]&8?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+266,DOWN-13,braces,
			 pc1403_lcd.reg[0x7c]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+274,DOWN-13,m,
			 pc1403_lcd.reg[0x7c]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+281,DOWN-13,e,
			 pc1403_lcd.reg[0x7c]&1?color[1]:color[0]);
    
    pocketc_draw_special(bitmap,RIGHT+31,DOWN+27,line/*calc*/,
			 pc1403_lcd.reg[0x3d]&0x40?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+52,DOWN+27,line/*run*/,
			 pc1403_lcd.reg[0x3d]&0x20?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+73,DOWN+27,line/*prog*/,
			 pc1403_lcd.reg[0x3d]&0x10?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+232,DOWN+27,line/*matrix*/, 
			 pc1403_lcd.reg[0x3c]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+253,DOWN+27,line/*stat*/,
			 pc1403_lcd.reg[0x3c]&8/*not tested*/?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+274,DOWN+27,line/*print*/,
			 pc1403_lcd.reg[0x7c]&0x40/*not tested*/?color[1]:color[0]);    
}

