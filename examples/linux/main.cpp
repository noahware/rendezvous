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
	auto gui = rv::make_gui(cstd::move(gui_renderer), input);

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
		static bool fac_enabled = true;
		static float fac_volume = 0.5f;
		static string_t fac_name = "edit me";
		static string_t fac_notes = "Line one\nLine two";
		static int fac_fruit = 0;

		// Buttons. Widget-specific setters (hover/pressed/on_click) chain first while the result
		// is still a button&; base-element setters (background_color/rounding/tooltip) come after.
		auto& buttons = gui->add_container("Buttons");
		auto& btn_row = buttons.add_row();
		btn_row.add_button("Primary")
			.hover_color({ 0.25f, 0.55f, 1.f, 1.f })
			.pressed_color({ 0.1f, 0.35f, 0.8f, 1.f })
			.on_click([]() { LOG_INFO("primary clicked"); })
			.background_color({ 0.15f, 0.45f, 0.95f, 1.f }).rounding(8.f)
			.tooltip("Submit the form\nShortcut: Enter");
		btn_row.add_button("Secondary")
			.hover_color({ 1.0f, 0.2f, 0.25f, 1.f })
			.pressed_color({ 0.08f, 1.0f, 0.1f, 1.f })
			.on_click([]() { LOG_INFO("secondary clicked"); })
			.background_color({ 0.12f, 0.12f, 0.15f, 0.f }).rounding(8.f)
			.border_color({ 0.35f, 0.35f, 0.4f, 1.f }).border_width(1.f);
		btn_row.add_button("Delete")
			.hover_color({ 0.95f, 0.25f, 0.25f, 1.f })
			.pressed_color({ 0.65f, 0.1f, 0.1f, 1.f })
			.on_click([]() { LOG_INFO("delete clicked"); })
			.background_color({ 0.85f, 0.15f, 0.15f, 1.f }).rounding(8.f);

		// Text.
		auto& text_demo = gui->add_container("Text");
		text_demo.add_label("Hello World").text_size(24.f).text_color({ 1.f, 0.f, 0.f, 1.f });
		text_demo.add_label("The quick brown fox jumps over the lazy dog and keeps on running.");

		// Controls with live labels + two-way binding.
		auto& controls = gui->add_container("Controls");
		controls.add_checkbox("Enable feature").bind(&fac_enabled)
			.on_change([](const bool v) { LOG_INFO("checkbox: {}", v); });

		auto& vol_label = controls.add_label("Volume: 50%");
		controls.add_slider(0.f, 1.f, 0.5f)
			.on_change([lbl = &vol_label](const float v) { lbl->content(std::format("Volume: {}%", static_cast<int>(v * 100.f))); })
			.fill_color({ 0.2f, 0.8f, 0.4f, 1.f })
			.bind(&fac_volume);

		auto& range_label = controls.add_label("Range: 25% - 75%");
		controls.add_range_slider(0.f, 1.f, 0.25f, 0.75f)
			.on_range_change([lbl = &range_label](const float lo, const float hi)
			{
				lbl->content(std::format("Range: {}% - {}%", static_cast<int>(lo * 100.f), static_cast<int>(hi * 100.f)));
			})
			.fill_color({ 0.9f, 0.4f, 0.2f, 1.f });

		controls.add_label("Favorite fruit:");
		controls.add_combo_box({ "Apple", "Banana", "Cherry", "Date", "Elderberry" })
			.bind(&fac_fruit)
			.on_change([](const int i) { LOG_INFO("combo selected: {}", i); })
			.tooltip("Choose which fruit you like best");

		// Text input.
		auto& inputs = gui->add_container("Text input");
		auto& name_label = inputs.add_label("Name: edit me");
		inputs.add_text_input("edit me")
			.bind(&fac_name)
			.on_change([lbl = &name_label](const string_t& v) { lbl->content("Name: " + v); })
			.on_submit([](const string_t& v) { LOG_INFO("submitted: {}", v); });
		inputs.add_label("Notes (multiline):");
		inputs.add_text_area("Line one\nLine two").bind(&fac_notes);

		// A floating, draggable/resizable panel — also built entirely with factories.
		static bool p_feature = false;
		auto& demo_panel = gui->add_panel();
		demo_panel.padding(16.f).gap(12.f)
			.inset_top(rv::styled_size::px(150.f)).inset_left(rv::styled_size::px(660.f));
		demo_panel.add_label("Draggable / resizable panel").text_color({ 0.7f, 0.7f, 0.75f, 1.f });
		demo_panel.add_checkbox("Checkbox").bind(&p_feature);
		demo_panel.add_slider(0.f, 1.f, 0.5f).fill_color({ 0.8f, 0.2f, 0.4f, 1.f });
		demo_panel.add_button("Button").background_color({ 0.2f, 0.6f, 0.3f, 1.f }).rounding(6.f);

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
