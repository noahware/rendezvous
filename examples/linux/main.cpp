#include "gui/element.hpp"
#include "gui/gui.hpp"
#include "gui/comp/button.hpp"
#include "gui/comp/text.hpp"
#include "gui/comp/slider.hpp"
#include "gui/comp/checkbox.hpp"
#include "gui/comp/panel.hpp"
#include "gui/comp/text_box.hpp"
#include "gui/comp/combo_box.hpp"
#include "log/log.hpp"
#include "render/impl/ogl.hpp"
#include "util/types.hpp"
#include "input/x11.hpp"

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <cstring>
#include <cmath>
#include <format>

rv::vector_2d<float> screen_size = { 1280.f, 720.f };

int main(int argc, char* argv[])
{
	LOG_INFO("rendezvous (linux)");

	// backend selection: --opengl3 for GL3, default GL2
	bool use_gl3 = false;
	for (int i = 1; i < argc; ++i)
	{
		if (string_view_t(argv[i]) == "--opengl3")
			use_gl3 = true;
	}

	// open X11 display
	Display* display = XOpenDisplay(nullptr);
	if (!display)
	{
		LOG_ERR("failed to open X11 display");
		return 1;
	}

	const int screen = DefaultScreen(display);
	Window root_win = RootWindow(display, screen);

	// choose GLX visual
	static int visual_attribs[] =
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

	// create colormap + window
	Colormap cmap = XCreateColormap(display, root_win, vi->visual, AllocNone);

	XSetWindowAttributes swa = {};
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
	                 StructureNotifyMask;

	Window win = XCreateWindow(display, root_win, 0, 0,
		static_cast<unsigned int>(screen_size.x), static_cast<unsigned int>(screen_size.y),
		0, vi->depth, InputOutput, vi->visual,
		CWColormap | CWEventMask, &swa);

	XStoreName(display, win, "rendezvous");
	XMapWindow(display, win);

	// handle window close
	Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, win, &wm_delete, 1);

	// create GL context
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

	// create renderer
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

	auto input = cstd::make_shared<rv::x11_input>();

	auto gui_renderer = cstd::make_unique<rv::gui_renderer_impl>(renderer);
	auto gui = cstd::make_unique<rv::gui>(cstd::move(gui_renderer), input);

	gui->default_style().gap = 8.f;
	gui->default_style().direction = rv::layout_direction::vertical;

	auto gui_root = gui->root();
	gui_root->direction(rv::layout_direction::vertical);

	// load font — try common Linux paths
	optional_t<rv::font> font;
	const char* font_paths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		"/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
		nullptr
	};

	for (int i = 0; font_paths[i]; ++i)
	{
		font = renderer->add_font(font_paths[i], 32.f);
		if (font)
		{
			LOG_INFO("loaded font: {}", font_paths[i]);
			break;
		}
	}

	shared_ptr_t<rv::gui_font_impl> gui_font;
	if (font)
	{
		gui_font = cstd::make_shared<rv::gui_font_impl>(*font);
		gui->set_font(gui_font);
	}

	// GUI elements
	if (gui_font)
	{
		// --- button row ---------------------------------------------------
		auto btn_row = gui->make_child<rv::element>(gui_root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(50.f) });
		btn_row->direction(rv::layout_direction::horizontal).gap(12.f).padding(8.f);

		auto primary = gui->make_child<rv::button>(btn_row,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::fill() }, gui_font);
		primary->background_color({ 0.15f, 0.45f, 0.95f, 1.f }).rounding(8.f).text_size(18.f);
		primary->text("Primary")
			.hover_color({ 0.25f, 0.55f, 1.f, 1.f })
			.pressed_color({ 0.1f, 0.35f, 0.8f, 1.f })
			.on_click([]() { LOG_INFO("primary clicked"); });
		primary->tooltip("Submit the form\nShortcut: Enter");

		auto secondary = gui->make_child<rv::button>(btn_row,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::fill() }, gui_font);
		secondary->background_color({ 0.12f, 0.12f, 0.15f, 0.f })
			.rounding(8.f)
			.border_color({ 0.35f, 0.35f, 0.4f, 1.f })
			.border_width(1.f);
		secondary->text_size(18.f);
		secondary->text("Secondary")
			.hover_color({ 1.0f, 0.2f, 0.25f, 1.f })
			.pressed_color({ 0.08f, 1.0f, 0.1f, 1.f })
			.on_click([]() { LOG_INFO("secondary clicked"); })
			.transition_speed(6.f);

		auto danger = gui->make_child<rv::button>(btn_row,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::fill() }, gui_font);
		danger->background_color({ 0.85f, 0.15f, 0.15f, 1.f }).rounding(8.f).text_size(18.f);
		danger->text("Delete")
			.hover_color({ 0.95f, 0.25f, 0.25f, 1.f })
			.pressed_color({ 0.65f, 0.1f, 0.1f, 1.f })
			.on_click([]() { LOG_INFO("delete clicked"); });

		auto icon_btn = gui->make_child<rv::button>(btn_row,
			rv::element_size{ rv::styled_size::px(40.f), rv::styled_size::px(40.f) }, gui_font);
		icon_btn->background_color({ 0.25f, 0.25f, 0.3f, 1.f }).rounding(20.f);
		icon_btn->hover_color({ 0.35f, 0.35f, 0.42f, 1.f })
			.pressed_color({ 0.18f, 0.18f, 0.22f, 1.f })
			.on_click([]() { LOG_INFO("icon clicked"); });
		icon_btn->tooltip("Settings");

		// --- text ---------------------------------------------------------
		auto label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		label->content("Hello World").text_size(24.f).text_color({ 1.f, 0.f, 0.f, 1.f });

		auto para = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{ rv::styled_size::px(400.f), rv::styled_size::auto_v() }, gui_font);
		para->content("The quick brown fox jumps over the lazy dog. This text should wrap across multiple lines automatically.")
			.text_size(18.f);

		auto centered = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::auto_v() }, gui_font);
		centered->content("Centered Heading")
			.text_alignment(rv::text_align::center).text_size(28.f);

		// --- sliders ------------------------------------------------------
		auto slider_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		slider_label->content("Volume: 50%").text_size(16.f);

		auto vol_slider = gui->make_child<rv::slider<>>(gui_root,
			rv::element_size{ rv::styled_size::px(300.f), rv::styled_size::px(30.f) }, input);
		vol_slider->value(0.5f)
			.on_change([slider_label](const float v)
			{
				const int pct = static_cast<int>(v * 100.f);
				slider_label->content("Volume: " + string_t(std::to_string(pct)) + "%");
			})
			.padding({ .top = 0, .right = 10.f, .bottom = 0.f, .left = 10.f })
			.rounding(17.5f);

		auto range_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		range_label->content("Range: 25% - 75%").text_size(16.f);

		auto price_range = gui->make_child<rv::range_slider<>>(gui_root,
			rv::element_size{ rv::styled_size::px(300.f), rv::styled_size::px(30.f) }, input);
		price_range->values(0.25f, 0.75f)
			.on_range_change([range_label](const float lo, const float hi)
			{
				const int lo_pct = static_cast<int>(lo * 100.f);
				const int hi_pct = static_cast<int>(hi * 100.f);
				range_label->content("Range: " + string_t(std::to_string(lo_pct)) + "% - " + string_t(std::to_string(hi_pct)) + "%");
			})
			.fill_color({ 0.9f, 0.4f, 0.2f, 1.f })
			.padding({ .top = 0, .right = 10.f, .bottom = 0.f, .left = 10.f })
			.rounding(17.5f);

		auto int_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		int_label->content("Level: 5").text_size(16.f);

		auto int_slider = gui->make_child<rv::slider<int>>(gui_root,
			rv::element_size{ rv::styled_size::px(300.f), rv::styled_size::px(30.f) }, input);
		int_slider->range(0, 10).value(5)
			.on_change([int_label](const int v)
			{
				int_label->content("Level: " + string_t(std::to_string(v)));
			})
			.fill_color({ 0.2f, 0.8f, 0.4f, 1.f });

		static float bound_volume = 0.5f;
		static int bound_level = 5;
		vol_slider->bind(&bound_volume);
		int_slider->bind(&bound_level);

		// --- checkboxes ---------------------------------------------------
		static bool feature_enabled = true;

		auto cb = gui->make_child<rv::checkbox>(gui_root,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::auto_v() }, gui_font);
		cb->label("Enable feature").checked(true).bind(&feature_enabled)
			.on_change([](const bool v)
			{
				LOG_INFO("checkbox: {}", v);
			})
			.text_size(16.f);

		auto cb2 = gui->make_child<rv::checkbox>(gui_root,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::auto_v() }, gui_font);
		cb2->label("Round checkbox")
			.on_change([](const bool v)
			{
				LOG_INFO("round checkbox: {}", v);
			})
			.text_size(16.f).rounding(10.f);

		// --- text boxes ---------------------------------------------------
		auto name_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		name_label->content("Name:").text_size(16.f);

		static string_t name_value = "edit me";

		auto name_input = gui->make_child<rv::text_box>(gui_root,
			rv::element_size{ rv::styled_size::px(300.f), rv::styled_size::auto_v() }, gui_font, input);
		name_input->text_size(18.f)
			.padding({ .top = 8.f, .right = 10.f, .bottom = 8.f, .left = 10.f })
			.rounding(6.f);
		name_input->bind(&name_value)
			.on_submit([](const string_t& v) { LOG_INFO("submitted: {}", v); })
			.on_change([name_label](const string_t& v)
			{
				name_label->content("Name: " + v);
			});

		auto notes_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		notes_label->content("Notes (multiline):").text_size(16.f);

		auto notes_input = gui->make_child<rv::text_box>(gui_root,
			rv::element_size{ rv::styled_size::px(400.f), rv::styled_size::px(120.f) }, gui_font, input);
		notes_input->multiline(true)
			.text("Line one\nLine two")
			.text_size(18.f)
			.padding({ .top = 8.f, .right = 10.f, .bottom = 8.f, .left = 10.f })
			.rounding(6.f);

		// --- combo box ----------------------------------------------------
		auto combo_label = gui->make_child<rv::text_element>(gui_root,
			rv::element_size{}, gui_font);
		combo_label->content("Favorite fruit:").text_size(16.f);

		static int selected_fruit = 0;
		auto combo = gui->make_child<rv::combo_box>(gui_root,
			rv::element_size{ rv::styled_size::px(220.f), rv::styled_size::px(32.f) }, gui_font, input);
		combo->text_size(16.f);
		combo->options({ "Apple", "Banana", "Cherry", "Date", "Elderberry" })
			.bind(&selected_fruit)
			.on_change([](const int i) { LOG_INFO("combo selected: {}", i); });
		combo->tooltip("Choose which fruit you like best");

		// --- draggable / resizable panel ----------------------------------
		auto demo_panel = gui->make_child<rv::panel>(gui_root,
			rv::element_size{ rv::styled_size::px(350.f), rv::styled_size::px(250.f) }, input);

		demo_panel->draggable(true)
			.resizable(true)
			.min_panel_size(250.f, 150.f)
			.shadow({ 0.f, 0.f, 0.f, 0.6f }, 10.f)
			.positioning(rv::position_type::absolute)
			.inset_top(rv::styled_size::px(150.f))
			.inset_left(rv::styled_size::px(600.f));

		auto panel_content = gui->make_child<rv::element>(demo_panel,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
		panel_content->direction(rv::layout_direction::vertical).padding(16.f).gap(12.f)
			.overflow(rv::overflow_mode::scroll);

		auto p_text = gui->make_child<rv::text_element>(panel_content,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::auto_v() }, gui_font);
		p_text->content("The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.")
			.text_size(16.f).text_color({ 0.7f, 0.7f, 0.75f, 1.f });

		static bool p_feature = false;
		auto p_cb = gui->make_child<rv::checkbox>(panel_content,
			rv::element_size{ rv::styled_size::auto_v(), rv::styled_size::auto_v() }, gui_font);
		p_cb->text_size(16.f);
		p_cb->label("Checkbox").bind(&p_feature);

		auto p_slider_label = gui->make_child<rv::text_element>(panel_content,
			rv::element_size{}, gui_font);
		p_slider_label->content("Slider ").text_size(16.f);

		auto p_slider = gui->make_child<rv::slider<>>(panel_content,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(30.f) }, input);
		p_slider->range(0.1f, 5.0f).value(1.0f)
			.on_change([p_slider_label](const float v) {
				p_slider_label->content(std::format("Slider: {:.1f}", v));
			})
			.fill_color({ 0.8f, 0.2f, 0.4f, 1.f });
		p_slider->padding({ .top = 0, .right = 10.f, .bottom = 0.f, .left = 10.f })
			.rounding(17.5f);

		auto p_btn = gui->make_child<rv::button>(panel_content,
			rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(35.f) }, gui_font);
		p_btn->text("Button")
			.hover_color({ 0.3f, 0.7f, 0.4f, 1.f })
			.pressed_color({ 0.15f, 0.5f, 0.25f, 1.f });
		p_btn->text_size(16.f)
			.background_color({ 0.2f, 0.6f, 0.3f, 1.f });
		p_btn->shadow({ 0.2f, 0.6f, 0.3f, 0.25f });
	}

	// main loop
	bool running = true;

	while (running)
	{
		// process all pending events
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

		// clear
		glDisable(GL_SCISSOR_TEST);
		glClearColor(0.1f, 0.1f, 0.1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_SCISSOR_TEST);

		// render
		renderer->begin_frame(screen_size);
		input->set_cursor(rv::cursor_type::arrow);

		// red filled rectangle with a basic black dropshadow
		renderer->draw_shadow_rect({ 105.f, 105.f }, { 305.f, 255.f }, { 0.f, 0.f, 0.f, 0.5f }, 17.5f, 20.f, 0.f);
		renderer->draw_rect_filled({ 100.f, 100.f }, { 300.f, 250.f }, { 0.2f, 0.2f, 0.2f, 1.f }, 17.5f);
		renderer->draw_rect({ 100.f, 100.f }, { 300.f, 250.f }, { 0.3f, 0.3f, 0.3f, 1.f }, 1.f, 17.5f);

		// green outlined rectangle with a subtle black glow
		renderer->draw_shadow_rect({ 400.f, 100.f }, { 600.f, 250.f }, { 0.f, 0.f, 0.f, 0.3f }, 8.f, 40.f, 0.f);
		renderer->draw_rect({ 400.f, 100.f }, { 600.f, 250.f }, { 0.f, 1.f, 0.f, 1.f }, 2.f, 8.f);

		// standalone blue shadow rect with a clipped magenta circle that follows the mouse
		renderer->draw_shadow_rect({ 700.f, 100.f }, { 900.f, 250.f }, { 0.f, 0.5f, 1.f, 0.8f }, 17.5f, 35.f, 3.f, rv::rounding_flags_all, true);
		renderer->push_clip_rect({ 700.f, 100.f }, { 900.f, 250.f }, 17.5f, rv::rounding_flags_all);
		renderer->draw_circle_filled(input->mouse_pos(), 25.f, { 1.f, 0.f, 1.f, 1.f });
		renderer->pop_clip_rect();

		// gradient rect
		renderer->draw_rect_filled_multi_color(
			{ 1000.f, 100.f }, { 1200.f, 250.f },
			{ 1.0f, 0.2f, 0.6f, 1.f }, { 1.0f, 0.5f, 0.0f, 1.f },
			{ 0.0f, 0.8f, 1.0f, 1.f }, { 0.5f, 0.0f, 1.0f, 1.f }, 20.f);

		// red filled rectangle with a really thick shadow
		renderer->draw_shadow_rect({ 100.f, 350.f }, { 300.f, 500.f }, { 0.f, 0.f, 0.f, 0.6f }, 17.5f, 10.f, 20.f);
		renderer->draw_rect_filled({ 100.f, 350.f }, { 300.f, 500.f }, { 1.f, 0.f, 0.f, 1.f }, 17.5f);

		// green filled rectangle with only the top left and bottom right corners rounded
		constexpr rv::rounding_flags selective_flags = static_cast<rv::rounding_flags>(rv::rounding_flags_top_left | rv::rounding_flags_bottom_right);
		renderer->draw_shadow_rect({ 400.f, 350.f }, { 600.f, 500.f }, { 0.f, 0.f, 0.f, 0.6f }, 30.f, 25.f, 0.f, selective_flags);
		renderer->draw_rect_filled({ 400.f, 350.f }, { 600.f, 500.f }, { 0.f, 1.f, 0.f, 1.f }, 30.f, selective_flags);

		const auto mouse_pos = input->mouse_pos();
		if (input->is_mouse_down(0))
			renderer->draw_circle_filled_radial(mouse_pos, 25.f, { 0.f, 0.5f, 1.f, 1.f }, { 0.f, 0.5f, 1.f, 0.f });
		else
			renderer->draw_circle(mouse_pos, 25.f, { 0.f, 0.5f, 1.f, 1.f }, 1.f);

		// scroll example — accumulate the wheel delta and move a scrollbar thumb
		static float cumulative_scroll = 0.f;
		cumulative_scroll += input->scroll_delta();
		if (cumulative_scroll > 10.f) cumulative_scroll = 10.f;
		if (cumulative_scroll < -10.f) cumulative_scroll = -10.f;
		const float thumb_y = 380.f - (cumulative_scroll * 18.f);

		renderer->draw_rect_filled({ 20.f, 200.f }, { 40.f, 600.f }, { 0.2f, 0.2f, 0.2f, 0.8f }, 10.f);
		renderer->draw_rect_filled({ 20.f, thumb_y }, { 40.f, thumb_y + 40.f }, { 0.8f, 0.8f, 0.8f, 1.f }, 10.f);

		// shadow circle (with hollow cutout)
		renderer->draw_shadow_circle({ 900.f, 450.f }, 40.f, { 1.f, 0.f, 0.f, 1.f }, 20.f, true);

		// shadow line (drawn under a solid line)
		renderer->draw_shadow_line({ 980.f, 410.f }, { 1080.f, 490.f }, { 0.f, 1.f, 0.f, 1.f }, 5.f, 15.f);
		renderer->draw_line({ 980.f, 410.f }, { 1080.f, 490.f }, { 1.f, 1.f, 1.f, 1.f }, 5.f);

		// shadow poly (drawn under a solid poly)
		renderer->add_path_point({ 1120.f, 490.f });
		renderer->add_path_point({ 1170.f, 410.f });
		renderer->add_path_point({ 1220.f, 490.f });
		renderer->draw_shadow_filled_path({ 0.f, 0.f, 1.f, 1.f }, 25.f);

		renderer->add_path_point({ 1120.f, 490.f });
		renderer->add_path_point({ 1170.f, 410.f });
		renderer->add_path_point({ 1220.f, 490.f });
		renderer->draw_filled_path({ 1.f, 1.f, 1.f, 1.f });

		// animated radial fill using an arc path
		const float fill_progress = std::fmod(renderer->state().time, 2.0f) / 2.0f;
		const float a_min = -cstd::numbers::pi_f / 2.0f;
		const float a_max = a_min + (fill_progress * cstd::numbers::pi_f * 2.0f);

		renderer->draw_circle({ 750.f, 450.f }, 50.f, { 1.f, 1.f, 1.f, 1.f }, 1.f);

		if (fill_progress > 0.01f)
		{
			renderer->add_path_point({ 750.f, 450.f });
			renderer->add_arc_path({ 750.f, 450.f }, 50.f, a_min, a_max, 32);
			renderer->draw_filled_path({ 1.f, 1.f, 1.f, 0.6f });
		}

		// clip rect example — a bouncing circle and label clipped to a region
		const rv::position clip_min = { 400.f, 100.f };
		const rv::position clip_max = { 600.f, 250.f };
		const float bounce_x = 500.f + std::sin(renderer->state().time * 3.f) * 150.f;

		renderer->push_clip_rect(clip_min, clip_max);
		renderer->draw_circle_filled({ bounce_x, 175.f }, 40.f, { 1.f, 0.5f, 0.f, 1.f });

		if (font)
		{
			constexpr float text_size = 18.f;
			const string_t circle_text = "Clipped!";
			const rv::position text_dim = renderer->calc_text_size(*font, circle_text, text_size);
			renderer->draw_text(*font, { bounce_x - text_dim.x / 2.f, 175.f - text_dim.y / 2.f }, circle_text, { 1.f, 1.f, 1.f, 1.f }, text_size);
		}

		renderer->pop_clip_rect();

		if (font)
		{
			const auto state = renderer->state();
			const string_t text = std::format("width {}px height {}px time {:.2f}s delta {:.4f}s fps {:.2f}",
				state.display_size.x, state.display_size.y, state.time, state.delta_time, 1.f / state.delta_time);

			constexpr float size = 35.f;
			constexpr rv::position text_pos = { 100.f, 580.f };
			renderer->add_text_shadow(*font, text_pos, text, { 1.f, 0.4f, 1.f, 1.f }, 15.f, size);
			renderer->draw_text(*font, text_pos, text, { 0.4f, 1.f, 1.f, 1.f }, size);
		}

		gui->render(screen_size);
		renderer->end_frame();

		glXSwapBuffers(display, win);
		input->reset();
	}

	// cleanup
	renderer.reset();
	glXMakeCurrent(display, None, nullptr);
	glXDestroyContext(display, glc);
	XDestroyWindow(display, win);
	XCloseDisplay(display);

	return 0;
}
