#pragma once
#ifdef DEBUG
#pragma comment(lib, "Spout_static_dbg.lib")
#else
#pragma comment(lib, "Spout_static.lib")
#endif

#include <obs-module.h>
#include <graphics/graphics.h>

#include "inc/SpoutSenderNames.h"

#define OBS_PLUGIN "spout2-texture-filter"
#define OBS_PLUGIN_ "spout2_texture_filter"
#define OBS_PLUGIN_VERSION_MAJOR 0
#define OBS_PLUGIN_VERSION_MINOR 0
#define OBS_PLUGIN_VERSION_RELEASE 1
#define OBS_PLUGIN_VERSION_STRING "0.0.1"
#define OBS_PLUGIN_LANG "en-US"
//#define OBS_PLUGIN_COLOR_SPACE GS_RGBA16
#define OBS_PLUGIN_COLOR_SPACE GS_RGBA_UNORM

#define OBS_SETTING_UI_FILTER_NAME "mahgu.spout2texture.ui.filter_title"
#define OBS_SETTING_UI_SENDER_NAME "mahgu.spout2texture.ui.sender_name"
#define OBS_SETTINGS_UI_BUTTON_TITLE "mahgu.spout2texture.ui.button_title"
#define OBS_SETTING_DEFAULT_SENDER_NAME \
	"mahgu.spout2texture.default.sender_name"

#define obs_log(level, format, ...) \
	blog(level, "[spout2-texture-filter] " format, ##__VA_ARGS__)

#define error(format, ...) obs_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) obs_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) obs_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) obs_log(LOG_DEBUG, format, ##__VA_ARGS__)

bool obs_module_load(void);
void obs_module_unload();

spoutSenderNames spout;

namespace Spout2Texture {
// DEBUG stuff

void report_version();

// OBS plugin stuff

static const char *filter_get_name(void *unused);
static obs_properties_t *filter_properties(void *unused);
static void filter_defaults(obs_data_t *defaults);

static void *filter_create(obs_data_t *settings, obs_source_t *source);
static void filter_destroy(void *data);
static void filter_render_callback(void *data, uint32_t cx, uint32_t cy);
static void filter_update(void *data, obs_data_t *settings);
static void filter_video_render(void *data, gs_effect_t *effect);

// Spout2 sender stuff
namespace Sender {


static void create(void *data, uint32_t width, uint32_t height);
static void release(void *data);

} // namespace Sender

// Spout2 texture stuff

namespace Texture {

static void reset_buffers(void *data, uint32_t width, uint32_t height);
static void reset(void *data, uint32_t width, uint32_t height);
static void render(void *data, obs_source_t *target, uint32_t cx, uint32_t cy);

} // namespace Texture

struct filter {
	obs_source_t *context;	

	uint32_t width;
	uint32_t height;

	enum gs_color_space prev_space;
	enum gs_color_format shared_format;

	gs_texture_t *prev_target;
	gs_texture_t *shared_texture;

	bool sender_created;
	bool buffer_swap;

	gs_texture_t *texture_buffer1;
	gs_texture_t *texture_buffer2;

	const char *setting_sender_name;	// realtime setting

	std::string sender_name;		// spout sendername

};

struct obs_source_info create_filter_info()
{
	struct obs_source_info filter_info = {};

	filter_info.id = OBS_PLUGIN_;
	filter_info.type = OBS_SOURCE_TYPE_FILTER;
	filter_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB;

	filter_info.get_name = filter_get_name;
	filter_info.get_properties = filter_properties;
	filter_info.get_defaults = filter_defaults;
	filter_info.create = filter_create;
	filter_info.destroy = filter_destroy;
	filter_info.video_render = filter_video_render;
	filter_info.update = filter_update;

	return filter_info;
};

} // namespace Spout2Texture
