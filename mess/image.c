#include "image.h"
#include "mess.h"
#include "includes/flopdrv.h"
#include "config.h"
#include "utils.h"
#include "mscommon.h"

/* ----------------------------------------------------------------------- */

static mame_file *image_fopen_new(int type, int id, int *effective_mode);


struct image_info
{
	mame_file *fp;
	int loaded;
	char *name;
	char *dir;
	UINT32 crc;
	UINT32 length;
	char *longname;
	char *manufacturer;
	char *year;
	char *playable;
	char *extrainfo;
	void *memory_pool;
};

static struct image_info images[IO_COUNT][MAX_DEV_INSTANCES];
int images_is_running;

/* ----------------------------------------------------------------------- */

static struct image_info *get_image(int type, int id)
{
	assert((type >= 0) && (type < IO_COUNT));
	assert((id >= 0) && (id < MAX_DEV_INSTANCES));
	return &images[type][id];
}

/* ----------------------------------------------------------------------- */

void *image_malloc(int type, int id, size_t size)
{
	struct image_info *img;
	img = get_image(type, id);
	return pool_malloc(&img->memory_pool, size);
}

void *image_realloc(int type, int id, void *ptr, size_t size)
{
	struct image_info *img;
	img = get_image(type, id);
	return pool_realloc(&img->memory_pool, ptr, size);
}

char *image_strdup(int type, int id, const char *src)
{
	struct image_info *img;
	img = get_image(type, id);
	return pool_strdup(&img->memory_pool, src);
}

static void image_free_resources(struct image_info *img)
{
	if (img->fp)
	{
		mame_fclose(img->fp);
		img->fp = NULL;
	}
	pool_exit(&img->memory_pool);
}

/* ----------------------------------------------------------------------- */

/* common init for all IO_FLOPPY devices */
static void	floppy_device_common_init(int id)
{
	logerror("floppy device common init: id: %02x\n",id);
	/* disk inserted */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_INSERTED, 1);
	/* drive connected */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_CONNECTED, 1);
}

/* common exit for all IO_FLOPPY devices */
static void floppy_device_common_exit(int id)
{
	logerror("floppy device common exit: id: %02x\n",id);
	/* disk removed */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_INSERTED, 0);
	/* drive disconnected */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_CONNECTED, 0);
}

int image_load(int type, int id, const char *name)
{
	const struct IODevice *dev;
	char *newname;
	struct image_info *img;
	int err;
	mame_file *fp = NULL;

	img = get_image(type, id);

	dev = device_find(Machine->gamedrv, type);
	assert(dev);

	if (img->loaded)
		image_unload(type, id);

	if (name && *name)
	{
		newname = image_strdup(type, id, name);
		if (!newname)
			return INIT_FAIL;
	}
	else
		newname = NULL;

	img->name = newname;
	img->dir = NULL;

	if (images_is_running && (dev->reset_depth >= IO_RESET_CPU))
		machine_reset();

	if (dev->init)
	{
		int effective_mode;
		int err0 = INIT_PASS;

		if ((dev->open_mode == OSD_FOPEN_NONE) || !image_exists(type, id))
			fp = NULL;
		else
		{
			fp = image_fopen_new(type, id, &effective_mode);
			if (fp == NULL)
			{
				printf("Unable to open image file %s\n", image_filename(type, id));
				err0 = INIT_FAIL;
			}
		}
		err = dev->init(id, fp, effective_mode);
		if (err0 != INIT_PASS)
			err = err0;
		if (err != INIT_PASS)
		{
			if (fp)
			{
				mame_fclose(fp);
				img->fp = NULL;
			}
			return err;
		}
	}

	/* init succeeded */
	/* if floppy, perform common init */
	if ((type == IO_FLOPPY) && img->name)
		floppy_device_common_init(id);

	//img->fp = fp;
	img->loaded = TRUE;
	return INIT_PASS;
}

void image_unload(int type, int id)
{
	const struct IODevice *dev;
	struct image_info *img;

	img = get_image(type, id);
	if (!img->loaded)
		return;

	dev = device_find(Machine->gamedrv, type);
	if (!dev)
		return;

	if (dev->exit)
		dev->exit(id);

	/* The following is always executed for a IO_FLOPPY exit */
	/* KT: if a image is removed:
		1. Disconnect drive
		2. Remove disk from drive */
	/* This is done here, so if a device doesn't support exit, the status
		will still be correct */
	if (type == IO_FLOPPY)
		floppy_device_common_exit(id);

	image_free_resources(img);
	memset(img, 0, sizeof(*img));
}

void image_unload_all(void)
{
	int type, id;

	for (type = 0; type < IO_COUNT; type++)
	{
		for (id = 0; id < MAX_DEV_INSTANCES; id++)
			image_unload(type, id);
	}
}

/* ----------------------------------------------------------------------- */

static int read_crc_config(const char *sysname, int type, int id)
{
	int rc = 1;
	config_file *config;
	struct image_info *img;
	char line[1024];
	char crc[9+1];

	config = config_open(sysname, sysname, FILETYPE_CRC);
	if (!config)
		goto done;

	img = get_image(type, id);
	snprintf(crc, sizeof(crc) / sizeof(crc[0]), "%08x", img->crc);
	config_load_string(config, sysname, 0, crc, line, sizeof(line));

	if (!line[0])
		goto done;

	logerror("found CRC %s= %s\n", crc, line);
	img->longname		= image_strdup(type, id, stripspace(strtok(line, "|")));
	img->manufacturer	= image_strdup(type, id, stripspace(strtok(NULL, "|")));
	img->year			= image_strdup(type, id, stripspace(strtok(NULL, "|")));
	img->playable		= image_strdup(type, id, stripspace(strtok(NULL, "|")));
	img->extrainfo		= image_strdup(type, id, stripspace(strtok(NULL, "|")));
	rc = 0;

done:
	if (config)
		config_close(config);
	return rc;
}


mame_file *image_fopen_custom(int type, int id, int filetype, int read_or_write)
{
	struct image_info *img;
	const char *sysname;
	mame_file *file;
	char buffer[512];

	img = get_image(type, id);
	assert(img);

	if (!img->name)
		return NULL;

	if (img->fp)
		/* If already open, we won't open the file again until it is closed. */
		return NULL;

	sysname = Machine->gamedrv->name;
	logerror("image_fopen: trying %s for system %s\n", img->name, sysname);
	img->fp = file = mame_fopen(sysname, img->name, filetype, read_or_write);

	if (file)
	{
		const struct IODevice *pc_dev = device_first(Machine->gamedrv);

		/* is this file actually a zip file? */
		if ((mame_fread(file, buffer, 4) == 4) && (buffer[0] == 0x50)
			&& (buffer[1] == 0x4B) && (buffer[2] == 0x03) && (buffer[3] == 0x04))
		{
			mame_fseek(file, 26, SEEK_SET);
			if (mame_fread(file, buffer, 2) == 2)
			{
				int fname_length = buffer[0];
				char *newname;

				mame_fseek(file, 30, SEEK_SET);
				mame_fread(file, buffer, fname_length);
				mame_fclose(file);
				img->fp = file = NULL;

				buffer[fname_length] = '\0';

				newname = image_malloc(type, id, strlen(img->name) + 1 + fname_length + 1);
				if (!newname)
					return NULL;

				strcpy(newname, img->name);
				strcat(newname, osd_path_separator());
				strcat(newname, buffer);
				img->fp = file = mame_fopen(sysname, newname, filetype, read_or_write);
				if (!file)
					return NULL;

				img->name = newname;
			}
		}
		mame_fseek(file, 0, SEEK_SET);

		logerror("image_fopen: found image %s for system %s\n", img->name, sysname);
		img->length = mame_fsize(file);
/* Cowering, partial crcs for NES/A7800/others */
		img->crc = 0;
		while( pc_dev && pc_dev->count && !img->crc)
		{
			logerror("partialcrc() -> %08lx\n", (long) pc_dev->partialcrc);
			if( type == pc_dev->type && pc_dev->partialcrc )
			{
				unsigned char *pc_buf = (unsigned char *)malloc(img->length);
				if( pc_buf )
				{
					mame_fseek(file,0,SEEK_SET);
					mame_fread(file,pc_buf,img->length);
					mame_fseek(file,0,SEEK_SET);
					logerror("Calling partialcrc()\n");
					img->crc = (*pc_dev->partialcrc)(pc_buf,img->length);
					free(pc_buf);
				}
				else
				{
					logerror("failed to malloc(%d)\n", img->length);
				}
			}
			pc_dev = device_next(Machine->gamedrv, pc_dev);
		}

		if (!img->crc) img->crc = mame_fcrc(file);
		if( img->crc == 0 && img->length < 0x100000 )
		{
			logerror("image_fopen: calling mame_fchecksum() for %d bytes\n", img->length);
			mame_fchecksum(sysname, img->name, &img->length, &img->crc);
			logerror("image_fopen: CRC is %08x\n", img->crc);
		}

		read_crc_config(sysname, type, id);
	}

	return file;
}

static mame_file *image_fopen_new(int type, int id, int *effective_mode)
{
	mame_file *fref;
	int effective_mode_local;
	const struct IODevice *dev;

	dev = device_find(Machine->gamedrv, type);
	assert(dev);
	assert(id < dev->count);

	switch (dev->open_mode) {
	case OSD_FOPEN_NONE:
	default:
		/* unsupported modes */
		printf("Internal Error in file \""__FILE__"\", line %d\n", __LINE__);
		fref = NULL;
		effective_mode_local = OSD_FOPEN_NONE;
		break;

	case OSD_FOPEN_READ:
	case OSD_FOPEN_WRITE:
	case OSD_FOPEN_RW:
	case OSD_FOPEN_RW_CREATE:
		/* supported modes */
		fref = image_fopen_custom(type, id, FILETYPE_IMAGE, dev->open_mode);
		effective_mode_local = dev->open_mode;
		break;

	case OSD_FOPEN_RW_OR_READ:
		/* R/W or read-only: emulated mode */
		fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_RW);
		if (fref)
			effective_mode_local = OSD_FOPEN_RW;
		else
		{
			fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_READ);
			effective_mode_local = OSD_FOPEN_READ;
		}
		break;

	case OSD_FOPEN_RW_CREATE_OR_READ:
		/* R/W, read-only, or create new R/W image: emulated mode */
		fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_RW);
		if (fref)
			effective_mode_local = OSD_FOPEN_RW;
		else
		{
			fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_READ);
			if (fref)
				effective_mode_local = OSD_FOPEN_READ;
			else
			{
				fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_RW_CREATE);
				effective_mode_local = OSD_FOPEN_RW_CREATE;
			}
		}
		break;

	case OSD_FOPEN_READ_OR_WRITE:
		/* read or write: emulated mode */
		fref = image_fopen_custom(type, id, FILETYPE_IMAGE, OSD_FOPEN_READ);
		if (fref)
			effective_mode_local = OSD_FOPEN_READ;
		else
		{
			fref = image_fopen_custom(type, id, FILETYPE_IMAGE, /*OSD_FOPEN_WRITE*/OSD_FOPEN_RW_CREATE);
			effective_mode_local = OSD_FOPEN_WRITE;
		}
		break;
	}

	if (effective_mode)
		*effective_mode = effective_mode_local;

	return fref;
}

/* ----------------------------------------------------------------------- */

const char *image_filename(int type, int id)
{
	return get_image(type, id)->name;
}

const char *image_basename(int type, int id)
{
	return osd_basename((char *) image_filename(type, id));
}

const char *image_filetype(int type, int id)
{
	const char *s;
	s = image_filename(type, id);
	s = strrchr(s, '.');
	return s ? s+1 : NULL;
}

const char *image_filedir(int type, int id)
{
	struct image_info *info;
	char *s;

	info = get_image(type, id);
	if (!info->dir)
	{
		info->dir = image_strdup(type, id, info->name);
		if (info->dir)
		{
			s = info->dir + strlen(info->dir);
			while(--s > info->dir)
			{
				if (!strchr("\\/:", *s))
				{
					*s = '\0';
					break;
				}
			}
		}
	}
	return info->dir;
}

int image_exists(int type, int id)
{
	return image_filename(type, id) != NULL;
}

unsigned int image_length(int type, int id)
{
	return get_image(type, id)->length;
}

unsigned int image_crc(int type, int id)
{
	return get_image(type, id)->crc;
}

const char *image_longname(int type, int id)
{
	return get_image(type, id)->longname;
}

const char *image_manufacturer(int type, int id)
{
	return get_image(type, id)->manufacturer;
}

const char *image_year(int type, int id)
{
	return get_image(type, id)->year;
}

const char *image_playable(int type, int id)
{
	return get_image(type, id)->playable;
}

const char *image_extrainfo(int type, int id)
{
	return get_image(type, id)->extrainfo;
}

mame_file *image_fp(int type, int id)
{
	return get_image(type, id)->fp;
}
