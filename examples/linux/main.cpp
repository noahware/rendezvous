#include "gui/element.hpp"
#include "gui/gui.hpp"
#include "gui/builders.hpp"
#include "gui/comp/button.hpp"
#include "gui/comp/text.hpp"
#include "gui/comp/slider.hpp"
#include "gui/comp/checkbox.hpp"
#include "gui/comp/panel.hpp"
#include "gui/comp/text_box.hpp"
#include "gui/comp/combo_box.hpp"
#include "gui/comp/color_picker.hpp"
#include "gui/comp/plot_lines.hpp"
#include "gui/comp/value_inspector.hpp"
#include "gui/comp/key_bind.hpp"
#include "gui/animation.hpp"
#include "../common/log.hpp"
#include "render/impl/ogl.hpp"
#include "util/types.hpp"
#include "input/x11.hpp"
#include "input/key_names.hpp"

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <cstring>
#include <cmath>
#include <format>

rv::vector_2d<float> screen_size = { 1280.f, 720.f };

static bool demo_enabled = true;
static bool demo_smooth_scroll = false;
static float demo_radius = 40.f;
static float demo_rounding = 15.f;
static float demo_thickness = 2.f;
static float demo_range_lo = 0.2f;
static float demo_range_hi = 0.8f;
static cstd::int32_t demo_shape = 0;
static cstd::int32_t demo_easing = 0;
static bool demo_show_shadows = true;
static bool demo_show_grid = false;
static rv::color demo_fill_color = { 0.2f, 0.6f, 1.f, 1.f };
static rv::color demo_accent_color = { 1.f, 0.4f, 0.2f, 1.f };
static rv::key demo_toggle_key = rv::key::none;
static float demo_anim_speed = 1.f;
static string_t demo_name = "rendezvous";
static string_t demo_notes = "A GPU-accelerated\n2D vector renderer.";

static float g_dbg_fps = 0.f;
static float g_dbg_ms = 0.f;
static cstd::int32_t g_dbg_frames = 0;

static shared_ptr_t<rv::x11_input> input;

static constexpr rv::easing easing_table[] =
{
	rv::easing::linear,
	rv::easing::ease_out_cubic,
	rv::easing::ease_out_back,
	rv::easing::ease_out_elastic,
	rv::easing::ease_out_bounce,
	rv::easing::ease_in_out_sine,
	rv::easing::ease_in_out_quad,
};

static void draw_canvas(rv::renderer& r, const rv::font& font, const rv::position canvas_min, const rv::position canvas_max)
{
	const float cw = canvas_max.x - canvas_min.x;
	const float ch = canvas_max.y - canvas_min.y;
	if (cw < 10.f || ch < 10.f) return;

	const rv::position center = { canvas_min.x + cw * 0.5f, canvas_min.y + ch * 0.45f };
	const float t = r.state().time;

	if (demo_show_grid)
	{
		const rv::color grid_col = { 1.f, 1.f, 1.f, 0.06f };
		for (float x = canvas_min.x; x < canvas_max.x; x += 50.f)
			r.draw_line({ x, canvas_min.y }, { x, canvas_max.y }, grid_col, 1.f);
		for (float y = canvas_min.y; y < canvas_max.y; y += 50.f)
			r.draw_line({ canvas_min.x, y }, { canvas_max.x, y }, grid_col, 1.f);
	}

	if (!demo_enabled) return;

	{
		const float gw = cw * 0.2f * (demo_range_hi - demo_range_lo + 0.3f);
		const float gh = ch * 0.2f;
		const rv::position gmin = { canvas_min.x + 20.f, canvas_min.y + 20.f };
		const rv::position gmax = { gmin.x + gw, gmin.y + gh };

		if (demo_show_shadows)
			r.draw_shadow_rect(gmin, gmax, { 0.f, 0.f, 0.f, 0.4f }, 12.f, 20.f, 0.f);

		r.draw_rect_filled_multi_color(gmin, gmax,
			{ 1.0f, 0.2f, 0.6f, 1.f },
			{ 1.0f, 0.5f, 0.0f, 1.f },
			{ 0.0f, 0.8f, 1.0f, 1.f },
			{ 0.5f, 0.0f, 1.0f, 1.f },
			12.f);
	}

	{
		const rv::position rc = { canvas_min.x + 70.f, canvas_min.y + ch * 0.2f + 60.f };
		if (demo_show_shadows)
			r.draw_shadow_circle(rc, 35.f, demo_accent_color, 15.f, false);
		r.draw_circle_filled_radial(rc, 35.f, demo_fill_color, { demo_fill_color.r, demo_fill_color.g, demo_fill_color.b, 0.f });
	}

	{
		const float rad = demo_radius;
		const float half = rad * 1.5f;

		switch (demo_shape)
		{
		case 0:
		{
			const rv::position smin = { center.x - half, center.y - half * 0.7f };
			const rv::position smax = { center.x + half, center.y + half * 0.7f };
			if (demo_show_shadows)
				r.draw_shadow_rect(smin, smax, { 0.f, 0.f, 0.f, 0.5f }, demo_rounding, 20.f, 0.f);
			r.draw_rect_filled(smin, smax, demo_fill_color, demo_rounding);
			r.draw_rect(smin, smax, demo_accent_color, demo_thickness, demo_rounding);
			break;
		}
		case 1:
		{
			if (demo_show_shadows)
				r.draw_shadow_circle(center, rad, { 0.f, 0.f, 0.f, 0.5f }, 20.f, false);
			r.draw_circle_filled(center, rad, demo_fill_color);
			r.draw_circle(center, rad, demo_accent_color, demo_thickness);
			break;
		}
		case 2:
		{
			const float s = rad * 1.2f;
			const rv::position p0 = { center.x, center.y - s };
			const rv::position p1 = { center.x - s * 0.866f, center.y + s * 0.5f };
			const rv::position p2 = { center.x + s * 0.866f, center.y + s * 0.5f };

			if (demo_show_shadows)
			{
				r.add_path_point(p0);
				r.add_path_point(p1);
				r.add_path_point(p2);
				r.draw_shadow_filled_path({ 0.f, 0.f, 0.f, 0.5f }, 20.f);
			}

			r.add_path_point(p0);
			r.add_path_point(p1);
			r.add_path_point(p2);
			r.draw_filled_path(demo_fill_color);

			r.add_path_point(p0);
			r.add_path_point(p1);
			r.add_path_point(p2);
			r.draw_lined_path(demo_accent_color, demo_thickness, true);
			break;
		}
		case 3:
		{
			const float outer = rad * 1.2f;
			const float inner = rad * 0.5f;
			constexpr cstd::int32_t points = 5;
			const float start_angle = -cstd::numbers::pi_f / 2.f;

			for (cstd::int32_t i = 0; i < points * 2; ++i)
			{
				const float angle = start_angle + static_cast<float>(i) * cstd::numbers::pi_f / static_cast<float>(points);
				const float r_val = (i % 2 == 0) ? outer : inner;
				r.add_path_point({ center.x + cstd::cosf(angle) * r_val, center.y + cstd::sinf(angle) * r_val });
			}
			r.draw_filled_path(demo_fill_color);

			for (cstd::int32_t i = 0; i < points * 2; ++i)
			{
				const float angle = start_angle + static_cast<float>(i) * cstd::numbers::pi_f / static_cast<float>(points);
				const float r_val = (i % 2 == 0) ? outer : inner;
				r.add_path_point({ center.x + cstd::cosf(angle) * r_val, center.y + cstd::sinf(angle) * r_val });
			}
			r.draw_lined_path(demo_accent_color, demo_thickness, true);
			break;
		}
		case 4:
		{
			const float sweep = std::fmod(t * demo_anim_speed, 4.f) / 4.f;
			const float a_min = -cstd::numbers::pi_f / 2.f;
			const float a_max = a_min + sweep * cstd::numbers::pi_f * 2.f;

			r.draw_circle(center, rad, { 1.f, 1.f, 1.f, 0.15f }, 1.f);

			if (sweep > 0.01f)
			{
				r.add_path_point(center);
				r.add_arc_path(center, rad, a_min, a_max, 32);
				r.draw_filled_path(demo_fill_color);
			}
			break;
		}
		default: break;
		}
	}

	{
		const float zone_x = canvas_max.x - cw * 0.25f;
		const float zone_y = canvas_min.y + 30.f;

		for (cstd::int32_t i = 0; i < 4; ++i)
		{
			const float thick = 1.f + static_cast<float>(i) * 1.5f;
			const float y = zone_y + static_cast<float>(i) * 20.f;
			const rv::color lc = { 0.5f + static_cast<float>(i) * 0.15f, 0.8f - static_cast<float>(i) * 0.1f, 1.f, 0.8f };
			r.draw_line({ zone_x - 60.f, y }, { zone_x + 60.f, y }, lc, thick);
		}

		const rv::position pent_center = { zone_x, zone_y + 110.f };
		const float pent_r = 30.f;
		for (cstd::int32_t i = 0; i < 5; ++i)
		{
			const float a = -cstd::numbers::pi_f / 2.f + static_cast<float>(i) * cstd::numbers::pi_f * 2.f / 5.f;
			r.add_path_point({ pent_center.x + cstd::cosf(a) * pent_r, pent_center.y + cstd::sinf(a) * pent_r });
		}

		if (demo_show_shadows)
		{
			for (cstd::int32_t i = 0; i < 5; ++i)
			{
				const float a = -cstd::numbers::pi_f / 2.f + static_cast<float>(i) * cstd::numbers::pi_f * 2.f / 5.f;
				r.add_path_point({ pent_center.x + cstd::cosf(a) * pent_r, pent_center.y + cstd::sinf(a) * pent_r });
			}
			r.draw_shadow_filled_path({ 0.f, 0.f, 0.f, 0.4f }, 15.f);
		}

		r.draw_filled_path({ 0.8f, 0.4f, 1.f, 0.7f });
	}

	{
		const float ball_y = canvas_max.y - 40.f;
		const float margin = 30.f;
		const float ball_x_min = canvas_min.x + margin;
		const float ball_x_max = canvas_max.x - margin;

		r.draw_line({ ball_x_min, ball_y }, { ball_x_max, ball_y }, { 1.f, 1.f, 1.f, 0.1f }, 1.f);

		const float raw_progress = std::fmod(t * demo_anim_speed, 2.f) / 2.f;
		const float ping_pong = raw_progress < 0.5f ? raw_progress * 2.f : 2.f - raw_progress * 2.f;

		const cstd::int32_t easing_idx = std::clamp(demo_easing, 0, static_cast<cstd::int32_t>(sizeof(easing_table) / sizeof(easing_table[0])) - 1);
		const float eased = rv::apply_easing(easing_table[easing_idx], ping_pong);

		const float ball_x = ball_x_min + eased * (ball_x_max - ball_x_min);
		r.draw_circle_filled(rv::position{ ball_x, ball_y }, 8.f, demo_accent_color);

		if (demo_show_shadows)
			r.draw_shadow_circle(rv::position{ ball_x, ball_y }, 8.f, demo_accent_color, 10.f, true);
	}

	{
		const rv::position clip_min = { canvas_max.x - cw * 0.25f - 50.f, canvas_max.y - ch * 0.35f };
		const rv::position clip_max = { clip_min.x + 100.f, clip_min.y + 70.f };

		r.draw_rect(clip_min, clip_max, { 1.f, 1.f, 1.f, 0.2f }, 1.f, 6.f);

		r.push_clip_rect(clip_min, clip_max, 6.f);
		const float bounce_x = (clip_min.x + clip_max.x) * 0.5f + cstd::sinf(t * 3.f) * 80.f;
		const float clip_cy = (clip_min.y + clip_max.y) * 0.5f;
		r.draw_circle_filled(rv::position{ bounce_x, clip_cy }, 30.f, { 1.f, 0.5f, 0.f, 0.8f });

		constexpr float clip_text_size = 12.f;
		const rv::position text_dim = r.calc_text_size(font, "Clipped!", clip_text_size);
		r.draw_text(font, { (clip_min.x + clip_max.x) * 0.5f - text_dim.x * 0.5f, clip_cy - text_dim.y * 0.5f },
			"Clipped!", { 1.f, 1.f, 1.f, 1.f }, clip_text_size);
		r.pop_clip_rect();
	}

	{
		const auto mp = input->mouse_pos();
		if (mp.x >= canvas_min.x && mp.x <= canvas_max.x && mp.y >= canvas_min.y && mp.y <= canvas_max.y)
		{
			if (input->is_mouse_down(0))
				r.draw_circle_filled_radial(mp, 20.f, demo_accent_color, { demo_accent_color.r, demo_accent_color.g, demo_accent_color.b, 0.f });
			else
				r.draw_circle(mp, 20.f, { 1.f, 1.f, 1.f, 0.3f }, 1.f);
		}
	}

	{
		const auto& state = r.state();
		const string_t info = std::format("{:.0f}x{:.0f}  {:.2f}s  {:.0f} fps", state.display_size.x, state.display_size.y, state.time, g_dbg_fps);
		constexpr float ts = 14.f;
		const rv::position tp = { canvas_min.x + 12.f, canvas_max.y - 60.f };
		r.add_text_shadow(font, tp, info, { 0.f, 0.f, 0.f, 0.6f }, 8.f, ts);
		r.draw_text(font, tp, info, { 1.f, 1.f, 1.f, 0.6f }, ts);
	}
}

int main(int argc, char* argv[])
{
	LOG_INFO("rendezvous demo (linux)");

	bool use_gl3 = false;
	for (cstd::int32_t i = 1; i < argc; ++i)
	{
		if (string_view_t(argv[i]) == "--opengl3")
			use_gl3 = true;
	}

	Display* display = XOpenDisplay(nullptr);
	if (!display)
	{
		LOG_ERR("failed to open X11 display");
		return 1;
	}

	const cstd::int32_t screen = DefaultScreen(display);
	Window root_win = RootWindow(display, screen);

	static cstd::int32_t visual_attribs[] =
	{
		GLX_RGBA,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER,
		None
	};

	XVisualInfo* vi = glXChooseVisual(display, screen, visual_attribs);
	if (!vi)
	{
		LOG_ERR("failed to choose GLX visual");
		XCloseDisplay(display);
		return 1;
	}

	Colormap cmap = XCreateColormap(display, root_win, vi->visual, AllocNone);

	XSetWindowAttributes swa = {};
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
	                 StructureNotifyMask;

	Window win = XCreateWindow(display, root_win, 0, 0,
		static_cast<cstd::uint32_t>(screen_size.x), static_cast<cstd::uint32_t>(screen_size.y),
		0, vi->depth, InputOutput, vi->visual,
		CWColormap | CWEventMask, &swa);

	XStoreName(display, win, "rendezvous demo");
	XMapWindow(display, win);

	Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, win, &wm_delete, 1);

	GLXContext glc = glXCreateContext(display, vi, nullptr, GL_TRUE);
	XFree(vi);

	if (!glc)
	{
		LOG_ERR("failed to create GLX context");
		XDestroyWindow(display, win);
		XCloseDisplay(display);
		return 1;
	}

	glXMakeCurrent(display, win, glc);

	shared_ptr_t<rv::renderer> renderer;

	if (use_gl3)
	{
		LOG_INFO("using OpenGL 3 backend");
		renderer = cstd::make_shared<rv::ogl3_renderer>();
	}
	else
	{
		LOG_INFO("using OpenGL 2 backend");
		renderer = cstd::make_shared<rv::ogl2_renderer>();
	}

	if (!renderer->init())
	{
		LOG_ERR("unable to init renderer");
		return 1;
	}

	input = cstd::make_shared<rv::x11_input>();
	input->set_window(display, win);

	optional_t<rv::font> font;
	const char* font_paths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		"/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
		"/usr/share/fonts/noto/NotoSans-Regular.ttf",
		nullptr
	};

	for (cstd::int32_t i = 0; font_paths[i]; ++i)
	{
		font = renderer->add_font(font_paths[i], 32.f);
		if (font)
		{
			LOG_INFO("loaded font: {}", font_paths[i]);
			break;
		}
	}

	auto gui_renderer = cstd::make_unique<rv::gui_renderer_impl>(renderer);
	auto gui = rv::make_gui(cstd::move(gui_renderer), input);

	gui->default_style().gap = 8.f;
	gui->default_style().direction = rv::layout_direction::vertical;

	shared_ptr_t<rv::gui_font_impl> gui_font;
	if (font)
	{
		gui_font = cstd::make_shared<rv::gui_font_impl>(*font);
		gui->set_font(gui_font);
	}

	rv::text_element* fps_label_ptr = nullptr;
	rv::plot_lines* fps_plot_ptr = nullptr;
	shared_ptr_t<rv::element> canvas_elem;

	if (gui_font)
	{
		auto root = gui->root();
		root->direction(rv::layout_direction::vertical).gap(0.f);

		auto header = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(42.f) });
		header->direction(rv::layout_direction::horizontal).padding(rv::border_vector{ 0.f, 16.f, 0.f, 16.f })
			.align(rv::alignment::center)
			.background_color({ 0.1f, 0.1f, 0.12f, 1.f })
			.border_width(rv::border_vector{ 0.f, 0.f, 1.f, 0.f }).border_color({ 0.25f, 0.25f, 0.3f, 1.f });

		auto title_label = gui->make_child<rv::text_element>(header, rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::auto_v() }, gui_font);
		title_label->content("rendezvous").text_size(22.f).text_color({ 0.4f, 0.7f, 1.f, 1.f });

		auto header_spacer = gui->make_child<rv::element>(header, rv::element_size{ rv::styled_size::px(0.f), rv::styled_size::px(1.f) });
		header_spacer->flex_grow(1.f);

		auto fps_label = gui->make_child<rv::text_element>(header, rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::auto_v() }, gui_font);
		fps_label->content("FPS: 0").text_size(14.f).text_color({ 0.6f, 0.6f, 0.65f, 1.f });
		fps_label_ptr = fps_label.get();

		auto body = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
		body->direction(rv::layout_direction::horizontal).gap(0.f);

		auto left_col = gui->make_child<rv::element>(body, rv::element_size{ rv::styled_size::px(390.f), rv::styled_size::fill() });
		left_col->direction(rv::layout_direction::vertical).gap(10.f).padding(12.f)
			.overflow(rv::overflow_mode::scroll).show_scrollbar(true)
			.background_color({ 0.09f, 0.09f, 0.11f, 1.f });

		{
			auto& buttons = left_col->add_container("Buttons");
			auto& btn_row = buttons.add_row();
			btn_row.add_button("Primary")
				.hover_color({ 0.25f, 0.55f, 1.f, 1.f })
				.pressed_color({ 0.1f, 0.35f, 0.8f, 1.f })
				.on_click([]() { LOG_INFO("primary clicked"); })
				.background_color({ 0.15f, 0.45f, 0.95f, 1.f }).rounding(8.f)
				.tooltip("Primary action button");
			btn_row.add_button("Secondary")
				.hover_color({ 0.3f, 0.3f, 0.35f, 1.f })
				.pressed_color({ 0.15f, 0.15f, 0.2f, 1.f })
				.on_click([]() { LOG_INFO("secondary clicked"); })
				.background_color({ 0.12f, 0.12f, 0.15f, 1.f }).rounding(8.f)
				.border_color({ 0.35f, 0.35f, 0.4f, 1.f }).border_width(1.f);
			btn_row.add_button("Danger")
				.hover_color({ 0.95f, 0.25f, 0.25f, 1.f })
				.pressed_color({ 0.65f, 0.1f, 0.1f, 1.f })
				.on_click([]() { LOG_INFO("danger clicked"); })
				.background_color({ 0.85f, 0.15f, 0.15f, 1.f }).rounding(8.f);
		}

		{
			auto& text_demo = left_col->add_container("Text");
			text_demo.add_label("Heading").text_size(24.f).text_color({ 0.4f, 0.7f, 1.f, 1.f });
			auto& paragraph = text_demo.add_label("The quick brown fox jumps over the lazy dog. This paragraph demonstrates text wrapping in the rendezvous GUI framework.");
			paragraph.set_declared_size({ rv::styled_size::fill(), rv::styled_size::auto_v() });
			auto& centered = text_demo.add_label("Center aligned");
			centered.text_alignment(rv::text_align::center);
			centered.set_declared_size({ rv::styled_size::fill(), rv::styled_size::auto_v() });
		}

		{
			auto& controls = left_col->add_container("Controls");
			controls.add_checkbox("Enable rendering").bind(&demo_enabled)
				.on_change([](const bool v) { LOG_INFO("rendering: {}", v); });
			controls.add_checkbox("Smooth scrolling").bind(&demo_smooth_scroll)
				.on_change([](const bool v) { LOG_INFO("smooth scrolling: {}", v); });

			auto& radius_label = controls.add_label("Radius: 40");
			controls.add_slider(5.f, 100.f, 40.f).bind(&demo_radius)
				.on_change([lbl = &radius_label](const float v) { lbl->content(std::format("Radius: {:.0f}", v)); })
				.fill_color({ 0.2f, 0.6f, 1.f, 1.f });

			auto& rounding_label = controls.add_label("Rounding: 15");
			controls.add_slider(0.f, 50.f, 15.f).bind(&demo_rounding)
				.on_change([lbl = &rounding_label](const float v) { lbl->content(std::format("Rounding: {:.0f}", v)); })
				.fill_color({ 0.2f, 0.8f, 0.5f, 1.f });

			auto& thick_label = controls.add_label("Thickness: 2.0");
			controls.add_slider(0.5f, 8.f, 2.f).bind(&demo_thickness)
				.on_change([lbl = &thick_label](const float v) { lbl->content(std::format("Thickness: {:.1f}", v)); })
				.fill_color({ 0.9f, 0.5f, 0.2f, 1.f });

			auto& range_label = controls.add_label("Gradient: 20% - 80%");
			controls.add_range_slider(0.f, 1.f, 0.2f, 0.8f)
				.on_range_change([lbl = &range_label](const float lo, const float hi)
				{
					demo_range_lo = lo;
					demo_range_hi = hi;
					lbl->content(std::format("Gradient: {:.0f}% - {:.0f}%", lo * 100.f, hi * 100.f));
				})
				.fill_color({ 0.8f, 0.3f, 0.6f, 1.f });

			controls.add_label("Shape:");
			controls.add_combo_box({ "Rectangle", "Circle", "Triangle", "Star", "Arc" })
				.bind(&demo_shape)
				.on_change([](const cstd::int32_t i) { LOG_INFO("shape: {}", i); });

			controls.add_label("Easing:");
			controls.add_combo_box({ "Linear", "Ease Out Cubic", "Ease Out Back", "Ease Out Elastic", "Ease Out Bounce", "Ease In Out Sine", "Ease In Out Quad" })
				.bind(&demo_easing)
				.on_change([](const cstd::int32_t i) { LOG_INFO("easing: {}", i); });
		}

		{
			auto& inputs = left_col->add_container("Text Input");
			auto& name_label = inputs.add_label("Name: rendezvous");
			auto& name_input = inputs.add_text_input("rendezvous");
			name_input.bind(&demo_name)
				.on_change([lbl = &name_label](const string_t& v) { lbl->content("Name: " + v); })
				.on_submit([](const string_t& v) { LOG_INFO("submitted: {}", v); });
			name_input.set_declared_size({ rv::styled_size::fill(), rv::styled_size::auto_v() });

			inputs.add_label("Notes (multiline):");
			auto& notes_area = inputs.add_text_area("A GPU-accelerated\n2D vector renderer.");
			notes_area.bind(&demo_notes);
			notes_area.set_declared_size({ rv::styled_size::fill(), rv::styled_size::px(120.f) });
		}

		{
			auto& scroll_section = left_col->add_container("Scroll Demo");
			auto scroll_box = gui->make_child<rv::element>(scroll_section.shared_from_this(),
				rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(150.f) });
			scroll_box->direction(rv::layout_direction::vertical).gap(4.f)
				.overflow(rv::overflow_mode::scroll).show_scrollbar(true);

			for (cstd::int32_t i = 0; i < 15; ++i)
			{
				auto btn = gui->make_child<rv::button>(scroll_box,
					rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(28.f) }, gui_font);
				btn->text(std::format("Item {}", i + 1)).text_size(14.f);
			}
		}

		auto right_col = gui->make_child<rv::element>(body, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
		right_col->direction(rv::layout_direction::vertical).gap(0.f);

		canvas_elem = gui->make_child<rv::element>(right_col, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
		canvas_elem->background_color({ 0.06f, 0.06f, 0.08f, 1.f });

		auto viz_row = gui->make_child<rv::element>(right_col, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(180.f) });
		viz_row->direction(rv::layout_direction::horizontal).gap(12.f).padding(8.f)
			.background_color({ 0.09f, 0.09f, 0.11f, 1.f })
			.border_width(rv::border_vector{ 1.f, 0.f, 0.f, 0.f }).border_color({ 0.25f, 0.25f, 0.3f, 1.f });

		auto fps_plot_elem = gui->make_child<rv::plot_lines>(viz_row,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() }, gui_font, input);
		fps_plot_elem->unbounded().view_window(300).overlay("frame ms").line_color({ 0.4f, 0.9f, 0.6f, 1.f });
		fps_plot_ptr = fps_plot_elem.get();

		{
			vector_t<float> sine_samples(96);
			for (cstd::size_t i = 0; i < sine_samples.size(); ++i)
				sine_samples[i] = cstd::sinf(static_cast<float>(i) * 0.13f);

			auto sine_plot = gui->make_child<rv::plot_lines>(viz_row,
				rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() }, gui_font, input);
			sine_plot->data(sine_samples).range(-1.f, 1.f).overlay("sine").line_color({ 0.6f, 0.5f, 1.f, 1.f });
		}

		auto inspector = gui->make_child<rv::value_inspector>(viz_row,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() }, gui_font);
		inspector->text_size(13.f)
			.watch("fps", &g_dbg_fps)
			.watch("frame ms", &g_dbg_ms)
			.watch("frame #", &g_dbg_frames)
			.watch("radius", &demo_radius)
			.watch("rounding", &demo_rounding);

		auto& settings_panel = gui->add_panel();
		settings_panel.set_declared_size({ rv::styled_size::px(300.f), rv::styled_size::px(400.f) });
		settings_panel.padding(16.f).gap(10.f)
			.inset_top(rv::styled_size::px(60.f)).inset_left(rv::styled_size::px(940.f));

		settings_panel.add_label("Settings").text_size(18.f).text_color({ 0.7f, 0.7f, 0.75f, 1.f });

		settings_panel.add_checkbox("Show shadows").bind(&demo_show_shadows);
		settings_panel.add_checkbox("Show grid").bind(&demo_show_grid);

		settings_panel.add_label("Fill color:").text_color({ 0.6f, 0.6f, 0.65f, 1.f });
		settings_panel.add_color_picker(demo_fill_color)
			.bind(&demo_fill_color)
			.on_change([](const rv::color) { });

		settings_panel.add_label("Accent color:").text_color({ 0.6f, 0.6f, 0.65f, 1.f });
		settings_panel.add_color_picker(demo_accent_color)
			.bind(&demo_accent_color)
			.on_change([](const rv::color) { });

		settings_panel.add_label("Toggle key:").text_color({ 0.6f, 0.6f, 0.65f, 1.f });
		settings_panel.add_key_bind()
			.bind(&demo_toggle_key)
			.on_change([](const rv::key k) { LOG_INFO("toggle key: {}", rv::key_name(k)); });

		auto& speed_label = settings_panel.add_label("Anim speed: 1.0x");
		settings_panel.add_slider(0.1f, 3.f, 1.f).bind(&demo_anim_speed)
			.on_change([lbl = &speed_label](const float v) { lbl->content(std::format("Anim speed: {:.1f}x", v)); })
			.fill_color({ 0.4f, 0.7f, 1.f, 1.f });

		auto& anim_panel = gui->add_panel();
		anim_panel.set_declared_size({ rv::styled_size::px(260.f), rv::styled_size::px(200.f) });
		anim_panel.padding(16.f).gap(10.f)
			.inset_top(rv::styled_size::px(480.f)).inset_left(rv::styled_size::px(940.f));

		anim_panel.add_label("Animations").text_size(18.f).text_color({ 0.7f, 0.7f, 0.75f, 1.f });

		auto& pulse_btn = anim_panel.add_button("Pulse");
		pulse_btn.background_color({ 0.2f, 0.2f, 0.25f, 1.f }).rounding(8.f);
		pulse_btn.animate(
			rv::keyframe_sequence{}
				.add(0.f, { .col = rv::color{ 0.2f, 0.2f, 0.25f, 1.f } })
				.add(0.5f, { .col = rv::color{ 0.85f, 0.15f, 0.15f, 1.f } })
				.add(1.f, { .col = rv::color{ 0.2f, 0.2f, 0.25f, 1.f } }),
			{ .duration = 1.5f, .ease = rv::easing::ease_in_out_quad, .iterations = -1 }
		);

		auto& slide_btn = anim_panel.add_button("Slide In");
		slide_btn.background_color({ 0.15f, 0.45f, 0.95f, 1.f }).rounding(8.f);
		slide_btn.animate(
			rv::keyframe_sequence{}
				.add(0.f, { .offset = rv::position{ -40.f, 0.f } })
				.add(1.f, { .offset = rv::position{ 0.f, 0.f } }),
			{ .duration = 0.8f, .ease = rv::easing::ease_out_back }
		);
		slide_btn.animate(
			rv::keyframe_sequence{}
				.add(0.f, { .opacity = 0.f })
				.add(1.f, { .opacity = 1.f }),
			{ .duration = 0.8f, .ease = rv::easing::ease_out_quad }
		);

		auto& fade_btn = anim_panel.add_button("Fade In");
		fade_btn.background_color({ 0.2f, 0.7f, 0.4f, 1.f }).rounding(8.f);
		fade_btn.animate(
			rv::keyframe_sequence{}
				.add(0.f, { .opacity = 0.f })
				.add(1.f, { .opacity = 1.f }),
			{ .duration = 1.2f, .ease = rv::easing::ease_out_cubic }
		);
	}

	bool running = true;

	while (running)
	{
		while (XPending(display))
		{
			XEvent event;
			XNextEvent(display, &event);

			if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == wm_delete)
			{
				running = false;
				break;
			}

			if (event.type == ConfigureNotify)
			{
				screen_size.x = static_cast<float>(event.xconfigure.width);
				screen_size.y = static_cast<float>(event.xconfigure.height);
			}

			input->handle_event(event);
		}

		if (!running) break;

		glDisable(GL_SCISSOR_TEST);
		glClearColor(0.08f, 0.08f, 0.1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_SCISSOR_TEST);

		renderer->begin_frame(screen_size);
		input->set_cursor(rv::cursor_type::arrow);

		g_dbg_ms = renderer->state().delta_time * 1000.f;
		g_dbg_fps = g_dbg_ms > 0.f ? 1000.f / g_dbg_ms : 0.f;
		++g_dbg_frames;
		if (fps_plot_ptr) fps_plot_ptr->push_value(g_dbg_ms);
		if (fps_label_ptr) fps_label_ptr->content(std::format("FPS: {:.0f}", g_dbg_fps));

		if (demo_toggle_key != rv::key::none && input->is_key_pressed(static_cast<cstd::int32_t>(demo_toggle_key)))
			demo_enabled = !demo_enabled;

		gui->default_style().smooth_scroll = demo_smooth_scroll;
		gui->render(screen_size);

		if (font && canvas_elem)
		{
			const auto c_min = canvas_elem->visual_pos();
			const auto c_size = canvas_elem->computed_size();
			const auto c_max = rv::position{ c_min.x + c_size.x, c_min.y + c_size.y };

			renderer->push_clip_rect(c_min, c_max);
			draw_canvas(*renderer, *font, c_min, c_max);
			renderer->pop_clip_rect();
		}

		renderer->end_frame();

		glXSwapBuffers(display, win);
		input->reset();
	}

	canvas_elem.reset();
	input.reset();
	renderer.reset();
	glXMakeCurrent(display, None, nullptr);
	glXDestroyContext(display, glc);
	XDestroyWindow(display, win);
	XCloseDisplay(display);

	return 0;
}
