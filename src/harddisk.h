/***************************************************************************

    harddisk.h

    Generic MAME hard disk implementation, with differencing files

***************************************************************************/

#pragma once

#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "driver.h"
#include "chd.h"


struct hard_disk_file;

struct hard_disk_info
{
	UINT32			cylinders;
	UINT32			heads;
	UINT32			sectors;
	UINT32			sectorbytes;
};



/*************************************
 *
 *  Prototypes
 *
 *************************************/

struct hard_disk_file *hard_disk_open(chd_file *chd);
void hard_disk_close(struct hard_disk_file *file);

chd_file *hard_disk_get_chd(struct hard_disk_file *file);
struct hard_disk_info *hard_disk_get_info(struct hard_disk_file *file);

UINT32 hard_disk_read(struct hard_disk_file *file, UINT32 lbasector, UINT32 numsectors, void *buffer);
UINT32 hard_disk_write(struct hard_disk_file *file, UINT32 lbasector, UINT32 numsectors, const void *buffer);

#endif	/* __HARDDISK_H__ */
