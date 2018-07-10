#pragma once
#include "gui.h"

#ifdef _DEBUG
#include <Windows.h>
#endif

namespace ui {
	namespace detail {
		template<typename ...REST>
		struct window_get {
			template<typename T>
			static T apply(T&&) {
				static_assert(false, "no matching element found for that name");
			}
		};
		template<typename i, typename INDEX, typename TYPE, typename ...REST>
		struct window_get<i, INDEX, TYPE, REST...> {
			static auto& apply(gui_window<INDEX, TYPE, REST ...>& w) {
				return window_get<i, REST...>::apply(w);
			}
			static auto& apply(const gui_window<INDEX, TYPE, REST ...>& w) {
				return window_get<i, REST...>::apply(w);
			}
		};
		template<typename INDEX, typename TYPE, typename ...REST>
		struct window_get<INDEX, INDEX, TYPE, REST...> {
			static TYPE& apply(gui_window<INDEX, TYPE, REST ...>& w) {
				return w.m_object;
			}
			static const TYPE& apply(const gui_window<INDEX, TYPE, REST ...>& w) {
				return w.m_object;
			}
		};
	}
}

template<typename INDEX, typename TYPE, typename ... REST>
class ui::gui_window<INDEX, TYPE, REST ...> : public ui::gui_window<REST ...> {
protected:
	TYPE m_object;
	bool create_named_member(world_state& ws, tagged_gui_object win, ui::element_tag t, const char* ns, const char* ne);
	ui::tagged_gui_object create_window(world_state& ws, const ui::window_def& def);
	template<typename window_type>
	void member_init_in_window(window_type& w, world_state& m);
public:
	template<typename window_type>
	void member_update_in_window(window_type& w, world_state& s);

	gui_window(ui::gui_window<INDEX, TYPE, REST ...>&&) = default;
	gui_window(ui::gui_window<INDEX, TYPE, REST ...>& b) noexcept : gui_window<INDEX, TYPE, REST ...>(std::move(b)) {}
	template<typename ...PARAMS>
	gui_window(PARAMS&& ... params) : gui_window<REST ...>(std::forward<PARAMS>(params) ...), m_object(std::forward<PARAMS>(params) ...) {}

	template<typename i>
	auto& get();
	template<typename i>
	auto& get() const;

	ui::tagged_gui_object create(world_state& manager, const ui::window_def& def);
	virtual void update_data(gui_object_tag, world_state&) override;

	friend struct ui::detail::window_get<INDEX, INDEX, TYPE, REST ...>;
};

template<typename BASE_BEHAVIOR>
class ui::gui_window<BASE_BEHAVIOR> : public BASE_BEHAVIOR {
protected:
	bool create_named_member(world_state& ws, tagged_gui_object win, ui::element_tag t, const char* ns, const char* ne);
	ui::tagged_gui_object create_window(world_state& ws, const ui::window_def& def);
	template<typename window_type>
	void member_init_in_window(window_type& w, world_state& m) {
		if constexpr(ui::detail::has_on_create<BASE_BEHAVIOR, world_state&>) {
			BASE_BEHAVIOR::on_create(m);
		}
	}
	template<typename window_type>
	void member_update_in_window(window_type& w, world_state& s) {}
public:
	gui_window(ui::gui_window<BASE_BEHAVIOR>&&) = default;
	gui_window(ui::gui_window<BASE_BEHAVIOR>& b) noexcept : gui_window(std::move(b)) {}
	template<typename ...PARAMS>
	gui_window(PARAMS&& ... params) : BASE_BEHAVIOR(std::forward<PARAMS>(params) ...) {}

	ui::gui_object_tag window_object;
	ui::tagged_gui_object create(world_state& manager, const ui::window_def& def);
};

template<typename INDEX, typename TYPE, typename ...REST>
template<typename i>
auto& ui::gui_window<INDEX, TYPE, REST...>::get() { return detail::window_get<i, INDEX, TYPE, REST...>::apply(*this); }

template<typename INDEX, typename TYPE, typename ...REST>
template<typename i>
auto& ui::gui_window<INDEX, TYPE, REST...>::get() const { return detail::window_get<i, INDEX, TYPE, REST...>::apply(*this); }

template<typename INDEX, typename TYPE, typename ...REST>
ui::tagged_gui_object ui::gui_window<INDEX, TYPE, REST...>::create_window(world_state& ws, const ui::window_def & def) {
	return gui_window<REST...>::create_window(ws, def);
}

template<typename INDEX, typename TYPE, typename ...REST>
void ui::gui_window<INDEX, TYPE, REST...>::update_data(gui_object_tag, world_state& w) {
	member_update_in_window(*this, w);
}

template<typename INDEX, typename TYPE, typename ...REST>
template<typename window_type>
void ui::gui_window<INDEX, TYPE, REST...>::member_update_in_window(window_type& w, world_state& s) {
	if constexpr(ui::detail::has_windowed_update<TYPE, window_type&, world_state&>)
		m_object.windowed_update(w, s);
	gui_window<REST...>::member_update_in_window(w, s);
}

template<typename INDEX, typename TYPE, typename ...REST>
template<typename window_type>
void ui::gui_window<INDEX, TYPE, REST...>::member_init_in_window(window_type& w, world_state& m) {
	if constexpr(ui::detail::has_initialize_in_window<TYPE, window_type&, world_state&>)
		m_object.initialize_in_window(w, m);
	gui_window<REST...>::member_init_in_window(w, m);
}

template<typename INDEX, typename TYPE, typename ...REST>
bool ui::gui_window<INDEX, TYPE, REST...>::create_named_member(world_state& ws, tagged_gui_object win, ui::element_tag t, const char* ns, const char* ne) {
	if (compile_time_str_compare_ci<INDEX>(ns, ne) == 0) {
		std::visit([_this = this, &win, &ws](auto tag) {
			if constexpr(ui::detail::can_create<decltype(tag), TYPE>)
				ui::create_static_element(ws, tag, win, _this->m_object);
			else {
#ifdef _DEBUG
				OutputDebugStringA("Unable to instantiate window element: bad tag type\n");
#endif
			}
		}, t);
		return true;
	} else {
		return gui_window<REST...>::create_named_member(ws, win, t, ns, ne);
	}
}

template<typename INDEX, typename TYPE, typename ...REST>
ui::tagged_gui_object ui::gui_window<INDEX, TYPE, REST...>::create(world_state& manager, const ui::window_def & definition) {
	const auto win = create_window(manager, definition);
	for (auto i = definition.sub_object_definitions.crbegin(); i != definition.sub_object_definitions.crend(); ++i) {
		auto rn = manager.s.gui_m.nmaps.get_raw_name(*i);
		const char* rn_s = rn.get_str(manager.s.gui_m.ui_definitions.name_data);
		const char* rn_e = rn_s + rn.length();

		if (!create_named_member(manager, win, *i, rn_s, rn_e)) {
			std::visit([&manager, &win](auto tag) {
				if constexpr(!std::is_same_v<decltype(tag), std::monostate>)
					ui::create_dynamic_element(manager, tag, win);
			}, *i);
		}
	}
	member_init_in_window(*this, manager);

	return win;
}

template<typename BASE_BEHAVIOR>
bool ui::gui_window<BASE_BEHAVIOR>::create_named_member(world_state& ws, tagged_gui_object win, ui::element_tag t, const char* ns, const char* ne) {
	return false;
}

template<typename BASE_BEHAVIOR>
ui::tagged_gui_object ui::gui_window<BASE_BEHAVIOR>::create_window(world_state& ws, const ui::window_def & definition) {
	const auto window = ws.w.gui_m.gui_objects.emplace();

	window.object.align = alignment_from_definition(definition);

	if (is_valid_index(definition.background_handle)) {
		const auto& bgdefinition = ws.s.gui_m.ui_definitions.buttons[definition.background_handle];
		ui::detail::instantiate_graphical_object(ws.s.gui_m, ws.w.gui_m, window, bgdefinition.graphical_object_handle);
	} else {
		window.object.type_dependant_handle.store(0, std::memory_order_release);
	}

	window.object.associated_behavior = this;
	BASE_BEHAVIOR::associated_object = &window.object;

	window.object.size = definition.size;
	window.object.position = definition.position;

	return window;
}

template<typename BASE_BEHAVIOR>
ui::tagged_gui_object ui::gui_window<BASE_BEHAVIOR>::create(world_state& ws, const ui::window_def& definition) {
	const auto win = create_window(ws, definition);
	for (auto i = definition.sub_object_definitions.crbegin(); i != definition.sub_object_definitions.crend(); ++i) {
		std::visit([&ws, &win](auto tag) {
			ui::create_dynamic_element(ws, tag, win);
		}, *i);
	}

	if constexpr(ui::detail::has_on_create<BASE_BEHAVIOR, world_state&>) {
		BASE_BEHAVIOR::on_create(ws);
	}
	return win;
}

template<typename ... REST>
ui::tagged_gui_object ui::create_static_element(world_state& ws, window_tag handle, tagged_gui_object parent, gui_window<REST...>& b) {
	const auto& window_definition = ws.s.gui_m.ui_definitions.windows[handle];
	const auto res = b.create(ws, window_definition);
	b.window_object = res.id;
	ui::add_to_back(ws.w.gui_m, parent, res);
	ws.w.gui_m.flag_minimal_update();
	return res;
}

#include "gui.hpp"
