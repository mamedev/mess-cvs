#include "osdepend.h"

int osd_init(void)
{
	return 0;
}

void osd_exit(void)
{
}

int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components)
{
	return 0;
}

void osd_close_display(void)
{
}

int osd_skip_this_frame(void)
{
	return 0;
}

struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap, struct rectangle *bounds)
{
	return NULL;
}

const char *osd_get_fps_text(const struct performance_info *performance)
{
	return NULL;
}

int osd_start_audio_stream(int stereo)
{
	return 0;
}

int osd_update_audio_stream(INT16 *buffer)
{
	return 0;
}

void osd_stop_audio_stream(void)
{
}

void osd_set_mastervolume(int attenuation)
{
}

int osd_get_mastervolume(void)
{
	return 0;
}

void osd_sound_enable(int enable)
{
}

const struct KeyboardInfo *osd_get_key_list(void)
{
	static const struct KeyboardInfo ki[1];
	return ki;
}

int osd_is_key_pressed(int keycode)
{
	return 0;
}

int osd_readkey_unicode(int flush)
{
	return 0;
}

const struct JoystickInfo *osd_get_joy_list(void)
{
	static const struct JoystickInfo ji[1];
	return ji;
}

int osd_is_joy_pressed(int joycode)
{
	return 0;
}

int osd_is_joystick_axis_code(int joycode)
{
	return 0;
}

int osd_joystick_needs_calibration(void)
{
	return 0;
}

void osd_joystick_start_calibration(void)
{
}

const char *osd_joystick_calibrate_next(void)
{
	return NULL;
}

void osd_joystick_calibrate(void)
{
}

void osd_joystick_end_calibration(void)
{
}

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
}

void osd_trak_read(int player, int *deltax, int *deltay)
{
}

void osd_analogjoy_read(int player,int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
}

void osd_customize_inputport_defaults(struct ipd *defaults)
{
}

int osd_display_loading_rom_message(const char *name,struct rom_load_data *romdata)
{
	return 0;
}

void osd_pause(int paused)
{
}

void CLIB_DECL osd_die(const char *text,...)
{
}

void CLIB_DECL logerror(const char *text,...)
{
}

int osd_select_file(mess_image *img, char *filename)
{
	return 0;
}

int osd_keyboard_disabled(void)
{
	return 0;
}

void osd_begin_final_unloading(void)
{
}

void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
}