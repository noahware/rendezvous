#include "gui/element.hpp"
#include "gui/gui.hpp"
#include "gui/comp/button.hpp"
#include "log/log.hpp"
#include "render/impl/dx11.hpp"
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

	input->handle_message(hwnd, msg, wparam, lparam);

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

cstd::int32_t main()
{
	LOG_INFO("rendezvous");

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

	DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };

	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = hwnd;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	rv::dx11_object<IDXGISwapChain> swap_chain;
	rv::dx11_object<ID3D11Device> device;
	rv::dx11_object<ID3D11DeviceContext> context;
	rv::dx11_object<ID3D11RenderTargetView> rtv;

	const HRESULT status = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
	                                                     D3D11_SDK_VERSION, &swap_chain_desc, swap_chain.release_and_get(), device.release_and_get(),
	                                                     nullptr, context.release_and_get());

	if (status != S_OK)
	{
		LOG_ERR("unable to create device and swap chain");

		return 1;
	}

	const auto create_rtv = [&]()
		{
			rv::dx11_object<ID3D11Texture2D> back_buffer;

			swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.release_and_get()));
			device->CreateRenderTargetView(back_buffer.value(), nullptr, rtv.release_and_get());

			back_buffer.release();
		};


	create_rtv();

	const auto renderer = cstd::make_shared<rv::dx11_renderer>(device.value(), context.value());

	if (!renderer->init())
	{
		LOG_ERR("unable to init renderer");

		return 1;
	}

	input = cstd::make_shared<rv::win32_input>();

	auto gui_renderer = cstd::make_unique<rv::gui_renderer_impl>(renderer);
	auto gui = cstd::make_unique<rv::gui>(cstd::move(gui_renderer), input);

	gui->default_style().gap = 8.f;
	gui->default_style().direction = rv::layout_direction::vertical;

	auto root = gui->root();
	root->direction(rv::layout_direction::vertical);

	auto vis_row = gui->make_child<rv::element>(root, rv::element_size{ rv::styled_size::fill(), rv::styled_size::px(40.f) });
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


	// example image loading using stb image
	// cstd::int32_t img_w, img_h, img_c;
	// unsigned char* img_data = stbi_load("images/landing.png", &img_w, &img_h, &img_c, 4);
	// auto logo_tex = img_data ? renderer->create_texture(span_t<const cstd::uint8_t>(img_data, img_w * img_h * 4), img_w, img_h) : nullptr;
	// if (img_data) stbi_image_free(img_data);
	// img_w /= 2;
	// img_h /= 2;

	rv::vector_2d<float> last_screen_size = screen_size;

	const auto font = renderer->add_font("C:\\Windows\\Fonts\\arial.ttf", 32.f);

	MSG msg = { };

	do
	{
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			continue;
		}

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
		
		renderer->begin_frame(screen_size);

		gui->render(screen_size);

		// red filled rectangle with a basic black dropshadow
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

		// red filled rectangle with a really thick shadow
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

		// win32 scroll example
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

			const string_t text = std::format("width {}px height {}px time {:.2f}s delta {:.4f}s", state.display_size.x, state.display_size.y, state.time, state.delta_time);

			constexpr float size = 35.f;
			constexpr rv::position text_pos = { 100.f, 580.f};
			const rv::position text_size = renderer->calc_text_size(*font, text, size);

			//renderer->draw_rect_filled(text_pos, {text_pos.x + text_size.x, text_pos.y + text_size.y}, {1.f, 0.25f, 0.f, 1.f});
			renderer->add_text_shadow(*font, text_pos, text, {1.f, 0.4f, 1.f , 1.f}, 15.f, size);
			renderer->draw_text(*font, text_pos, text, { 0.4f, 1.f, 1.f, 1.f }, size);
		}
		renderer->end_frame();

		swap_chain->Present(1, 0);

		input->reset();
	} while (msg.message != WM_QUIT);

	return 0;
}
