#include "tabs.hpp"

namespace rv
{
	void tabs::apply_orientation() noexcept
	{
		const bool left = side_ == tab_side::left;

		// root stacks the bar and the content along the bar's cross axis
		style_.direction = left ? layout_direction::horizontal : layout_direction::vertical;

		if (tab_bar_)
		{
			tab_bar_->set_declared_size(left
				? element_size{ styled_size::px(bar_width_), styled_size::fill() }
				: element_size{ styled_size::fill(), styled_size::auto_v() });

			tab_bar_->direction(left ? layout_direction::vertical : layout_direction::horizontal)
			         .gap(left ? 2.f : tab_gap);

			// sidebar surface in left mode; an invisible strip in top mode
			tab_bar_->background_color(left ? bar_color_ : color{ 0.f, 0.f, 0.f, 0.f });
		}

		if (content_host_)
		{
			content_host_->set_declared_size(element_size{ styled_size::fill(), styled_size::fill() });
			content_host_->direction(layout_direction::vertical);
		}

		for (const auto& btn : buttons_)
		{
			style_tab_button(*btn);
		}

		// re-apply per-state styling (accent/colours) for the new orientation on the next update
		applied_active_ = -2;

		mark_layout_dirty();
	}

	// Size + label alignment per orientation. Left = full-width nav rows with left-aligned,
	// indented labels and square corners; top = shrink-wrapped, centred, rounded tabs.
	void tabs::style_tab_button(button& b) const noexcept
	{
		if (side_ == tab_side::left)
		{
			b.set_declared_size(element_size{ styled_size::fill(), styled_size::px(nav_item_height) });
			b.text_alignment(text_align::left).label_indent(nav_label_indent).rounding(0.f);
		}
		else
		{
			b.set_declared_size(element_size{ styled_size::auto_v(), styled_size::px(tab_button_height) });
			b.text_alignment(text_align::center).label_indent(0.f).rounding(6.f);
		}
	}

	element& tabs::add_tab(const string_view_t label)
	{
		const cstd::int32_t index = static_cast<cstd::int32_t>(panels_.size());

		// header button (add_child, not make_child) so gui_ + layout-dirty ptr propagate
		auto btn = make_element<button>(element_size{ }, font_);
		btn->text(label);
		btn->text_size(button_text_size_);
		btn->on_click([this, index]()
		{
			commit(index);
		});
		style_tab_button(*btn);

		tab_bar_->add_child(btn);
		buttons_.push_back(btn);

		// content panel, returned to the caller to populate; add_child propagates gui_ so the
		// caller's panel.add_*(...) factory calls resolve the gui and register in the tree.
		auto panel = make_element<element>(element_size{ styled_size::fill(), styled_size::fill() });
		panel->direction(layout_direction::vertical).gap(8.f);
		content_host_->add_child(panel);
		panels_.push_back(panel);

		// first tab added becomes active by default
		if (active_ < 0)
		{
			active_ = index;
		}

		// seed visibility so the very first laid-out frame is already correct for the common case
		// (no programmatic active() override); update() reconciles any later change.
		panel->set_visible(active_ == index);

		mark_layout_dirty();

		return *panels_.back();
	}

	void tabs::commit(const cstd::int32_t index)
	{
		active_ = clamp_index(index);

		if (on_change_ && active_ >= 0)
		{
			on_change_(active_);
		}
	}

	void tabs::update(const float dt)
	{
		// reconcile panel visibility + header styling only when the active tab changes, matching
		// the combo_box / color_picker pattern (toggle + relayout on change, not every frame).
		if (active_ != applied_active_)
		{
			const bool left = side_ == tab_side::left;

			for (cstd::int32_t i = 0; i < static_cast<cstd::int32_t>(panels_.size()); ++i)
			{
				const bool on = i == active_;

				// visible(bool) marks layout dirty so the host re-flows: the shown panel claims
				// the freed space, the hidden ones reserve none.
				panels_[i]->visible(on);

				// resting colours; button::update saves/restores style_.background_color around
				// its hover/press override, so writing the resting value here survives.
				buttons_[i]->background_color(on ? active_tab_color_ : inactive_tab_color_);
				buttons_[i]->text_color(on ? active_text_color_ : inactive_text_color_);

				// red accent stripe marks the active item, sidebar (left) mode only
				buttons_[i]->accent(left && on ? accent_bar_color_ : color{ 0.f, 0.f, 0.f, 0.f },
				                    left && on ? accent_bar_thickness : 0.f);
			}

			applied_active_ = active_;
		}

		if (hovered_ && input_)
		{
			input_->set_cursor(cursor_type::hand);
		}

		element::update(dt);
	}
}
