#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "button.hpp"

namespace rv
{
	enum class tab_side : cstd::uint8_t
	{
		top,
		left
	};

	// Tab container: a strip of header buttons plus a content host that shows only the active
	// tab's panel. Switching relies on toggling sibling panels' visibility: the layout engine
	// skips invisible children entirely (no reserved space), so the visible panel fills the host.
	// Built from real `button` widgets so hover/press/click behaviour is reused, not re-drawn.
	class tabs final : public element
	{
	public:
		tabs(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			gap(tab_content_gap);

			tab_bar_ = make_child<element>(element_size{ styled_size::fill(), styled_size::auto_v() });
			content_host_ = make_child<element>(element_size{ styled_size::fill(), styled_size::fill() });

			// subtle surface so a freshly-created tab group reads as an intentional panel; all of
			// this is overridable by the caller via the returned content panel / the tabs element.
			content_host_->background_color({ 0.13f, 0.13f, 0.16f, 1.f }).rounding(8.f).padding(12.f).gap(8.f);

			apply_orientation();
		}

		// Adds a tab with the given header label and returns its (empty) content panel, so the
		// caller can populate it directly: tabs.add_tab("Settings").add_checkbox("Enable").
		// Must be called while the tabs widget is attached to the tree (the add_tabs() factory
		// guarantees this) so the new panel inherits the gui back-pointer. Defined in tabs.cpp.
		element& add_tab(string_view_t label);

		// Tab strip on top (horizontal) or on the left (vertical). Re-flows immediately.
		tabs& bar_side(const tab_side side) noexcept
		{
			if (side_ != side)
			{
				side_ = side;
				apply_orientation();
			}

			return *this;
		}

		// Programmatic selection, does NOT fire on_change (mirrors combo_box::selected).
		tabs& active(const cstd::int32_t index) noexcept
		{
			active_ = clamp_index(index);

			return *this;
		}

		[[nodiscard]] cstd::int32_t active() const noexcept
		{
			return active_;
		}

		[[nodiscard]] cstd::int32_t count() const noexcept
		{
			return static_cast<cstd::int32_t>(panels_.size());
		}

		// Fired when the user clicks a tab (mirrors combo_box::on_change / commit).
		tabs& on_change(function_t<void(cstd::int32_t)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		// Set by the add_tabs() factory; applied to each header button's label.
		tabs& tab_text_size(const float px) noexcept
		{
			button_text_size_ = px;

			return *this;
		}

		tabs& active_tab_color(const color c) noexcept    { active_tab_color_ = c;    return *this; }
		tabs& inactive_tab_color(const color c) noexcept  { inactive_tab_color_ = c;  return *this; }
		tabs& active_text_color(const color c) noexcept   { active_text_color_ = c;   return *this; }
		tabs& inactive_text_color(const color c) noexcept { inactive_text_color_ = c; return *this; }

		// sidebar styling (left mode)
		tabs& bar_width(const float px) noexcept   { bar_width_ = px; if (side_ == tab_side::left) apply_orientation(); return *this; }
		tabs& bar_color(const color c) noexcept    { bar_color_ = c; if (tab_bar_ && side_ == tab_side::left) tab_bar_->background_color(c); return *this; }
		tabs& accent_color(const color c) noexcept { accent_bar_color_ = c; applied_active_ = -2; return *this; }

		void update(float dt) override; // defined in tabs.cpp

	private:
		void apply_orientation() noexcept;               // defined in tabs.cpp
		void style_tab_button(button& b) const noexcept; // defined in tabs.cpp
		void commit(cstd::int32_t index);                // defined in tabs.cpp

		[[nodiscard]] cstd::int32_t clamp_index(const cstd::int32_t index) const noexcept
		{
			if (panels_.empty())
			{
				return -1;
			}

			if (index < 0)
			{
				return 0;
			}

			const cstd::int32_t last = static_cast<cstd::int32_t>(panels_.size()) - 1;

			return index > last ? last : index;
		}

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;

		shared_ptr_t<element> tab_bar_;      // row (top) / column (left) of header buttons
		shared_ptr_t<element> content_host_; // holds one content panel per tab

		// parallel arrays indexed by tab: buttons_[i] is the header for panels_[i]
		vector_t<shared_ptr_t<button>> buttons_;
		vector_t<shared_ptr_t<element>> panels_;

		cstd::int32_t active_ = -1;         // selected tab, -1 when empty
		cstd::int32_t applied_active_ = -2; // last index whose visibility/styling was reconciled (see update)
		tab_side side_ = tab_side::top;
		float button_text_size_ = 16.f;

		function_t<void(cstd::int32_t)> on_change_;

		color active_tab_color_ = { 0.18f, 0.18f, 0.22f, 1.f };
		color inactive_tab_color_ = { 0.11f, 0.11f, 0.14f, 1.f };
		color active_text_color_ = { 1.f, 1.f, 1.f, 1.f };
		color inactive_text_color_ = { 0.62f, 0.62f, 0.68f, 1.f };

		color bar_color_ = { 0.08f, 0.08f, 0.10f, 1.f };        // sidebar background (left mode)
		color accent_bar_color_ = { 0.90f, 0.20f, 0.20f, 1.f }; // active item accent stripe (left mode)
		float bar_width_ = 160.f;                               // sidebar width (left mode)

		static constexpr float tab_button_height = 34.f;   // top-mode button height
		static constexpr float nav_item_height = 40.f;     // left-mode (sidebar) row height
		static constexpr float nav_label_indent = 16.f;    // left text inset for nav items
		static constexpr float accent_bar_thickness = 3.f; // active accent stripe width
		static constexpr float tab_gap = 4.f;          // between header buttons
		static constexpr float tab_content_gap = 6.f;  // between the bar and the content host
	};
}
