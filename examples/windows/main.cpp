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
#include "render/impl/dx11.hpp"
#include "render/impl/ogl.hpp"
#include "input/win32.hpp"
#include "util/types.hpp"

// uncomment this if you want to try the example image rendering
// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

rv::vector_2d<float> screen_size = { 1280.f, 720.f };
shared_ptr_t<rv::win32_input> input = { };

static LRESULT CALLBACK wnd_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam)
{
	if (msg == WM_SIZE && wparam != SIZE_MINIMIZED)
	{
		screen_size.x = static_cast<float>(LOWORD(lparam));
		screen_size.y = static_cast<float>(HIWORD(lparam));
	}

	if (input)
	{
		input->handle_message(hwnd, msg, wparam, lparam);
	}

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static rv::plot_lines& add_data_visualization_demo(rv::gui& g)
{
	auto& viz = g.add_container("Visualization");
	viz.add_label("Frame time (ms) - hover to inspect");
	auto& fps_plot = viz.add_plot_lines();
	fps_plot.capacity(180).overlay("frame ms").line_color({ 0.4f, 0.9f, 0.6f, 1.f });

	viz.add_label("Static sine wave");
	vector_t<float> sine_samples(96);
	for (cstd::size_t i = 0; i < sine_samples.size(); ++i)
	{
		sine_samples[i] = cstd::sinf(static_cast<float>(i) * 0.13f);
	}
	viz.add_plot_lines().data(sine_samples).range(-1.f, 1.f);

	return fps_plot;
}

cstd::int32_t main(int argc, char* argv[])
{
	LOG_INFO("rendezvous");

	// runtime backend selection: --opengl for OGL2, --opengl3 for OGL3
	enum class backend_type { dx11, ogl2, ogl3 } backend = backend_type::dx11;
	for (int i = 1; i < argc; ++i)
	{
		if (string_view_t(argv[i]) == "--opengl3")
			backend = backend_type::ogl3;
		else if (string_view_t(argv[i]) == "--opengl")
			backend = backend_type::ogl2;
	}
	const bool use_opengl = (backend == backend_type::ogl2 || backend == backend_type::ogl3);

	WNDCLASSEXW wnd_class = { };
	wnd_class.cbSize = sizeof(wnd_class);
	wnd_class.lpfnWndProc = wnd_proc;
	wnd_class.hInstance = GetModuleHandleA(nullptr);
	wnd_class.lpszClassName = L"rv_window";

	RegisterClassExW(&wnd_class);

	const HWND hwnd = CreateWindowExW(0, L"rv_window", L"rendezvous", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
	                                  static_cast<cstd::int32_t>(screen_size.x),
	                                  static_cast<cstd::int32_t>(screen_size.y), nullptr, nullptr, wnd_class.hInstance,
	                                  nullptr);

	ShowWindow(hwnd, SW_SHOW);

	// DX11 objects (only used when !use_opengl)
	rv::dx11_object<IDXGISwapChain> swap_chain;
	rv::dx11_object<ID3D11Device> device;
	rv::dx11_object<ID3D11DeviceContext> context;
	rv::dx11_object<ID3D11RenderTargetView> rtv;

	// OGL2 objects (only used when use_opengl)
	HDC hdc = nullptr;
	HGLRC hglrc = nullptr;

	shared_ptr_t<rv::renderer> renderer;

	auto create_rtv = [&]()
		{
			rv::dx11_object<ID3D11Texture2D> back_buffer;

			swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.release_and_get()));
			device->CreateRenderTargetView(back_buffer.value(), nullptr, rtv.release_and_get());

			back_buffer.release();
		};

	if (use_opengl)
	{
		hdc = GetDC(hwnd);

		PIXELFORMATDESCRIPTOR pfd = { };
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;

		const int pixel_format = ChoosePixelFormat(hdc, &pfd);
		if (!pixel_format || !SetPixelFormat(hdc, pixel_format, &pfd))
		{
			LOG_ERR("failed to set pixel format");
			return 1;
		}

		if (backend == backend_type::ogl3)
		{
			LOG_INFO("using OpenGL 3 backend");

			// create temp context to load wglCreateContextAttribsARB
			HGLRC tmp_ctx = wglCreateContext(hdc);
			if (!tmp_ctx || !wglMakeCurrent(hdc, tmp_ctx))
			{
				LOG_ERR("failed to create temp WGL context");
				return 1;
			}

			using wglCreateContextAttribsARB_t = HGLRC(APIENTRY*)(HDC, HGLRC, const int*);
			auto wglCreateContextAttribsARB = reinterpret_cast<wglCreateContextAttribsARB_t>(
				wglGetProcAddress("wglCreateContextAttribsARB"));

			if (!wglCreateContextAttribsARB)
			{
				LOG_ERR("wglCreateContextAttribsARB not available");
				wglMakeCurrent(nullptr, nullptr);
				wglDeleteContext(tmp_ctx);
				return 1;
			}

			constexpr int attribs[] =
			{
				0x2091, 3,    // WGL_CONTEXT_MAJOR_VERSION_ARB
				0x2092, 3,    // WGL_CONTEXT_MINOR_VERSION_ARB
				0x9126, 0x2,  // WGL_CONTEXT_PROFILE_MASK_ARB = COMPATIBILITY_PROFILE_BIT
				0
			};

			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(tmp_ctx);

			hglrc = wglCreateContextAttribsARB(hdc, nullptr, attribs);
			if (!hglrc || !wglMakeCurrent(hdc, hglrc))
			{
				LOG_ERR("failed to create GL 3.3 core context");
				return 1;
			}

			renderer = cstd::make_shared<rv::ogl3_renderer>();
		}
		else
		{
			LOG_INFO("using OpenGL 2 backend");

			hglrc = wglCreateContext(hdc);
			if (!hglrc || !wglMakeCurrent(hdc, hglrc))
			{
				LOG_ERR("failed to create WGL context");
				return 1;
			}

			renderer = cstd::make_shared<rv::ogl2_renderer>();
		}
	}
	else
	{
		LOG_INFO("using Direct3D 11 backend");

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };

		swap_chain_desc.BufferCount = 2;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.OutputWindow = hwnd;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.Windowed = true;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		const HRESULT status = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		                                                     D3D11_SDK_VERSION, &swap_chain_desc, swap_chain.release_and_get(), device.release_and_get(),
		                                                     nullptr, context.release_and_get());

		if (status != S_OK)
		{
			LOG_ERR("unable to create device and swap chain");
			return 1;
		}

		create_rtv();

		renderer = cstd::make_shared<rv::dx11_renderer>(device.value(), context.value());
	}

	if (!renderer->init())
	{
		LOG_ERR("unable to init renderer");

		return 1;
	}

	input = cstd::make_shared<rv::win32_input>();

	auto gui_renderer = cstd::make_unique<rv::gui_renderer_impl>(renderer);
	auto gui = rv::make_gui(cstd::move(gui_renderer), input);

	gui->default_style().gap = 8.f;
	gui->default_style().direction = rv::layout_direction::vertical;

	auto root = gui->root();
	root->direction(rv::layout_direction::vertical);

	/*auto vis_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	vis_row->direction(rv::layout_direction::horizontal);

	auto vis_btn1 = gui->make_child<rv::button>(vis_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	auto vis_btn2 = gui->make_child<rv::button>(vis_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	vis_btn2->visible(false);
	auto vis_btn3 = gui->make_child<rv::button>(vis_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });

	auto jc_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	jc_row->direction(rv::layout_direction::horizontal).justify(rv::justify_content::center);

	gui->make_child<rv::button>(jc_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(jc_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(jc_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });

	auto jsb_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	jsb_row->direction(rv::layout_direction::horizontal).justify(rv::justify_content::space_between);

	gui->make_child<rv::button>(jsb_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(jsb_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(jsb_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });

	auto scroll_box = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(150.f) });
	scroll_box->direction(rv::layout_direction::vertical).gap(6.f)
		.overflow(rv::overflow_mode::scroll);

	for (int i = 0; i < 12; ++i)
	{
		gui->make_child<rv::button>(scroll_box, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(30.f) });
	}

	auto ac_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(80.f) });
	ac_row->direction(rv::layout_direction::horizontal).align(rv::alignment::center);

	gui->make_child<rv::button>(ac_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::px(20.f) });
	gui->make_child<rv::button>(ac_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::px(60.f) });
	gui->make_child<rv::button>(ac_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::px(40.f) });

	auto mg_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(60.f) });
	mg_row->direction(rv::layout_direction::horizontal);

	gui->make_child<rv::button>(mg_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	auto mg_btn = gui->make_child<rv::button>(mg_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	mg_btn->margin(rv::border_vector{ 10.f, 20.f, 10.f, 20.f });
	gui->make_child<rv::button>(mg_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });

	auto anim_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(50.f) });
	anim_row->direction(rv::layout_direction::horizontal).gap(16.f);

	auto fade_btn = gui->make_child<rv::button>(anim_row, rv::element_size{ rv::styled_size::px(120.f), rv::styled_size::fill() });
	fade_btn->animate(
		rv::keyframe_sequence{}
			.add(0.f, { .col = rv::color{1.f, 1.f, 1.f, 0.f} })
			.add(1.f, { .col = rv::color{1.f, 1.f, 1.f, 1.f} }),
		{ .duration = 10.f, .ease = rv::easing::ease_out_cubic }
	);

	auto slide_btn = gui->make_child<rv::button>(anim_row, rv::element_size{ rv::styled_size::px(120.f), rv::styled_size::fill() });
	slide_btn->animate(
		rv::keyframe_sequence{}
			.add(0.f, { .offset = rv::position{0.f, -40.f} })
			.add(1.f, { .offset = rv::position{0.f, 0.f} }),
		{ .duration = 0.6f, .ease = rv::easing::ease_out_back }
	);
	slide_btn->animate(
		rv::keyframe_sequence{}
			.add(0.f, { .opacity = 0.f })
			.add(1.f, { .opacity = 1.f }),
		{ .duration = 0.6f, .ease = rv::easing::ease_out_quad }
	);

	auto pulse_btn = gui->make_child<rv::button>(anim_row, rv::element_size{ rv::styled_size::px(120.f), rv::styled_size::fill() });
	pulse_btn->animate(
		rv::keyframe_sequence{}
			.add(0.f, { .col = rv::color{1.f, 1.f, 1.f, 1.f} })
			.add(0.5f, { .col = rv::color{1.f, 0.3f, 0.3f, 1.f} })
			.add(1.f, { .col = rv::color{1.f, 1.f, 1.f, 1.f} }),
		{ .duration = 1.5f, .ease = rv::easing::ease_in_out_quad, .iterations = -1 }
	);

	auto moving_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(50.f) });
	moving_row->direction(rv::layout_direction::horizontal).gap(8.f);
	moving_row->animate(
		rv::keyframe_sequence{}
			.add(0.f, { .offset = rv::position{0.f, 0.f} })
			.add(0.5f, { .offset = rv::position{30.f, 0.f} })
			.add(1.f, { .offset = rv::position{0.f, 0.f} }),
		{ .duration = 2.f, .ease = rv::easing::ease_in_out_sine, .iterations = -1 }
	);
	gui->make_child<rv::button>(moving_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(moving_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(moving_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });

	auto pad_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(60.f) });
	pad_row->direction(rv::layout_direction::horizontal).padding(12.f);
	gui->make_child<rv::button>(pad_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	gui->make_child<rv::button>(pad_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });

	auto sev_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	sev_row->direction(rv::layout_direction::horizontal).justify(rv::justify_content::space_evenly);
	gui->make_child<rv::button>(sev_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(sev_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(sev_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });

	auto fg_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	fg_row->direction(rv::layout_direction::horizontal).gap(4.f);
	auto fg1 = gui->make_child<rv::button>(fg_row, rv::element_size{ rv::styled_size::px(0.f), rv::styled_size::fill() });
	fg1->flex_grow(1.f);
	auto fg2 = gui->make_child<rv::button>(fg_row, rv::element_size{ rv::styled_size::px(0.f), rv::styled_size::fill() });
	fg2->flex_grow(2.f);
	auto fg3 = gui->make_child<rv::button>(fg_row, rv::element_size{ rv::styled_size::px(0.f), rv::styled_size::fill() });
	fg3->flex_grow(1.f);

	auto mx_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	mx_row->direction(rv::layout_direction::horizontal).gap(8.f);
	auto mx_btn = gui->make_child<rv::button>(mx_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	mx_btn->max_width(rv::styled_size::px(200.f));
	gui->make_child<rv::button>(mx_row, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });

	auto rev_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
	rev_row->direction(rv::layout_direction::horizontal_reverse).gap(8.f);
	gui->make_child<rv::button>(rev_row, rv::element_size{ rv::styled_size::px(60.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(rev_row, rv::element_size{ rv::styled_size::px(120.f), rv::styled_size::fill() });
	gui->make_child<rv::button>(rev_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::fill() });

	auto abs_container = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(80.f) });
	abs_container->direction(rv::layout_direction::horizontal).gap(8.f);
	gui->make_child<rv::button>(abs_container, rv::element_size{ rv::styled_size::fill(), rv::styled_size::fill() });
	auto abs_child = gui->make_child<rv::button>(abs_container, rv::element_size{ rv::styled_size::px(40.f), rv::styled_size::px(40.f) });
	abs_child->positioning(rv::position_type::absolute)
		.inset_right(rv::styled_size::px(8.f))
		.inset_bottom(rv::styled_size::px(8.f));

	auto wrap_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(90.f) });
	wrap_row->direction(rv::layout_direction::horizontal).gap(6.f).row_gap(6.f)
		.wrap(rv::wrap_mode::wrap).align_content(rv::align_content::flex_start);

	for (int i = 0; i < 8; ++i)
	{
		gui->make_child<rv::button>(wrap_row, rv::element_size{ rv::styled_size::px(80.f), rv::styled_size::px(28.f) });
	}

	auto wrap_sb = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(90.f) });
	wrap_sb->direction(rv::layout_direction::horizontal).gap(6.f).row_gap(6.f)
		.wrap(rv::wrap_mode::wrap).align_content(rv::align_content::space_between);

	for (int i = 0; i < 10; ++i)
	{
		gui->make_child<rv::button>(wrap_sb, rv::element_size{ rv::styled_size::px(70.f), rv::styled_size::px(28.f) });
	}
	*/

	// example image loading using stb image
	// cstd::int32_t img_w, img_h, img_c;
	// unsigned char* img_data = stbi_load("images/landing.png", &img_w, &img_h, &img_c, 4);
	// auto logo_tex = img_data ? renderer->create_texture(span_t<const cstd::uint8_t>(img_data, img_w * img_h * 4), img_w, img_h) : nullptr;
	// if (img_data) stbi_image_free(img_data);
	// img_w /= 2;
	// img_h /= 2;

	rv::vector_2d<float> last_screen_size = screen_size;

	const auto font = renderer->add_font("C:\\Windows\\Fonts\\arial.ttf", 32.f);

	auto gui_font = cstd::make_shared<rv::gui_font_impl>(*font);

	// font used by the gui for overlay drawing (tooltips)
	gui->set_font(gui_font);

	static bool fac_enabled = true;
	static float fac_volume = 0.5f;
	static string_t fac_name = "edit me";
	static string_t fac_notes = "Line one\nLine two";
	static int fac_fruit = 0;

	// Buttons. Widget-specific setters (hover/pressed/on_click) chain first while the result is
	// still a button&; base-element setters (background_color/rounding/tooltip) come after.
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
	auto& fps_plot = add_data_visualization_demo(*gui);

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

	MSG msg = { };

	do
	{
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			continue;
		}

		if (use_opengl)
		{
			last_screen_size = screen_size;
			glDisable(GL_SCISSOR_TEST);
			glClearColor(0.1f, 0.1f, 0.1f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_SCISSOR_TEST);
		}
		else
		{
			if (last_screen_size != screen_size)
			{
				context->OMSetRenderTargets(0, nullptr, nullptr);
				rtv.release();

				const HRESULT hr = swap_chain->ResizeBuffers(0,
					static_cast<cstd::uint32_t>(screen_size.x),
					static_cast<cstd::uint32_t>(screen_size.y),
					DXGI_FORMAT_UNKNOWN, 0);

				if (FAILED(hr))
				{
					LOG_ERR("ResizeBuffers failed");
				}

				swap_chain->ResizeBuffers(0, static_cast<cstd::uint32_t>(screen_size.x),
				                          static_cast<cstd::uint32_t>(screen_size.y), DXGI_FORMAT_UNKNOWN, 0);

				create_rtv();

				last_screen_size = screen_size;
			}

			ID3D11RenderTargetView* const tmp_rtv = rtv.value();

			constexpr array_t<float, 4> clear_color = { 0.1f, 0.1f, 0.1f, 1.f };

			context->OMSetRenderTargets(1, &tmp_rtv, nullptr);
			context->ClearRenderTargetView(tmp_rtv, clear_color.data());
		}
		
		renderer->begin_frame(screen_size);
		input->set_cursor(rv::cursor_type::arrow);

		//// red filled rectangle with a basic black dropshadow
		renderer->draw_shadow_rect({ 105.f, 105.f }, { 305.f, 255.f }, { 0.f, 0.f, 0.f, 0.5f }, 17.5f, 20.f, 0.f);
		renderer->draw_rect_filled({ 100.f, 100.f }, { 300.f, 250.f }, { 0.2f, 0.2f, 0.2f, 1.f }, 17.5f);
		renderer->draw_rect({ 100.f, 100.f }, { 300.f, 250.f }, { 0.3f, 0.3f, 0.3f, 1.f }, 1.f, 17.5f);

		// green outlined rectangle with a subtle black glow
		renderer->draw_shadow_rect({ 400.f, 100.f }, { 600.f, 250.f }, { 0.f, 0.f, 0.f, 0.3f }, 8.f, 40.f, 0.f);
		renderer->draw_rect({ 400.f, 100.f }, { 600.f, 250.f }, { 0.f, 1.f, 0.f, 1.f }, 2.f, 8.f);

		// standalone blue shadow rect
		renderer->draw_shadow_rect({ 700.f, 100.f }, { 900.f, 250.f }, { 0.f, 0.5f, 1.f, 0.8f }, 17.5f, 35.f, 3.f, rv::rounding_flags_all, true);
		renderer->push_clip_rect({700.f, 100.f}, {900.f, 250.f}, 17.5f, rv::rounding_flags_all);
		renderer->draw_circle_filled(input->mouse_pos(), 25.f, {1.f, 0.f, 1.f, 1.f});
		renderer->pop_clip_rect();

		// gradient rect
		renderer->draw_rect_filled_multi_color(
			{ 1000.f, 100.f }, { 1200.f, 250.f },
			{ 1.0f, 0.2f, 0.6f, 1.f },
			{ 1.0f, 0.5f, 0.0f, 1.f },
			{ 0.0f, 0.8f, 1.0f, 1.f },
			{ 0.5f, 0.0f, 1.0f, 1.f },
			20.f
		);

		//// red filled rectangle with a really thick shadow
		renderer->draw_shadow_rect({ 100.f, 350.f }, { 300.f, 500.f }, { 0.f, 0.f, 0.f, 0.6f }, 17.5f, 10.f, 20.f);
		renderer->draw_rect_filled({ 100.f, 350.f }, { 300.f, 500.f }, { 1.f, 0.f, 0.f, 1.f }, 17.5f);

		// green filled rectangle with only the top left and bottom right corners rounded
		constexpr rv::rounding_flags selective_flags = static_cast<rv::rounding_flags>(rv::rounding_flags_top_left | rv::rounding_flags_bottom_right);
		renderer->draw_shadow_rect({ 400.f, 350.f }, { 600.f, 500.f }, { 0.f, 0.f, 0.f, 0.6f }, 30.f, 25.f, 0.f, selective_flags);
		renderer->draw_rect_filled({ 400.f, 350.f }, { 600.f, 500.f }, { 0.f, 1.f, 0.f, 1.f }, 30.f, selective_flags);

		const auto mouse_pos = input->mouse_pos();
		if (input->is_mouse_down(0))
		{
			renderer->draw_circle_filled_radial({ mouse_pos.x, mouse_pos.y }, 25.f, {0.f, 0.5f, 1.f, 1.f}, { 0.f, 0.5f, 1.f, 0.f});
		}
		else
		{
			renderer->draw_circle({ mouse_pos.x, mouse_pos.y }, 25.f, { 0.f, 0.5f, 1.f, 1.f }, 1.f);
		}

		//// win32 scroll example
		static float cumulative_scroll = 0.f;
		cumulative_scroll += input->scroll_delta();
	
		if (cumulative_scroll > 10.f) cumulative_scroll = 10.f;
		if (cumulative_scroll < -10.f) cumulative_scroll = -10.f;
		float thumb_y = 380.f - (cumulative_scroll * 18.f);

		renderer->draw_rect_filled({ 20.f, 200.f }, { 40.f, 600.f }, { 0.2f, 0.2f, 0.2f, 0.8f }, 10.f);
		renderer->draw_rect_filled({ 20.f, thumb_y }, { 40.f, thumb_y + 40.f }, { 0.8f, 0.8f, 0.8f, 1.f }, 10.f);

		// test shadow circle (with hollow cutout)
		renderer->draw_shadow_circle({ 900.f, 450.f }, 40.f, { 1.f, 0.f, 0.f, 1.f }, 20.f, true);

		// test shadow line (drawn under a solid line)
		renderer->draw_shadow_line({ 980.f, 410.f }, { 1080.f, 490.f }, { 0.f, 1.f, 0.f, 1.f }, 5.f, 15.f);
		renderer->draw_line({ 980.f, 410.f }, { 1080.f, 490.f }, { 1.f, 1.f, 1.f, 1.f }, 5.f);

		// test shadow poly (drawn under a solid poly)
		renderer->add_path_point({ 1120.f, 490.f });
		renderer->add_path_point({ 1170.f, 410.f });
		renderer->add_path_point({ 1220.f, 490.f });
		renderer->draw_shadow_filled_path({ 0.f, 0.f, 1.f, 1.f }, 25.f);
		
		renderer->add_path_point({ 1120.f, 490.f });
		renderer->add_path_point({ 1170.f, 410.f });
		renderer->add_path_point({ 1220.f, 490.f });
		renderer->draw_filled_path({ 1.f, 1.f, 1.f, 1.f });

		const float fill_progress = std::fmod(renderer->state().time, 2.0f) / 2.0f;
		const float a_min = -cstd::numbers::pi_f / 2.0f;
		const float a_max = a_min + (fill_progress * cstd::numbers::pi_f * 2.0f);
		
		renderer->draw_circle({ 750.f, 450.f }, 50.f, { 1.f, 1.f, 1.f, 1.f }, 1.f);
		
		if (fill_progress > 0.01f)
		{
			// center point
			renderer->add_path_point({ 750.f, 450.f });
			renderer->add_arc_path({ 750.f, 450.f }, 50.f, a_min, a_max, 32);
			renderer->draw_filled_path({ 1.f, 1.f, 1.f, 0.6f });
		}

		// clip rect example
		const rv::position clip_min = { 400.f, 100.f };
		const rv::position clip_max = { 600.f, 250.f };

		const float bounce_x = 500.f + std::sinf(renderer->state().time * 3.f) * 150.f;

		// push the clip rect, draw the circle, and pop it
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

		// if (logo_tex)
		// {
		// 	renderer->draw_image_rounded(logo_tex, { 50.f, 50.f }, {  (float)img_w, (float)img_h }, 30.f);
		// 	renderer->draw_rect({ 50.f, 50.f }, { (float)img_w, (float)img_h }, { 1.f, 1.f, 1.f, 0.5f }, 2.f, 30.f);
		// }

		if (font)
		{
			const auto state = renderer->state();

			const string_t text = std::format("width {}px height {}px time {:.2f}s delta {:.4f}s fps {:.2f}", state.display_size.x, state.display_size.y, state.time, state.delta_time, 1.f / state.delta_time);

			constexpr float size = 35.f;
			constexpr rv::position text_pos = { 100.f, 580.f};
			const rv::position text_size = renderer->calc_text_size(*font, text, size);

			//renderer->draw_rect_filled(text_pos, {text_pos.x + text_size.x, text_pos.y + text_size.y}, {1.f, 0.25f, 0.f, 1.f});
			renderer->add_text_shadow(*font, text_pos, text, {1.f, 0.4f, 1.f , 1.f}, 15.f, size);
			renderer->draw_text(*font, text_pos, text, { 0.4f, 1.f, 1.f, 1.f }, size);
		}

		fps_plot.push_value(renderer->state().delta_time * 1000.f);

		gui->render(screen_size);

		renderer->end_frame();

		if (use_opengl)
		{
			SwapBuffers(hdc);
		}
		else
		{
			swap_chain->Present(0, 0);
		}

		input->reset();
	} while (msg.message != WM_QUIT);

	// cleanup
	if (use_opengl)
	{
		renderer.reset();
		if (hglrc)
		{
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(hglrc);
		}
		if (hdc)
		{
			ReleaseDC(hwnd, hdc);
		}
	}

	return 0;
}
