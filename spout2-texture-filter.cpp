#include "spout2-texture-filter.h"

#ifdef DEBUG
#include <string>
#endif;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(OBS_PLUGIN, OBS_PLUGIN_LANG)

namespace Spout2Texture {

static const char *filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text(OBS_SETTING_UI_FILTER_NAME);
}

static bool filter_update_spout_sender_name(obs_properties_t *,
					    obs_property_t *, void *data)
{
	auto filter = (struct filter *)data;

	obs_data_t *settings = obs_source_get_settings(filter->context);
	filter_update(filter, settings);
	obs_data_release(settings);
	return true;
}

static obs_properties_t *filter_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	auto props = obs_properties_create();

	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	obs_properties_add_text(props, OBS_SETTING_UI_SENDER_NAME,
				obs_module_text(OBS_SETTING_UI_SENDER_NAME),
				OBS_TEXT_DEFAULT);

	obs_properties_add_button(props, OBS_SETTINGS_UI_BUTTON_TITLE,
				  obs_module_text(OBS_SETTINGS_UI_BUTTON_TITLE),
				  filter_update_spout_sender_name);

	return props;
}

static void filter_defaults(obs_data_t *defaults)
{
	obs_data_set_default_string(
		defaults, OBS_SETTING_UI_SENDER_NAME,
		obs_module_text(OBS_SETTING_DEFAULT_SENDER_NAME));
}

namespace Sender {

static void release(void *data)
{
	auto filter = (struct filter *)data;

	if (filter->sender_created) {
		spout.ReleaseSenderName(filter->sender_name.c_str());
	}
}

static void create(void *data, uint32_t width, uint32_t height)
{

	auto filter = (struct filter *)data;

	spout.CreateSender(
		filter->sender_name.c_str(), width, height,
		(HANDLE)gs_texture_get_shared_handle(filter->shared_texture),
		DXGI_FORMAT_R8G8B8A8_UNORM);

	filter->sender_created = true;
}

} // namespace Sender

namespace Texture {

static void reset_buffers(void *data, uint32_t width, uint32_t height)
{
	auto filter = (struct filter *)data;

	gs_texture_destroy(filter->texture_buffer1);
	gs_texture_destroy(filter->texture_buffer2);

	filter->texture_buffer1 = gs_texture_create(width, height,
						    filter->shared_format, 1,
						    NULL, GS_RENDER_TARGET);

	filter->texture_buffer2 = gs_texture_create(width, height,
						    filter->shared_format, 1,
						    NULL, GS_RENDER_TARGET);
}

static void reset(void *data, uint32_t width, uint32_t height)
{
	auto filter = (struct filter *)data;

	reset_buffers(filter, width, height);

	gs_texture_destroy(filter->shared_texture);

	filter->width = width;
	filter->height = height;

	filter->shared_texture =
		gs_texture_create(width, height, filter->shared_format, 1, NULL,
				  GS_RENDER_TARGET | GS_SHARED_TEX);

	Sender::release(filter);
	Sender::create(filter, width, height);
}

static void render(void *data, obs_source_t *target, uint32_t cx, uint32_t cy)
{
	auto filter = (struct filter *)data;

	if (filter->width != cx || filter->height != cy)
		Texture::reset(filter, cx, cy);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();
	gs_matrix_identity();

	filter->prev_target = gs_get_render_target();
	filter->prev_space = gs_get_color_space();

	gs_set_render_target_with_color_space(filter->buffer_swap
						      ? filter->texture_buffer1
						      : filter->texture_buffer2,
					      NULL, GS_CS_SRGB);

	gs_set_viewport(0, 0, filter->width, filter->height);

	struct vec4 background;
	vec4_zero(&background);

	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	obs_source_video_render(target);

	gs_blend_state_pop();
	gs_set_render_target_with_color_space(filter->prev_target, NULL,
					      filter->prev_space);

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	// Copy from oldest buffer
	gs_copy_texture(filter->shared_texture,
			!filter->buffer_swap ? filter->texture_buffer1
					     : filter->texture_buffer2);

	// Swap buffers
	filter->buffer_swap = !filter->buffer_swap;
}

} // namespace Texture

static void filter_render_callback(void *data, uint32_t cx, uint32_t cy)
{
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);

	if (!data)
		return;

	auto filter = (struct filter *)data;

	if (!filter->context)
		return;

	auto target = obs_filter_get_parent(filter->context);

	if (!target)
		return;

	auto target_width = obs_source_get_base_width(target);
	auto target_height = obs_source_get_base_height(target);

	if (target_width == 0 || target_height == 0)
		return;

	// Render latest
	Texture::render(filter, target, target_width, target_height);
}

static void filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	auto filter = (struct filter *)data;

	obs_remove_main_render_callback(filter_render_callback, filter);

	if (strcmp(filter->setting_sender_name, filter->sender_name.c_str()) !=
	    0) {
		obs_enter_graphics();

		// Relase current sender
		Sender::release(filter);

		// Change current sender
		filter->sender_name = std::string(filter->setting_sender_name);

		// Create current sender again
		Sender::create(filter, filter->width, filter->height);

		obs_leave_graphics();
	}

	obs_add_main_render_callback(filter_render_callback, filter);
}

static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
	auto filter = (struct filter *)bzalloc(sizeof(Spout2Texture::filter));

	// Baseline everything
	filter->prev_target = nullptr;

	filter->shared_texture = nullptr;
	filter->texture_buffer1 = nullptr;
	filter->texture_buffer2 = nullptr;

	filter->shared_format = OBS_PLUGIN_COLOR_SPACE;

	filter->width = 0;
	filter->height = 0;
	filter->sender_created = false;

	// Setup the obs context
	filter->context = source;

	// setup the ui setting
	filter->setting_sender_name =
		obs_data_get_string(settings, OBS_SETTING_UI_SENDER_NAME);

	// Copy it to our sendername
	filter->sender_name = std::string(filter->setting_sender_name);

	// force an update
	filter_update(filter, settings);

	return filter;
}

static void filter_destroy(void *data)
{
	auto filter = (struct filter *)data;

	if (filter) {
		obs_remove_main_render_callback(filter_render_callback, filter);

		if (filter->sender_created) {
			spout.ReleaseSenderName(filter->sender_name.c_str());
		}

		obs_enter_graphics();

		gs_texture_destroy(filter->texture_buffer1);
		gs_texture_destroy(filter->texture_buffer2);

		filter->texture_buffer1 = nullptr;
		filter->texture_buffer2 = nullptr;

		gs_texture_destroy(filter->shared_texture);

		filter->shared_texture = nullptr;
		filter->prev_target = nullptr;

		obs_leave_graphics();

		bfree(filter);
	}
}

static void filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	auto filter = (struct filter *)data;

	if (!filter->context)
		return;

	obs_source_skip_video_filter(filter->context);
}

// Writes a simple log entry to OBS
void report_version()
{
#ifdef DEBUG
	info("you can haz spout2-texture tooz (Version: %s)",
	     OBS_PLUGIN_VERSION_STRING);
#else
	info("obs-spout2-filter [mrmahgu] - version %s",
	     OBS_PLUGIN_VERSION_STRING);
#endif
}

} // namespace Spout2Texture

bool obs_module_load(void)
{
	auto filter_info = Spout2Texture::create_filter_info();

	obs_register_source(&filter_info);

	Spout2Texture::report_version();

	return true;
}

void obs_module_unload() {}
