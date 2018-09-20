#include "common\\common.h"
#include "trigger_gui.h"
#include "gui\\gui.hpp"
#include "effects.h"
#include "codes.h"
#include "world_state\\world_state.h"
#include <random>
#include "population\\population_function.h"
#include "nations\\nations_functions.hpp"
#include "modifiers\\modifiers_gui.h"
#include "technologies\\technologies_functions.h"

namespace triggers {
	namespace {
		inline uint32_t cmp_code_to_fixed_ui(uint16_t code) {
			switch(code & trigger_codes::association_mask) {
				case trigger_codes::association_eq: return scenario::fixed_ui::cmp_eq;
				case trigger_codes::association_le: return scenario::fixed_ui::cmp_le;
				case trigger_codes::association_lt: return scenario::fixed_ui::cmp_lt;
				case trigger_codes::association_ge: return scenario::fixed_ui::cmp_ge;
				case trigger_codes::association_gt: return scenario::fixed_ui::cmp_gt;
				case trigger_codes::association_ne: return scenario::fixed_ui::cmp_ne;
			}
			return scenario::fixed_ui::cmp_eq;
		}

		inline uint32_t cmp_has_code_to_fixed_ui(uint16_t code) {
			switch(code & trigger_codes::association_mask) {
				case trigger_codes::association_eq: return scenario::fixed_ui::cmp_has_eq;
				case trigger_codes::association_le: return scenario::fixed_ui::cmp_has_eq;
				case trigger_codes::association_lt: return scenario::fixed_ui::cmp_has_ne;
				case trigger_codes::association_ge: return scenario::fixed_ui::cmp_has_eq;
				case trigger_codes::association_gt: return scenario::fixed_ui::cmp_has_ne;
				case trigger_codes::association_ne: return scenario::fixed_ui::cmp_has_ne;
			}
			return scenario::fixed_ui::cmp_has_eq;
		}

		inline uint32_t any_all_code_to_fixed_ui(uint16_t code) {
			if(code & trigger_codes::is_disjunctive_scope)
				return scenario::fixed_ui::any_of_the_following;
			else
				return scenario::fixed_ui::all_of_the_following;
		}

		inline uint32_t every_any_code_to_fixed_ui(uint16_t code) {
			if(code & trigger_codes::is_existance_scope)
				return scenario::fixed_ui::any;
			else
				return scenario::fixed_ui::every;
		}

		template<typename T>
		ui::xy_pair display_with_comparison_no_newline(
			uint16_t trigger_code,
			uint32_t left_label,
			T value,
			display_type d_type,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[left_label], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[cmp_code_to_fixed_ui(trigger_code)], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);

			char16_t local_buffer[32];
			put_pos_value_in_buffer(local_buffer, d_type, std::abs(value));
			cursor_in = ui::text_chunk_to_instances(ws.s.gui_m, ws.w.gui_m, vector_backed_string<char16_t>(local_buffer), container, cursor_in, fmt, lm);

			return cursor_in;
		}

		template<typename T>
		ui::xy_pair display_with_comparison(
			uint16_t trigger_code,
			uint32_t left_label,
			T value,
			display_type d_type,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = display_with_comparison_no_newline(trigger_code, left_label, value, d_type, ws, container, cursor_in, lm, fmt);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();
			return cursor_in;
		}

		inline ui::xy_pair display_with_comparison_no_newline(
			uint16_t trigger_code,
			uint32_t left_label,
			text_data::text_tag value,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[left_label], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[cmp_code_to_fixed_ui(trigger_code)], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, value, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			return cursor_in;
		}

		inline ui::xy_pair display_with_comparison(
			uint16_t trigger_code,
			uint32_t left_label,
			text_data::text_tag value,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = display_with_comparison_no_newline(trigger_code, left_label, value, ws, container, cursor_in, lm, fmt);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();
			return cursor_in;
		}

		inline ui::xy_pair display_with_comparison(
			uint16_t trigger_code,
			text_data::text_tag value,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[cmp_code_to_fixed_ui(trigger_code)], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, value, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();
			return cursor_in;
		}

		inline ui::xy_pair display_with_has_comparison_no_newline(
			uint16_t trigger_code,
			text_data::text_tag value,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[cmp_has_code_to_fixed_ui(trigger_code)], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, value, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			return cursor_in;
		}

		inline ui::xy_pair display_with_has_comparison(
			uint16_t trigger_code,
			text_data::text_tag value,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt) {

			cursor_in = display_with_has_comparison_no_newline(trigger_code, value, ws, container, cursor_in, lm, fmt);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();
			return cursor_in;
		}

		inline uint32_t count_subtriggers(uint16_t const* source) {
			uint32_t count = 0;
			const auto source_size = 1 + get_trigger_payload_size(source);
			auto sub_units_start = source + 2 + trigger_scope_data_payload(source[0]);
			while(sub_units_start < source + source_size) {
				++count;
				sub_units_start += 1 + get_trigger_payload_size(sub_units_start);
			}
			return count;
		}

		inline ui::xy_pair display_subtriggers(
			uint16_t const* source,
			world_state& ws,
			ui::tagged_gui_object container,
			ui::xy_pair cursor_in,
			ui::unlimited_line_manager& lm,
			ui::text_format const& fmt,
			void* primary_slot,
			void* this_slot,
			void* from_slot,
			population::rebel_faction* rebel_slot) {

			const auto source_size = 1 + get_trigger_payload_size(source);
			auto sub_units_start = source + 2 + trigger_scope_data_payload(source[0]);
			while(sub_units_start < source + source_size) {
				cursor_in = make_trigger_description( ws, container, cursor_in, lm, fmt,
					sub_units_start, primary_slot, this_slot, from_slot, rebel_slot );
				sub_units_start += 1 + get_trigger_payload_size(sub_units_start);
			}
			return cursor_in;
		}
#define TRIGGER_DISPLAY_PARAMS uint16_t const* tval, world_state& ws, ui::tagged_gui_object container, \
			ui::xy_pair cursor_in, ui::unlimited_line_manager& lm, ui::text_format const& fmt, void* primary_slot, \
			void* this_slot, void* from_slot, population::rebel_faction* rebel_slot


		ui::xy_pair tf_none(TRIGGER_DISPLAY_PARAMS) {
			return cursor_in;
		}

		ui::xy_pair tf_generic_scope(TRIGGER_DISPLAY_PARAMS) {
			if(count_subtriggers(tval) > 1) {
				//cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}
			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, primary_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_neighbor_province_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::neighboring_province], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_neighbor_country_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::neighboring_nation], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_neighbor_country_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::neighboring_nation], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_war_countries_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::nation_at_war_against], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_war_countries_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::nation_at_war_against], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_greater_power_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::great_power], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_owned_province_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owned_province], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::state_instance*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_state],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_owned_province_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owned_province], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_core_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::core_in], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_core_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::core_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_state_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::state_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_substate_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::substate_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_sphere_member_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::nation_in_sphere], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_pop_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::pop], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_pop_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::pop], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_pop_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::pop], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_x_provinces_in_variable_region(TRIGGER_DISPLAY_PARAMS) {
			auto region = trigger_payload(*(tval + 2)).state;

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[every_any_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::province_in], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				ws.s.province_m.state_names[region],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_owner_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owner_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::state_instance*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_state],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((nations::state_instance*)primary_slot)->owner : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_owner_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owner_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->owner : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_controller_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::controller_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->controller : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_location_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::location_of_pop], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto loc = bool(primary_slot) ? ((population::pop*)primary_slot)->location : provinces::province_tag();
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				is_valid_index(loc) ? &(ws.w.province_s.province_state_container[loc]) : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_country_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owner_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::state_instance*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_state],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, 
				bool(primary_slot) ? ((nations::state_instance*)primary_slot)->owner : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_country_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::nation_of_pop], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, 
				bool(primary_slot) ? population::get_pop_owner(ws, *((population::pop*)primary_slot)) : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_capital_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::capital_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto loc = bool(primary_slot) ? ((nations::nation*)primary_slot)->current_capital : provinces::province_tag();
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, 
				is_valid_index(loc) ? &(ws.w.province_s.province_state_container[loc]) : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_this_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(this_slot) ? ((nations::nation*)this_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::this_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				this_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_this_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(this_slot) ? ((nations::state_instance*)this_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::this_state],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				this_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_this_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(this_slot) ? ((provinces::province_state*)this_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::this_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				this_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_this_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				ws.s.fixed_ui_text[scenario::fixed_ui::this_pop],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				this_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_from_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(from_slot) ? ((nations::nation*)from_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::from_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				from_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_from_scope_state(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(from_slot) ? ((nations::state_instance*)from_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::from_state],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				from_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_from_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				bool(from_slot) ? ((provinces::province_state*)from_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::from_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				from_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_from_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				ws.s.fixed_ui_text[scenario::fixed_ui::from_pop],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				from_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_sea_zone_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in,
				ws.s.fixed_ui_text[scenario::fixed_ui::adjacent_sea],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto sea_zones = ws.s.province_m.coastal_adjacency.get_row(((provinces::province_state*)primary_slot)->id);
			if(sea_zones.first != sea_zones.second) {
				cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
					&(ws.w.province_s.province_state_container[*sea_zones.first]), this_slot, from_slot, rebel_slot);
			}
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_cultural_union_scope(TRIGGER_DISPLAY_PARAMS) {
			auto prim_culture = primary_slot ? ((nations::nation*)primary_slot)->primary_culture : cultures::culture_tag();
			auto cg = is_valid_index(prim_culture) ? ws.s.culture_m.culture_container[prim_culture].group : cultures::culture_group_tag();

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::cultural_union_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				is_valid_index(cg) ? ws.s.culture_m.culture_groups[cg].name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			if(is_valid_index(cg)) {
				auto union_tag = ws.s.culture_m.culture_groups[cg].union_tag;
				if(is_valid_index(union_tag)) {
					auto union_holder = ws.w.culture_s.national_tags_state[union_tag].holder;
					cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
						union_holder, this_slot, from_slot, rebel_slot);
				}
			}
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_overlord_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::overlord_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->overlord : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_sphere_owner_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::sphere_leader_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((nations::nation*)primary_slot)->sphere_leader : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_independence_scope(TRIGGER_DISPLAY_PARAMS) {
			auto rtag = bool(rebel_slot) ? rebel_slot->independence_tag : cultures::national_tag();
			auto ination = is_valid_index(rtag) ? ws.w.culture_s.national_tags_state[rtag].holder : nullptr;

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::reb_independence_nation], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				ination, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_flashpoint_tag_scope(TRIGGER_DISPLAY_PARAMS) {
			auto ctag = bool(primary_slot) ? ((nations::state_instance*)primary_slot)->crisis_tag : cultures::national_tag();
			auto ination = is_valid_index(ctag) ? ws.w.culture_s.national_tags_state[ctag].holder : nullptr;

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::flashpoint_nation], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				ination, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);

			return cursor_in;
		}
		ui::xy_pair tf_crisis_state_scope(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::crisis_state], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto cstate = ws.w.crisis_state;
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				is_valid_index(cstate) ? &(ws.w.nation_s.states[cstate]) : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);

			return cursor_in;
		}
		ui::xy_pair tf_state_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::containing_state], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->state_instance : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_state_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::containing_state], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				ws.s.fixed_ui_text[scenario::fixed_ui::singular_pop],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto pop_province = bool(primary_slot) ? ((population::pop*)primary_slot)->location : provinces::province_tag();
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				is_valid_index(pop_province) ? ws.w.province_s.province_state_container[pop_province].state_instance : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_tag_scope(TRIGGER_DISPLAY_PARAMS) {
			auto tag = trigger_payload(tval[2]).tag;
			auto tag_holder = ws.w.culture_s.national_tags_state[tag].holder;

			cursor_in = ui::add_linear_text(cursor_in, tag_holder ? tag_holder->name : ws.s.culture_m.national_tags[tag].default_name.name, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto pop_province = bool(primary_slot) ? ((population::pop*)primary_slot)->location : provinces::province_tag();
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				tag_holder, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_integer_scope(TRIGGER_DISPLAY_PARAMS) {
			provinces::province_tag ptag(tval[2]);

			cursor_in = ui::add_linear_text(cursor_in, ws.w.province_s.province_state_container[ptag].name, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			auto pop_province = bool(primary_slot) ? ((population::pop*)primary_slot)->location : provinces::province_tag();
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				&(ws.w.province_s.province_state_container[ptag]), this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_country_scope_nation(TRIGGER_DISPLAY_PARAMS) {
			if(count_subtriggers(tval) > 1) {
				//cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}
			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt, primary_slot, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_country_scope_province(TRIGGER_DISPLAY_PARAMS) {
			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::owner_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_province],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
				bool(primary_slot) ? ((provinces::province_state*)primary_slot)->owner : nullptr, this_slot, from_slot, rebel_slot);
			lm.decrease_indent(1);
			return cursor_in;
		}
		ui::xy_pair tf_cultural_union_scope_pop(TRIGGER_DISPLAY_PARAMS) {
			auto prim_culture = primary_slot ? ((population::pop*)primary_slot)->culture : cultures::culture_tag();
			auto cg = is_valid_index(prim_culture) ? ws.s.culture_m.culture_container[prim_culture].group : cultures::culture_group_tag();

			cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::cultural_union_of], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
			cursor_in = ui::add_linear_text(cursor_in,
				is_valid_index(cg) ? ws.s.culture_m.culture_groups[cg].name : ws.s.fixed_ui_text[scenario::fixed_ui::singular_nation],
				fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
			cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
			lm.finish_current_line();

			if(count_subtriggers(tval) > 1) {
				cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt, 3);
				cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[any_all_code_to_fixed_ui(tval[0])], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
				cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
				lm.finish_current_line();
			}

			lm.increase_indent(1);
			if(is_valid_index(cg)) {
				auto union_tag = ws.s.culture_m.culture_groups[cg].union_tag;
				if(is_valid_index(union_tag)) {
					auto union_holder = ws.w.culture_s.national_tags_state[union_tag].holder;
					cursor_in = display_subtriggers(tval, ws, container, cursor_in, lm, fmt,
						union_holder, this_slot, from_slot, rebel_slot);
				}
			}
			lm.decrease_indent(1);
			return cursor_in;
		}
	}

	/*
	#define TRIGGER_DISPLAY_PARAMS uint16_t const* tval, world_state& ws, ui::tagged_gui_object container, \
			ui::xy_pair cursor_in, ui::unlimited_line_manager& lm, ui::text_format const& fmt, void* primary_slot, \
			void* this_slot, void* from_slot, population::rebel_faction* rebel_slot
	*/

	static ui::xy_pair(*scope_functions[])(uint16_t const* tval, world_state& ws, ui::tagged_gui_object,
		ui::xy_pair, ui::unlimited_line_manager&, ui::text_format const&,
		void*, void*, void*, population::rebel_faction*) = {
		tf_generic_scope, //constexpr uint16_t generic_scope = 0x0000; // or & and
		tf_x_neighbor_province_scope, //constexpr uint16_t x_neighbor_province_scope = 0x0001;
		tf_x_neighbor_country_scope_nation, //constexpr uint16_t x_neighbor_country_scope_nation = 0x0002;
		tf_x_neighbor_country_scope_pop, //constexpr uint16_t x_neighbor_country_scope_pop = 0x0003;
		tf_x_war_countries_scope_nation, //constexpr uint16_t x_war_countries_scope_nation = 0x0004;
		tf_x_war_countries_scope_pop, //constexpr uint16_t x_war_countries_scope_pop = 0x0005;
		tf_x_greater_power_scope, //constexpr uint16_t x_greater_power_scope = 0x0006;
		tf_x_owned_province_scope_state, //constexpr uint16_t x_owned_province_scope_state = 0x0007;
		tf_x_owned_province_scope_nation, //constexpr uint16_t x_owned_province_scope_nation = 0x0008;
		tf_x_core_scope_province, //constexpr uint16_t x_core_scope_province = 0x0009;
		tf_x_core_scope_nation, //constexpr uint16_t x_core_scope_nation = 0x000A;
		tf_x_state_scope, //constexpr uint16_t x_state_scope = 0x000B;
		tf_x_substate_scope, //constexpr uint16_t x_substate_scope = 0x000C;
		tf_x_sphere_member_scope, //constexpr uint16_t x_sphere_member_scope = 0x000D;
		tf_x_pop_scope_province, //constexpr uint16_t x_pop_scope_province = 0x000E;
		tf_x_pop_scope_state, //constexpr uint16_t x_pop_scope_state = 0x000F;
		tf_x_pop_scope_nation, //constexpr uint16_t x_pop_scope_nation = 0x0010;
		tf_x_provinces_in_variable_region, //constexpr uint16_t x_provinces_in_variable_region = 0x0011; // variable name
		tf_owner_scope_state, //constexpr uint16_t owner_scope_state = 0x0012;
		tf_owner_scope_province, //constexpr uint16_t owner_scope_province = 0x0013;
		tf_controller_scope, //constexpr uint16_t controller_scope = 0x0014;
		tf_location_scope, //constexpr uint16_t location_scope = 0x0015;
		tf_country_scope_state, //constexpr uint16_t country_scope_state = 0x0016;
		tf_country_scope_pop, //constexpr uint16_t country_scope_pop = 0x0017;
		tf_capital_scope, //constexpr uint16_t capital_scope = 0x0018;
		tf_this_scope_pop, //constexpr uint16_t this_scope_pop = 0x0019;
		tf_this_scope_nation, //constexpr uint16_t this_scope_nation = 0x001A;
		tf_this_scope_state, //constexpr uint16_t this_scope_state = 0x001B;
		tf_this_scope_province, //constexpr uint16_t this_scope_province = 0x001C;
		tf_from_scope_pop, //constexpr uint16_t from_scope_pop = 0x001D;
		tf_from_scope_nation, //constexpr uint16_t from_scope_nation = 0x001E;
		tf_from_scope_state, //constexpr uint16_t from_scope_state = 0x001F;
		tf_from_scope_province, //constexpr uint16_t from_scope_province = 0x0020;
		tf_sea_zone_scope, //constexpr uint16_t sea_zone_scope = 0x0021;
		tf_cultural_union_scope, //constexpr uint16_t cultural_union_scope = 0x0022;
		tf_overlord_scope, //constexpr uint16_t overlord_scope = 0x0023;
		tf_sphere_owner_scope, //constexpr uint16_t sphere_owner_scope = 0x0024;
		tf_independence_scope, //constexpr uint16_t independence_scope = 0x0025;
		tf_flashpoint_tag_scope, //constexpr uint16_t flashpoint_tag_scope = 0x0026;
		tf_crisis_state_scope, //constexpr uint16_t crisis_state_scope = 0x0027;
		tf_state_scope_pop, //constexpr uint16_t state_scope_pop = 0x0028;
		tf_state_scope_province, //constexpr uint16_t state_scope_province = 0x0029;
		tf_tag_scope, //constexpr uint16_t tag_scope = 0x002A; // variable name
		tf_integer_scope, //constexpr uint16_t integer_scope = 0x002B; // variable name
		tf_country_scope_nation, //constexpr uint16_t country_scope_nation = 0x002C;
		tf_country_scope_province, //constexpr uint16_t country_scope_province = 0x002D;
		tf_cultural_union_scope_pop, //constexpr uint16_t cultural_union_scope_pop = 0x002E;
	};

	ui::xy_pair tf_year(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::year, int32_t(tval[2]), display_type::integer, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_month(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::month, int32_t(tval[2]), display_type::integer, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_port(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::a_port], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_rank(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::national_rank, int32_t(tval[2]), display_type::integer, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_technology(TRIGGER_DISPLAY_PARAMS) {
		cursor_in = display_with_has_comparison_no_newline(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::technology], ws, container, cursor_in, lm, fmt);
		cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
		auto tech = trigger_payload(tval[2]).tech;
		cursor_in = ui::add_linear_text(cursor_in, ws.s.technology_m.technologies_container[tech].name, fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
		cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
		lm.finish_current_line();
		return cursor_in;
	}
	ui::xy_pair tf_strata_rich(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::strata, ws.s.fixed_ui_text[scenario::fixed_ui::rich], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_strata_middle(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::strata, ws.s.fixed_ui_text[scenario::fixed_ui::middle], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_strata_poor(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::strata, ws.s.fixed_ui_text[scenario::fixed_ui::poor], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_life_rating_province(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::life_rating, trigger_payload(tval[2]).signed_value, display_type::integer, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_life_rating_state(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::minimum_state_life_raing, trigger_payload(tval[2]).signed_value, display_type::integer, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_has_empty_adjacent_state_province(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::an_empy_adj_prov], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_has_empty_adjacent_state_state(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::an_empy_adj_prov], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_state_id_province(TRIGGER_DISPLAY_PARAMS) {
		provinces::province_tag pid(tval[2]);

		text_data::replacement repl[1] = {
				text_data::replacement{
					text_data::value_type::text,
					text_data::text_tag_to_backing(ws.s.gui_m.text_data_sequences, ws.w.province_s.province_state_container[pid].name),
					[](ui::tagged_gui_object) {} }
		};

		cursor_in = ui::add_linear_text(cursor_in,
			ws.s.fixed_ui_text[(tval[0] & trigger_codes::association_mask) == trigger_codes::association_eq ?
				scenario::fixed_ui::belongs_to_same_state :
				scenario::fixed_ui::does_not_belong_to_same_state], 
			fmt, ws.s.gui_m, ws.w.gui_m, container, lm, repl, 1);
		cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
		lm.finish_current_line();
		return cursor_in;
	}
	ui::xy_pair tf_state_id_state(TRIGGER_DISPLAY_PARAMS) {
		provinces::province_tag pid(tval[2]);

		text_data::replacement repl[1] = {
				text_data::replacement{
					text_data::value_type::text,
					text_data::text_tag_to_backing(ws.s.gui_m.text_data_sequences, ws.w.province_s.province_state_container[pid].name),
					[](ui::tagged_gui_object) {} }
		};

		cursor_in = ui::add_linear_text(cursor_in,
			ws.s.fixed_ui_text[(tval[0] & trigger_codes::association_mask) == trigger_codes::association_eq ?
			scenario::fixed_ui::contains_blank :
			scenario::fixed_ui::does_not_contain_blank],
			fmt, ws.s.gui_m, ws.w.gui_m, container, lm, repl, 1);
		cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
		lm.finish_current_line();
		return cursor_in;
	}
	ui::xy_pair tf_cash_reserves(TRIGGER_DISPLAY_PARAMS) {
		cursor_in = display_with_comparison_no_newline(tval[0], scenario::fixed_ui::pop_savings, read_float_from_payload(tval + 2), display_type::fp_one_place, ws, container, cursor_in, lm, fmt);
		cursor_in = ui::advance_cursor_by_space(cursor_in, ws.s.gui_m, fmt);
		cursor_in = ui::add_linear_text(cursor_in, ws.s.fixed_ui_text[scenario::fixed_ui::times_target_needs_spending], fmt, ws.s.gui_m, ws.w.gui_m, container, lm);
		cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
		lm.finish_current_line();
		return cursor_in;
	}
	ui::xy_pair tf_unemployment_nation(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::unemployment, read_float_from_payload(tval + 2), display_type::percent,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_unemployment_state(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::unemployment, read_float_from_payload(tval + 2), display_type::percent,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_unemployment_province(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::unemployment, read_float_from_payload(tval + 2), display_type::percent,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_unemployment_pop(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::unemployment, read_float_from_payload(tval + 2), display_type::percent,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_is_slave_nation(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::slavery, ws.s.fixed_ui_text[scenario::fixed_ui::allowed], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_is_slave_state(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::a_slave_state], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_is_slave_province(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], scenario::fixed_ui::slavery, ws.s.fixed_ui_text[scenario::fixed_ui::allowed], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_is_slave_pop(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::a_slave], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_is_independant(TRIGGER_DISPLAY_PARAMS) {
		return display_with_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::an_independent_nation], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_has_national_minority_province(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::non_prim_culture_pops], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_has_national_minority_state(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::non_prim_culture_pops], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_has_national_minority_nation(TRIGGER_DISPLAY_PARAMS) {
		return display_with_has_comparison(tval[0], ws.s.fixed_ui_text[scenario::fixed_ui::non_prim_culture_pops], ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_government_nation(TRIGGER_DISPLAY_PARAMS) {
		auto gov = trigger_payload(tval[2]).small.values.government;
		return display_with_comparison(tval[0], scenario::fixed_ui::government, ws.s.governments_m.governments_container[gov].name, ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_government_pop(TRIGGER_DISPLAY_PARAMS) {
		auto gov = trigger_payload(tval[2]).small.values.government;
		return display_with_comparison(tval[0], scenario::fixed_ui::government, ws.s.governments_m.governments_container[gov].name,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_capital(TRIGGER_DISPLAY_PARAMS) {
		auto prov = provinces::province_tag(tval[2]);
		return display_with_comparison(tval[0], scenario::fixed_ui::capital, ws.w.province_s.province_state_container[prov].name,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_tech_school(TRIGGER_DISPLAY_PARAMS) {
		auto t = trigger_payload(tval[2]).nat_mod;
		return display_with_comparison(tval[0], scenario::fixed_ui::technology_school, ws.s.modifiers_m.national_modifiers[t].name,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_primary_culture(TRIGGER_DISPLAY_PARAMS) {
		auto t = trigger_payload(tval[2]).culture;
		return display_with_comparison(tval[0], scenario::fixed_ui::primary_culture, ws.s.culture_m.culture_container[t].name,
			ws, container, cursor_in, lm, fmt);
	}
	ui::xy_pair tf_accepted_culture(TRIGGER_DISPLAY_PARAMS) {
		auto has_culture = contains_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, trigger_payload(tval[2]).culture);
		return compare_values(tval[0], has_culture, true);
	}
	ui::xy_pair tf_culture_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_culture_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_culture_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_culture_nation(TRIGGER_DISPLAY_PARAMS) {
		auto has_culture = ((nations::nation*)primary_slot)->primary_culture == trigger_payload(tval[2]).culture || contains_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, trigger_payload(tval[2]).culture);
		return compare_values(tval[0], has_culture, true);
	}
	ui::xy_pair tf_culture_pop_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->culture == rebel_slot->culture, true);
	}
	ui::xy_pair tf_culture_state_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_culture == rebel_slot->culture, true);
	}
	ui::xy_pair tf_culture_province_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_culture == rebel_slot->culture, true);
	}
	ui::xy_pair tf_culture_nation_reb(TRIGGER_DISPLAY_PARAMS) {
		auto has_culture = ((nations::nation*)primary_slot)->primary_culture == rebel_slot->culture || contains_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, rebel_slot->culture);
		return compare_values(tval[0], has_culture, true);
	}
	ui::xy_pair tf_culture_this_nation(TRIGGER_DISPLAY_PARAMS) {
		auto has_culture = ((nations::nation*)this_slot)->primary_culture == ((population::pop*)primary_slot)->culture || contains_item(ws.w.culture_s.culture_arrays, ((nations::nation*)this_slot)->accepted_cultures, ((population::pop*)primary_slot)->culture);
		return compare_values(tval[0], has_culture, true);
	}
	ui::xy_pair tf_culture_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_culture_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_culture_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_owner = ((nations::state_instance*)this_slot)->owner;
		if(state_owner)
			return tf_culture_this_nation(tval, ws, primary_slot, state_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)this_slot)->location;
		if(is_valid_index(location)) {
			auto prov_owner = ws.w.province_s.province_state_container[location].owner;
			if(prov_owner)
				return tf_culture_this_nation(tval, ws, primary_slot, prov_owner, nullptr, nullptr);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_owner = ((provinces::province_state*)this_slot)->owner;
		if(prov_owner)
			return tf_culture_this_nation(tval, ws, primary_slot, prov_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_nation(TRIGGER_DISPLAY_PARAMS) {
		auto cg = trigger_payload(tval[2]).culture_group;
		auto nation_pculture = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(nation_pculture))
			return compare_values(tval[0], ws.s.culture_m.culture_container[nation_pculture].group == cg, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_pop(TRIGGER_DISPLAY_PARAMS) {
		auto cg = trigger_payload(tval[2]).culture_group;
		auto pculture = ((population::pop*)primary_slot)->culture;
		if(is_valid_index(pculture))
			return compare_values(tval[0], ws.s.culture_m.culture_container[pculture].group == cg, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_reb_nation(TRIGGER_DISPLAY_PARAMS) {
		auto rc = rebel_slot->culture;
		if(is_valid_index(rc)) {
			auto cg = ws.s.culture_m.culture_container[rc].group;
			auto nation_pculture = ((nations::nation*)primary_slot)->primary_culture;
			if(is_valid_index(nation_pculture))
				return compare_values(tval[0], ws.s.culture_m.culture_container[nation_pculture].group == cg, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_reb_pop(TRIGGER_DISPLAY_PARAMS) {
		auto rc = rebel_slot->culture;
		if(is_valid_index(rc)) {
			auto cg = ws.s.culture_m.culture_container[rc].group;
			auto pculture = ((population::pop*)primary_slot)->culture;
			if(is_valid_index(pculture))
				return compare_values(tval[0], ws.s.culture_m.culture_container[pculture].group == cg, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_nation_this_nation(TRIGGER_DISPLAY_PARAMS) {
		auto rc = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(rc)) {
			auto cg = ws.s.culture_m.culture_container[rc].group;
			auto nation_pculture = ((nations::nation*)this_slot)->primary_culture;
			if(is_valid_index(nation_pculture))
				return compare_values(tval[0], ws.s.culture_m.culture_container[nation_pculture].group == cg, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_pop_this_nation(TRIGGER_DISPLAY_PARAMS) {
		auto rc = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(rc)) {
			auto cg = ws.s.culture_m.culture_container[rc].group;
			auto pculture = ((population::pop*)this_slot)->culture;
			if(is_valid_index(pculture))
				return compare_values(tval[0], ws.s.culture_m.culture_container[pculture].group == cg, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_nation_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_culture_group_nation_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_culture_group_pop_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_culture_group_pop_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_culture_group_nation_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_culture_group_nation_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_pop_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_culture_group_pop_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_nation_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_culture_group_nation_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_pop_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_culture_group_pop_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_culture_group_nation_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_culture_group_pop_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_culture_group_pop_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_religion(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_religion_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == rebel_slot->religion, true);
	}
	ui::xy_pair tf_religion_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == ((nations::nation*)from_slot)->national_religion, true);
	}
	ui::xy_pair tf_religion_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == ((nations::nation*)this_slot)->national_religion, true);
	}
	ui::xy_pair tf_religion_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((population::pop*)primary_slot)->religion == owner->national_religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_religion_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((population::pop*)primary_slot)->religion == owner->national_religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_religion_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], ((population::pop*)primary_slot)->religion == owner->national_religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_terrain_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->terrain == trigger_payload(tval[2]).prov_mod, true);
	}
	ui::xy_pair tf_terrain_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location))
			return compare_values(tval[0], ws.w.province_s.province_state_container[location].terrain == trigger_payload(tval[2]).prov_mod, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_trade_goods(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->rgo_production == trigger_payload(tval[2]).small.values.good, true);
	}

	ui::xy_pair tf_is_secondary_power_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->overall_rank <= int32_t(ws.s.modifiers_m.global_defines.colonial_rank), true);
	}
	ui::xy_pair tf_is_secondary_power_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_is_secondary_power_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_faction_nation(TRIGGER_DISPLAY_PARAMS) {
		auto nation_factions = get_range(ws.w.population_s.rebel_faction_arrays, ((nations::nation*)primary_slot)->active_rebel_factions);
		auto rebel_type = trigger_payload(tval[2]).small.values.rebel;
		bool has_faction = false;
		for(auto f : nation_factions) {
			if(is_valid_index(f) && ws.w.population_s.rebel_factions[f].type == rebel_type) {
				has_faction = true;
				break;
			}
		}
		return compare_values(tval[0], has_faction, true);
	}
	ui::xy_pair tf_has_faction_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_faction = ((population::pop*)primary_slot)->rebel_faction;
		if(is_valid_index(pop_faction))
			return compare_values(tval[0], ws.w.population_s.rebel_factions[pop_faction].type == trigger_payload(tval[2]).small.values.rebel, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_unclaimed_cores(TRIGGER_DISPLAY_PARAMS) {
		auto nation_tag = ((nations::nation*)primary_slot)->tag;
		bool has_all_cores = true;
		if(is_valid_index(nation_tag)) {
			auto core_range = get_range(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[nation_tag].core_provinces);
			for(auto p : core_range) {
				if(is_valid_index(p) && ws.w.province_s.province_state_container[p].owner != ((nations::nation*)primary_slot)) {
					has_all_cores = false;
					break;
				}
			}
		}
		return compare_values(tval[0], has_all_cores, true);
	}
	ui::xy_pair tf_is_cultural_union_bool(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::union_holder_of(ws, *((nations::nation*)primary_slot)) == primary_slot, true);
	}
	ui::xy_pair tf_is_cultural_union_this_self_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pculture = ((population::pop*)primary_slot)->culture;
		auto pop_union = is_valid_index(pculture) ? nations::union_holder_for(ws, pculture) : nullptr;

		return compare_values(tval[0], this_slot == pop_union, true);
	}
	ui::xy_pair tf_is_cultural_union_pop_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto nation = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(nation)
			return tf_is_cultural_union_this_self_pop(tval, ws, primary_slot, nation, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_this_nation(TRIGGER_DISPLAY_PARAMS) {
		auto main_union = nations::union_holder_of(ws, *((nations::nation*)primary_slot));
		return compare_values(tval[0], this_slot == main_union, true);
	}
	ui::xy_pair tf_is_cultural_union_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(pop_owner)
			return tf_is_cultural_union_this_nation(tval, ws, primary_slot, pop_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_owner = ((nations::state_instance*)this_slot)->owner;
		if(state_owner)
			return tf_is_cultural_union_this_nation(tval, ws, primary_slot, state_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_owner = ((provinces::province_state*)this_slot)->owner;
		if(prov_owner)
			return tf_is_cultural_union_this_nation(tval, ws, primary_slot, prov_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_this_rebel(TRIGGER_DISPLAY_PARAMS) {
		if(is_valid_index(rebel_slot->culture))
			return compare_values(tval[0], primary_slot == nations::union_holder_for(ws, rebel_slot->culture), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_tag_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::union_tag_of(ws, *((nations::nation*)primary_slot)) == trigger_payload(tval[2]).tag, true);
	}
	ui::xy_pair tf_is_cultural_union_tag_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(pop_owner)
			return tf_is_cultural_union_tag_nation(tval, ws, pop_owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_tag_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_owner = ((nations::state_instance*)this_slot)->owner;
		if(state_owner)
			return tf_is_cultural_union_tag_nation(tval, ws, state_owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_tag_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_owner = ((provinces::province_state*)this_slot)->owner;
		if(prov_owner)
			return tf_is_cultural_union_tag_nation(tval, ws, prov_owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_cultural_union_tag_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_is_cultural_union_tag_nation(tval, ws, this_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_can_build_factory_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->current_rules.rules_value | issues::rules::pop_build_factory), true);
	}
	ui::xy_pair tf_can_build_factory_province(TRIGGER_DISPLAY_PARAMS) {
		auto p_owner = ((provinces::province_state*)primary_slot)->owner;
		if(p_owner)
			return tf_can_build_factory_nation(tval, ws, p_owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_can_build_factory_pop(TRIGGER_DISPLAY_PARAMS) {
		auto p_owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(p_owner)
			return tf_can_build_factory_nation(tval, ws, p_owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.military_s.war_arrays, ((nations::nation*)primary_slot)->wars_involved_in) != 0, true);
	}
	ui::xy_pair tf_war_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_war_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_exhaustion_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->war_exhaustion, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_war_exhaustion_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_war_exhaustion_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_war_exhaustion_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_war_exhaustion_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_blockade(TRIGGER_DISPLAY_PARAMS) {
		auto cpc = float(((nations::nation*)primary_slot)->central_province_count);
		if(cpc != 0.0f)
			return compare_values(tval[0], float(((nations::nation*)primary_slot)->blockaded_count) / cpc, read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_owns(TRIGGER_DISPLAY_PARAMS) {
		provinces::province_tag prov(tval[2]);
		return compare_values(tval[0], ws.w.province_s.province_state_container[prov].owner == primary_slot, true);
	}
	ui::xy_pair tf_controls(TRIGGER_DISPLAY_PARAMS) {
		provinces::province_tag prov(tval[2]);
		return compare_values(tval[0], ws.w.province_s.province_state_container[prov].controller == primary_slot, true);
	}
	ui::xy_pair tf_is_core_integer(TRIGGER_DISPLAY_PARAMS) {
		provinces::province_tag prov(tval[2]);
		return compare_values(tval[0], contains_item(ws.w.province_s.core_arrays, ws.w.province_s.province_state_container[prov].cores, ((nations::nation*)primary_slot)->tag), true);
	}
	ui::xy_pair tf_is_core_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, ((nations::nation*)this_slot)->tag), true);
	}
	ui::xy_pair tf_is_core_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_is_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_core_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_is_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_core_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_is_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_core_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_is_core_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_is_core_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, rebel_slot->independence_tag), true);
	}
	ui::xy_pair tf_is_core_tag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, trigger_payload(tval[2]).tag), true);
	}
	ui::xy_pair tf_num_of_revolts(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->rebel_controlled_provinces, tval[2]);
	}
	ui::xy_pair tf_revolt_percentage(TRIGGER_DISPLAY_PARAMS) {
		auto cpc = float(((nations::nation*)primary_slot)->central_province_count);
		if(cpc != 0.0f)
			return compare_values(tval[0], float(((nations::nation*)primary_slot)->rebel_controlled_provinces) / cpc, read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_num_of_cities_int(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces), uint32_t(tval[2]));
	}
	ui::xy_pair tf_num_of_cities_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces),
			get_size(ws.w.province_s.province_arrays, ((nations::nation*)from_slot)->owned_provinces));
	}
	ui::xy_pair tf_num_of_cities_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces),
			get_size(ws.w.province_s.province_arrays, ((nations::nation*)this_slot)->owned_provinces));
	}
	ui::xy_pair tf_num_of_cities_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_num_of_cities_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces), 0ui32);
	}
	ui::xy_pair tf_num_of_cities_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_num_of_cities_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces), 0ui32);
	}
	ui::xy_pair tf_num_of_cities_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_num_of_cities_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces), 0ui32);
	}
	ui::xy_pair tf_num_of_ports(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->num_connected_ports, tval[2]);
	}
	ui::xy_pair tf_num_of_allies(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies), uint32_t(tval[2]));
	}
	ui::xy_pair tf_num_of_vassals(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->vassals), uint32_t(tval[2]));
	}
	ui::xy_pair tf_owned_by_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		return compare_values(tval[0], bool((holder != nullptr)  & (holder == ((provinces::province_state*)primary_slot)->owner)), true);
	}
	ui::xy_pair tf_owned_by_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], bool((this_slot != nullptr)  & (this_slot == ((provinces::province_state*)primary_slot)->owner)), true);
	}
	ui::xy_pair tf_owned_by_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_owned_by_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_owned_by_this_province(TRIGGER_DISPLAY_PARAMS) {
		return tf_owned_by_this_nation(tval, ws, primary_slot, ((provinces::province_state*)this_slot)->owner, nullptr, nullptr);
	}
	ui::xy_pair tf_owned_by_this_state(TRIGGER_DISPLAY_PARAMS) {
		return tf_owned_by_this_nation(tval, ws, primary_slot, ((nations::state_instance*)this_slot)->owner, nullptr, nullptr);
	}
	ui::xy_pair tf_owned_by_this_pop(TRIGGER_DISPLAY_PARAMS) {
		return tf_owned_by_this_nation(tval, ws, primary_slot, population::get_pop_owner(ws, *((population::pop*)this_slot)), nullptr, nullptr);
	}
	ui::xy_pair tf_exists_bool(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces) != 0ui32, true);
	}
	ui::xy_pair tf_exists_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], get_size(ws.w.province_s.province_arrays, holder->owned_provinces) != 0ui32, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_country_flag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.variable_s.national_flags_arrays, ((nations::nation*)primary_slot)->national_flags, trigger_payload(tval[2]).nat_flag), true);
	}
	ui::xy_pair tf_has_country_flag_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_has_country_flag(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_country_flag_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_has_country_flag(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_continent_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_id = ((provinces::province_state*)primary_slot)->id;
		return compare_values(tval[0], ws.s.province_m.province_container[prov_id].continent == trigger_payload(tval[2]).prov_mod, true);
	}
	ui::xy_pair tf_continent_state(TRIGGER_DISPLAY_PARAMS) {
		auto region_id = ((nations::state_instance*)primary_slot)->region_id;
		if(is_valid_index(region_id)) {
			auto prov_id = ws.s.province_m.states_to_province_index.get(region_id, 0);
			return compare_values(tval[0], ws.s.province_m.province_container[prov_id].continent == trigger_payload(tval[2]).prov_mod, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_continent_nation(TRIGGER_DISPLAY_PARAMS) {
		auto prov_id = ((nations::nation*)primary_slot)->current_capital;
		if(is_valid_index(prov_id))
			return compare_values(tval[0], ws.s.province_m.province_container[prov_id].continent == trigger_payload(tval[2]).prov_mod, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_continent_pop(TRIGGER_DISPLAY_PARAMS) {
		auto prov_id = ((population::pop*)primary_slot)->location;
		if(is_valid_index(prov_id))
			return compare_values(tval[0], ws.s.province_m.province_container[prov_id].continent == trigger_payload(tval[2]).prov_mod, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_continent_nation_this(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_nation(tval, ws, this_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_state_this(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_state(tval, ws, this_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_province_this(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_province(tval, ws, this_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_pop_this(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_pop(tval, ws, this_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_nation_from(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_nation(tval, ws, from_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_state_from(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_state(tval, ws, from_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_province_from(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_province(tval, ws, from_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_continent_pop_from(TRIGGER_DISPLAY_PARAMS) {
		return tf_continent_pop(tval, ws, from_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_casus_belli_tag(TRIGGER_DISPLAY_PARAMS) {
		auto tag_holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(tag_holder)
			return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *tag_holder), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_casus_belli_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot)), true);
	}
	ui::xy_pair tf_casus_belli_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot)), true);
	}
	ui::xy_pair tf_casus_belli_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_casus_belli_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_casus_belli_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], military::can_use_cb_against(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_military_access_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *holder), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_military_access_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot)), true);
	}
	ui::xy_pair tf_military_access_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot)), true);
	}
	ui::xy_pair tf_military_access_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_military_access_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_military_access_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], military::has_military_access_with(ws, *((nations::nation*)primary_slot), *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_prestige_value(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_prestige_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), nations::get_prestige(*((nations::nation*)from_slot)));
	}
	ui::xy_pair tf_prestige_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), nations::get_prestige(*((nations::nation*)this_slot)));
	}
	ui::xy_pair tf_prestige_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), nations::get_prestige(*owner));
		else
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), 0.0f);
	}
	ui::xy_pair tf_prestige_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), nations::get_prestige(*owner));
		else
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), 0.0f);
	}
	ui::xy_pair tf_prestige_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), nations::get_prestige(*owner));
		else
			return compare_values(tval[0], nations::get_prestige(*((nations::nation*)primary_slot)), 0.0f);
	}
	ui::xy_pair tf_badboy(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->infamy, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_has_building_fort(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->fort_level != 0ui8, true);
	}
	ui::xy_pair tf_has_building_railroad(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->railroad_level != 0ui8, true);
	}
	ui::xy_pair tf_has_building_naval_base(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->railroad_level != 0ui8, true);
	}

	ui::xy_pair tf_has_building_factory(TRIGGER_DISPLAY_PARAMS) {
		auto si = ((nations::state_instance*)primary_slot);
		auto has_factories = [si]() {
			for(uint32_t i = 0; i < std::extent_v<decltype(si->factories)>; ++i) {
				if(si->factories[i].type)
					return true;
			}
			return false;
		}();
		return compare_values(tval[0], has_factories, true);
	}
	ui::xy_pair tf_has_building_state(TRIGGER_DISPLAY_PARAMS) {
		auto si = ((nations::state_instance*)primary_slot);
		auto type = trigger_payload(tval[2]).small.values.factory;
		auto has_factory = [si, type]() {
			for(uint32_t i = 0; i < std::extent_v<decltype(si->factories)>; ++i) {
				if(si->factories[i].type && si->factories[i].type->id == type)
					return true;
			}
			return false;
		}();
		return compare_values(tval[0], has_factory, true);
	}
	ui::xy_pair tf_has_building_state_from_province(TRIGGER_DISPLAY_PARAMS) {
		auto state = ((provinces::province_state*)primary_slot)->state_instance;
		if(state)
			return tf_has_building_state(tval, ws, state, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_building_factory_from_province(TRIGGER_DISPLAY_PARAMS) {
		auto state = ((provinces::province_state*)primary_slot)->state_instance;
		if(state)
			return tf_has_building_factory(tval, ws, state, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_empty(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->owner == nullptr, true);
	}
	ui::xy_pair tf_is_blockaded(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], (((provinces::province_state*)primary_slot)->flags & provinces::province_state::is_blockaded) != 0, true);
	}
	ui::xy_pair tf_has_country_modifier(TRIGGER_DISPLAY_PARAMS) {
		const auto mod = trigger_payload(tval[2]).nat_mod;
		return compare_values(tval[0],
			modifiers::has_national_modifier(ws, *((nations::nation*)primary_slot), mod),
			true);
	}
	ui::xy_pair tf_has_country_modifier_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_has_country_modifier(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_province_modifier(TRIGGER_DISPLAY_PARAMS) {
		const auto mod = trigger_payload(tval[2]).prov_mod;
		return compare_values(tval[0],
			modifiers::has_provincial_modifier(ws, *((provinces::province_state*)primary_slot), mod),
			true);
	}
	ui::xy_pair tf_region(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].state_id == trigger_payload(tval[2]).state, true);
	}
	ui::xy_pair tf_tag_tag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->tag == trigger_payload(tval[2]).tag, true);
	}
	ui::xy_pair tf_tag_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], primary_slot == this_slot, true);
	}
	ui::xy_pair tf_tag_this_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], primary_slot == ((provinces::province_state*)this_slot)->owner, true);
	}
	ui::xy_pair tf_tag_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], primary_slot == from_slot, true);
	}
	ui::xy_pair tf_tag_from_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], primary_slot == ((provinces::province_state*)from_slot)->owner, true);
	}
	ui::xy_pair tf_tag_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_tag_tag(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_neighbour_tag(TRIGGER_DISPLAY_PARAMS) {
		auto tag_holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(tag_holder)
			return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->neighboring_nations, tag_holder->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_neighbour_this(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			contains_item(
				ws.w.nation_s.nations_arrays,
				((nations::nation*)primary_slot)->neighboring_nations,
				((nations::nation*)this_slot)->id),
			true);
	}
	ui::xy_pair tf_neighbour_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			contains_item(
				ws.w.nation_s.nations_arrays,
				((nations::nation*)primary_slot)->neighboring_nations,
				((nations::nation*)from_slot)->id),
			true);
	}
	ui::xy_pair tf_units_in_province_value(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::total_units_in_province(ws, *((provinces::province_state*)primary_slot)), uint32_t(tval[2]));
	}
	ui::xy_pair tf_units_in_province_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::has_units_in_province(ws, *((nations::nation*)from_slot), *((provinces::province_state*)primary_slot)), true);
	}
	ui::xy_pair tf_units_in_province_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::has_units_in_province(ws, *((nations::nation*)this_slot), *((provinces::province_state*)primary_slot)), true);
	}
	ui::xy_pair tf_units_in_province_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::has_units_in_province(ws, *owner, *((provinces::province_state*)primary_slot)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_units_in_province_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::has_units_in_province(ws, *owner, *((provinces::province_state*)primary_slot)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_units_in_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], military::has_units_in_province(ws, *owner, *((provinces::province_state*)primary_slot)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_with_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), holder->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_with_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id), true);
	}
	ui::xy_pair tf_war_with_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->id), true);
	}
	ui::xy_pair tf_war_with_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_with_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_war_with_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], military::in_war_against(ws, *((nations::nation*)primary_slot), owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_unit_in_battle(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::province_is_contested(ws, *((provinces::province_state*)primary_slot)), true);
	}
	ui::xy_pair tf_total_amount_of_divisions(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::total_active_divisions(ws, *((nations::nation*)primary_slot)), uint32_t(tval[2]));
	}
	ui::xy_pair tf_money(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], nations::national_treasury(ws, id), economy::goods_qnty_type(read_float_from_payload(tval + 2)));
		else
			return compare_values(tval[0], economy::goods_qnty_type(0.0), economy::goods_qnty_type(read_float_from_payload(tval + 2)));
	}
	ui::xy_pair tf_lost_national(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 1.0f - nations::fraction_of_cores_owned(ws, *((nations::nation*)primary_slot)), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_is_vassal(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord != nullptr, true);
	}
	ui::xy_pair tf_ruling_party_ideology_nation(TRIGGER_DISPLAY_PARAMS) {
		auto ri = ((nations::nation*)primary_slot)->ruling_ideology;
		if(is_valid_index(ri))
			return compare_values(tval[0], ri == trigger_payload(tval[2]).small.values.ideology, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_ruling_party_ideology_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_ruling_party_ideology_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_ruling_party(TRIGGER_DISPLAY_PARAMS) {
		auto rp = ((nations::nation*)primary_slot)->ruling_party;
		if(is_valid_index(rp))
			return compare_values(tval[0], ws.s.governments_m.parties[rp].name == trigger_payload(tval[2]).text, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_ideology_enabled(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ideologies::ideology_enabled(ws, trigger_payload(tval[2]).small.values.ideology), true);
	}
	ui::xy_pair tf_political_reform_want_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->political_interest_fraction, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_political_reform_want_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(pop_id))
			return compare_values(tval[0], issues::calculate_political_interest(ws, ws.w.population_s.pop_demographics.get_row(pop_id)), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_social_reform_want_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->social_interest_fraction, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_social_reform_want_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(pop_id))
			return compare_values(tval[0], issues::calculate_social_interest(ws, ws.w.population_s.pop_demographics.get_row(pop_id)), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_total_amount_of_ships(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::total_active_ships(ws, *((nations::nation*)primary_slot)), uint32_t(tval[2]));
	}
	ui::xy_pair tf_plurality(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->plurality, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_corruption(TRIGGER_DISPLAY_PARAMS) {
		auto cpc = float(((nations::nation*)primary_slot)->central_province_count);
		if(cpc != 0.0f)
			return compare_values(tval[0], float(((nations::nation*)primary_slot)->crime_count) / cpc, read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_is_state_religion_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], owner->national_religion == ((population::pop*)primary_slot)->religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_state_religion_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->national_religion == ((provinces::province_state*)primary_slot)->dominant_religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_state_religion_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->national_religion == ((nations::state_instance*)primary_slot)->dominant_religion, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], owner->primary_culture == ((population::pop*)primary_slot)->culture, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->primary_culture == ((provinces::province_state*)primary_slot)->dominant_culture, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->primary_culture == ((nations::state_instance*)primary_slot)->dominant_culture, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->primary_culture == ((population::pop*)this_slot)->culture, true);
	}
	ui::xy_pair tf_is_primary_culture_nation_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->primary_culture == ((nations::nation*)this_slot)->primary_culture, true);
	}
	ui::xy_pair tf_is_primary_culture_nation_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((nations::state_instance*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_nation_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_nation_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((provinces::province_state*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_nation_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_state_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_culture == ((nations::nation*)this_slot)->primary_culture, true);
	}
	ui::xy_pair tf_is_primary_culture_state_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(this_owner)
			return tf_is_primary_culture_state_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_state_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((nations::state_instance*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_state_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_state_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((provinces::province_state*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_state_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_province_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_culture == ((nations::nation*)this_slot)->primary_culture, true);
	}
	ui::xy_pair tf_is_primary_culture_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(this_owner)
			return tf_is_primary_culture_province_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_province_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((nations::state_instance*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_province_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_province_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((provinces::province_state*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_province_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_pop_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->culture == ((nations::nation*)this_slot)->primary_culture, true);
	}
	ui::xy_pair tf_is_primary_culture_pop_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(this_owner)
			return tf_is_primary_culture_pop_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_pop_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((nations::state_instance*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_pop_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_primary_culture_pop_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto this_owner = ((provinces::province_state*)this_slot)->owner;
		if(this_owner)
			return tf_is_primary_culture_pop_this_nation(tval, ws, primary_slot, this_owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_accepted_culture_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0],
				owner->primary_culture == ((population::pop*)primary_slot)->culture ||
				contains_item(ws.w.culture_s.culture_arrays, owner->accepted_cultures, ((population::pop*)primary_slot)->culture),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_accepted_culture_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0],
				owner->primary_culture == ((provinces::province_state*)primary_slot)->dominant_culture ||
				contains_item(ws.w.culture_s.culture_arrays, owner->accepted_cultures, ((provinces::province_state*)primary_slot)->dominant_culture),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_accepted_culture_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0],
				owner->primary_culture == ((nations::state_instance*)primary_slot)->dominant_culture ||
				contains_item(ws.w.culture_s.culture_arrays, owner->accepted_cultures, ((nations::state_instance*)primary_slot)->dominant_culture),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_coastal(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			0 != (ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].flags & provinces::province::coastal),
			true);
	}
	ui::xy_pair tf_in_sphere_tag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			((nations::nation*)primary_slot)->sphere_leader == ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder,
			true);
	}
	ui::xy_pair tf_in_sphere_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			((nations::nation*)primary_slot)->sphere_leader == from_slot,
			true);
	}
	ui::xy_pair tf_in_sphere_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			((nations::nation*)primary_slot)->sphere_leader == this_slot,
			true);
	}
	ui::xy_pair tf_in_sphere_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_in_sphere_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_sphere_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_in_sphere_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_sphere_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_in_sphere_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_produces_nation(TRIGGER_DISPLAY_PARAMS) {
		auto good = trigger_payload(tval[2]).small.values.good;

		auto owned_range = get_range(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces);
		for(auto p : owned_range) {
			provinces::province_state& ps = ws.w.province_s.province_state_container[p];
			if((ps.rgo_production == good) | (ps.artisan_production == good))
				return compare_values(tval[0], true, true);
		}
		auto states = get_range(ws.w.nation_s.state_arrays, ((nations::nation*)primary_slot)->member_states);
		for(auto s = states.first; s != states.second; ++s) {
			auto si = s->state;
			for(int32_t i = int32_t(std::extent_v<decltype(si->factories)>); i--; ) {
				if(si->factories[i].type && si->factories[i].type->output_good == good)
					return compare_values(tval[0], true, true);
			}
		}

		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_produces_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			(((provinces::province_state*)primary_slot)->rgo_production == trigger_payload(tval[2]).small.values.good) ||
			(((provinces::province_state*)primary_slot)->artisan_production == trigger_payload(tval[2]).small.values.good),
			true);
	}
	ui::xy_pair tf_produces_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_provinces = ws.s.province_m.states_to_province_index.get_row(((nations::state_instance*)primary_slot)->region_id);
		auto state_owner = ((nations::state_instance*)primary_slot)->owner;
		auto good = trigger_payload(tval[2]).small.values.good;

		for(auto p : state_provinces) {
			provinces::province_state& ps = ws.w.province_s.province_state_container[p];
			if((ps.owner == state_owner) & ((ps.rgo_production == good) | (ps.artisan_production == good)))
				return compare_values(tval[0], true, true);
		}
		for(int32_t i = int32_t(std::extent_v<decltype(((nations::state_instance*)primary_slot)->factories)>); i--; ) {
			if(((nations::state_instance*)primary_slot)->factories[i].type && ((nations::state_instance*)primary_slot)->factories[i].type->output_good == good)
				return compare_values(tval[0], true, true);
		}
		return compare_values(tval[0], false, true);
	}

	ui::xy_pair tf_produces_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_location = ((population::pop*)primary_slot)->location;
		if(((population::pop*)primary_slot)->type == ws.s.population_m.artisan && is_valid_index(pop_location))
			return compare_values(tval[0],
				ws.w.province_s.province_state_container[pop_location].rgo_production == trigger_payload(tval[2]).small.values.good,
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_average_militancy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto nation_id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(nation_id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(nation_id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(nation_id, population::militancy_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_average_militancy_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(state_id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(state_id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(state_id, population::militancy_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_average_militancy_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(prov_id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(prov_id, population::militancy_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_average_consciousness_nation(TRIGGER_DISPLAY_PARAMS) {
		auto nation_id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(nation_id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(nation_id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(nation_id, population::consciousness_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_average_consciousness_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(state_id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(state_id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(state_id, population::consciousness_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_average_consciousness_province(TRIGGER_DISPLAY_PARAMS) {
		auto prov_id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(prov_id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(prov_id, population::consciousness_demo_tag(ws))) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_is_next_reform_nation(TRIGGER_DISPLAY_PARAMS) {
		auto reform_id = trigger_payload(tval[2]).small.values.option;
		auto reform_parent = ws.s.issues_m.options[reform_id].parent_issue;
		auto nation_id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(nation_id)) {
			auto active_option = ws.w.nation_s.active_issue_options.get(nation_id, reform_parent);
			return compare_values(tval[0], (to_index(active_option) + 1) == to_index(reform_id), true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_is_next_reform_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_is_next_reform_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_rebel_power_fraction(TRIGGER_DISPLAY_PARAMS) {
		// note: virtually unused
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_recruited_percentage_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::recruited_pop_fraction(ws, *((nations::nation*)primary_slot)), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_recruited_percentage_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_recruited_percentage_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_has_culture_core(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		auto culture = ((population::pop*)primary_slot)->culture;
		if(is_valid_index(location)) {
			auto cores = get_range(ws.w.province_s.core_arrays, ws.w.province_s.province_state_container[location].cores);
			for(auto tag : cores) {
				if(is_valid_index(tag)) {
					if(auto holder = ws.w.culture_s.national_tags_state[tag].holder; bool(holder) && holder->primary_culture == culture)
						return compare_values(tval[0], true, true);
				}
			}
			return compare_values(tval[0], false, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_has_culture_core_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((provinces::province_state*)primary_slot)->id;
		auto culture = ((population::pop*)this_slot)->culture;
		if(is_valid_index(location)) {
			auto cores = get_range(ws.w.province_s.core_arrays, ws.w.province_s.province_state_container[location].cores);
			for(auto tag : cores) {
				if(is_valid_index(tag)) {
					if(auto holder = ws.w.culture_s.national_tags_state[tag].holder; bool(holder) && holder->primary_culture == culture)
						return compare_values(tval[0], true, true);
				}
			}
			return compare_values(tval[0], false, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_nationalism(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->nationalism, float(tval[2]));
	}
	ui::xy_pair tf_is_overseas(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((provinces::province_state*)primary_slot)->flags & provinces::province_state::is_overseas), true);
	}
	ui::xy_pair tf_controlled_by_rebels(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->rebel_controller != nullptr, true);
	}
	ui::xy_pair tf_controlled_by_tag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder, true);
	}
	ui::xy_pair tf_controlled_by_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == from_slot, true);
	}
	ui::xy_pair tf_controlled_by_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == this_slot, true);
	}
	ui::xy_pair tf_controlled_by_this_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == ((provinces::province_state*)this_slot)->owner, true);
	}
	ui::xy_pair tf_controlled_by_this_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == ((nations::state_instance*)this_slot)->owner, true);
	}
	ui::xy_pair tf_controlled_by_this_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->controller == population::get_pop_owner(ws, *((population::pop*)this_slot)), true);
	}
	ui::xy_pair tf_controlled_by_owner(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->owner == ((provinces::province_state*)primary_slot)->controller, true);
	}
	ui::xy_pair tf_controlled_by_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->rebel_controller == rebel_slot, true);
	}
	ui::xy_pair tf_is_canal_enabled(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.is_canal_enabled[tval[2] - 1] != 0ui8, true);
	}
	ui::xy_pair tf_is_state_capital(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto si = ((provinces::province_state*)primary_slot)->state_instance;
		if(si) {
			auto state_range = ws.s.province_m.states_to_province_index.get_row(ws.s.province_m.province_container[id].state_id);
			for(auto ip : state_range) {
				if(ws.w.province_s.province_state_container[ip].state_instance == si) {
					if(ip == id)
						return compare_values(tval[0], true, true);
					else
						return compare_values(tval[0], false, true);
				}
			}
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_truce_with_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), holder->id }), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_truce_with_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), ((nations::nation*)from_slot)->id }), true);
	}
	ui::xy_pair tf_truce_with_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), ((nations::nation*)this_slot)->id }), true);
	}
	ui::xy_pair tf_truce_with_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), owner->id }), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_truce_with_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), owner->id }), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_truce_with_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.truce_arrays, ((nations::nation*)primary_slot)->truces, nations::truce { date_tag(), owner->id }), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_total_pops_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::total_population_tag)), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_total_pops_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::total_population_tag)), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_total_pops_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::total_population_tag)), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_total_pops_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			if(auto si = ws.w.province_s.province_state_container[location].state_instance; si)
				return tf_total_pops_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_has_pop_type_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0],
				0 != ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type)),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_type_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0],
				0 != ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type)),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_type_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		return compare_values(tval[0], 0 != ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type)), true);
	}
	ui::xy_pair tf_has_pop_type_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->type == trigger_payload(tval[2]).small.values.pop_type, true);
	}
	ui::xy_pair tf_has_empty_adjacent_province(TRIGGER_DISPLAY_PARAMS) {
		auto adj_range = ws.s.province_m.same_type_adjacency.get_row(((provinces::province_state*)primary_slot)->id);
		for(auto p : adj_range) {
			if(ws.w.province_s.province_state_container[p].owner == nullptr)
				return compare_values(tval[0], true, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_leader(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], military::has_named_leader(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).text), true);
	}
	ui::xy_pair tf_ai(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_not_ai_controlled), true);
	}
	ui::xy_pair tf_can_create_vassals(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], true, true); // stub for apparently unused 
	}
	ui::xy_pair tf_is_possible_vassal(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], can_release_as_vassal(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tag), true);
	}
	ui::xy_pair tf_province_id(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->id == provinces::province_tag(tval[2]), true);
	}
	ui::xy_pair tf_vassal_of_tag(TRIGGER_DISPLAY_PARAMS) {
		auto tag_holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(tag_holder)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == tag_holder, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_vassal_of_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == from_slot, true);
	}
	ui::xy_pair tf_vassal_of_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == this_slot, true);
	}
	ui::xy_pair tf_vassal_of_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == owner, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_vassal_of_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == owner, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_vassal_of_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->overlord == owner, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_substate_of_tag(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_tag(tval, ws, primary_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_substate_of_from(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_substate_of_this_nation(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_this_nation(tval, ws, primary_slot, this_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_substate_of_this_province(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_this_province(tval, ws, primary_slot, this_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_substate_of_this_state(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_this_state(tval, ws, primary_slot, this_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_substate_of_this_pop(TRIGGER_DISPLAY_PARAMS) {
		if(0 == (((nations::nation*)primary_slot)->flags & nations::nation::is_substate))
			return compare_values(tval[0], false, true);
		else
			return tf_vassal_of_this_pop(tval, ws, primary_slot, this_slot, nullptr, nullptr);
	}
	ui::xy_pair tf_alliance_with_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, holder->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_alliance_with_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, ((nations::nation*)from_slot)->id), true);
	}
	ui::xy_pair tf_alliance_with_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, ((nations::nation*)this_slot)->id), true);
	}
	ui::xy_pair tf_alliance_with_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_alliance_with_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_alliance_with_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], contains_item(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies, owner->id), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_recently_lost_war(TRIGGER_DISPLAY_PARAMS) {
		auto last_lost = ((nations::nation*)primary_slot)->last_lost_war;
		return compare_values(tval[0], is_valid_index(last_lost) && (to_index(ws.w.current_date) - to_index(last_lost)) < (365 * 5 + 1), true);
	}
	ui::xy_pair tf_is_mobilised(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_mobilized), true);
	}
	ui::xy_pair tf_mobilisation_size(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->modifier_values[modifiers::national_offsets::mobilisation_size], read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_crime_higher_than_education_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->administrative_spending >= ((nations::nation*)primary_slot)->education_spending, true);
	}
	ui::xy_pair tf_crime_higher_than_education_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->administrative_spending >= owner->education_spending, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_crime_higher_than_education_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->administrative_spending >= owner->education_spending, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_crime_higher_than_education_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], owner->administrative_spending >= owner->education_spending, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_agree_with_ruling_party(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		auto pop_id = ((population::pop*)primary_slot)->id;
		if(owner && ws.w.population_s.pops.is_valid_index(pop_id)) {
			auto ruling_ideology = owner->ruling_ideology;
			auto population_size = ws.w.population_s.pop_demographics.get(pop_id, population::total_population_tag);
			if(is_valid_index(ruling_ideology) & (population_size != 0)) {
				return compare_values(tval[0],
					float(ws.w.population_s.pop_demographics.get(pop_id, population::to_demo_tag(ws, ruling_ideology))) / float(population_size),
					read_float_from_payload(tval + 2));
			}
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_is_colonial_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			0 != (((nations::state_instance*)primary_slot)->flags & (nations::state_instance::is_colonial | nations::state_instance::is_protectorate)),
			true);
	}
	ui::xy_pair tf_is_colonial_province(TRIGGER_DISPLAY_PARAMS) {
		auto si = ((provinces::province_state*)primary_slot)->state_instance;
		if(si)
			return tf_is_colonial_state(tval, ws, si, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_factories(TRIGGER_DISPLAY_PARAMS) {
		for(int32_t i = int32_t(std::extent_v<decltype(((nations::state_instance*)primary_slot)->factories)>); i--; ) {
			if(((nations::state_instance*)primary_slot)->factories[i].type)
				return compare_values(tval[0], true, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_default_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0],
			(0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_bankrupt)) &&
				contains_item(ws.w.nation_s.loan_arrays, ((nations::nation*)primary_slot)->loans, nations::loan { 0.0f, holder->id }),
				true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_default_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			(0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_bankrupt)) &&
			contains_item(ws.w.nation_s.loan_arrays, ((nations::nation*)primary_slot)->loans, nations::loan { 0.0f, ((nations::nation*)from_slot)->id }),
			true);
	}
	ui::xy_pair tf_in_default_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			(0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_bankrupt)) &&
			contains_item(ws.w.nation_s.loan_arrays, ((nations::nation*)primary_slot)->loans, nations::loan { 0.0f, ((nations::nation*)this_slot)->id }),
			true);
	}
	ui::xy_pair tf_in_default_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_in_default_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_default_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_in_default_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_in_default_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_in_default_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_total_num_of_ports(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->num_ports, tval[2]);
	}
	ui::xy_pair tf_always(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_election(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_holding_election), true);
	}
	ui::xy_pair tf_has_global_flag(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.variable_s.global_variables[trigger_payload(tval[2]).global_var] != 0.0f, true);
	}
	ui::xy_pair tf_is_capital(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->current_capital == ((provinces::province_state*)primary_slot)->id, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_nationalvalue_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->national_value == trigger_payload(tval[2]).nat_mod, true);
	}
	ui::xy_pair tf_nationalvalue_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_nationalvalue_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_nationalvalue_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_nationalvalue_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_industrial_score_value(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, trigger_payload(tval[2]).signed_value);
	}
	ui::xy_pair tf_industrial_score_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, ((nations::nation*)from_slot)->industrial_score);
	}
	ui::xy_pair tf_industrial_score_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, ((nations::nation*)this_slot)->industrial_score);
	}
	ui::xy_pair tf_industrial_score_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, owner->industrial_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, 0i16);
	}
	ui::xy_pair tf_industrial_score_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, owner->industrial_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, 0i16);
	}
	ui::xy_pair tf_industrial_score_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, owner->industrial_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->industrial_score, 0i16);
	}
	ui::xy_pair tf_military_score_value(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, trigger_payload(tval[2]).signed_value);
	}
	ui::xy_pair tf_military_score_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, ((nations::nation*)from_slot)->military_score);
	}
	ui::xy_pair tf_military_score_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, ((nations::nation*)this_slot)->military_score);
	}
	ui::xy_pair tf_military_score_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, owner->military_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, 0i16);
	}
	ui::xy_pair tf_military_score_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, owner->military_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, 0i16);
	}
	ui::xy_pair tf_military_score_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, owner->military_score);
		else
			return compare_values(tval[0], ((nations::nation*)primary_slot)->military_score, 0i16);
	}
	ui::xy_pair tf_civilized_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_civilized), true);
	}
	ui::xy_pair tf_civilized_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], 0 != (owner->flags & nations::nation::is_civilized), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_civilized_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], 0 != (owner->flags & nations::nation::is_civilized), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_national_provinces_occupied(TRIGGER_DISPLAY_PARAMS) {
		auto owned_range = get_range(ws.w.province_s.province_arrays, ((nations::nation*)primary_slot)->owned_provinces);
		if(owned_range.first != owned_range.second) {
			int32_t count_owned_controlled = 0;
			for(auto p : owned_range) {
				if(ws.w.province_s.province_state_container[p].controller == primary_slot)
					++count_owned_controlled;
			}
			return compare_values(tval[0], float(count_owned_controlled) / float(owned_range.second - owned_range.first), read_float_from_payload(tval + 2));
		} else {
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
		}
	}
	ui::xy_pair tf_is_greater_power_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::is_great_power(ws, *((nations::nation*)primary_slot)), true);
	}
	ui::xy_pair tf_is_greater_power_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], nations::is_great_power(ws, *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_greater_power_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return compare_values(tval[0], nations::is_great_power(ws, *owner), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_rich_tax(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->rich_tax, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_middle_tax(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->middle_tax, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_poor_tax(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->poor_tax, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_social_spending_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->social_spending, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_social_spending_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_social_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_social_spending_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_social_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_military_spending_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->military_spending, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_military_spending_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_military_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_military_spending_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_military_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_military_spending_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return tf_military_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_administration_spending_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->administrative_spending, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_administration_spending_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_administration_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_administration_spending_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_administration_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_administration_spending_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return tf_administration_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_education_spending_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->education_spending, int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_education_spending_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_education_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_education_spending_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_education_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_education_spending_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return tf_education_spending_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], int8_t(0), int8_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_colonial_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_colonial_nation), true);
	}
	ui::xy_pair tf_pop_majority_religion_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->dominant_religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_pop_majority_religion_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_pop_majority_religion_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_pop_majority_culture_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->dominant_culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_pop_majority_culture_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_pop_majority_culture_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_pop_majority_issue_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->dominant_issue == trigger_payload(tval[2]).small.values.option, true);
	}
	ui::xy_pair tf_pop_majority_issue_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_issue == trigger_payload(tval[2]).small.values.option, true);
	}
	ui::xy_pair tf_pop_majority_issue_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_issue == trigger_payload(tval[2]).small.values.option, true);
	}
	ui::xy_pair tf_pop_majority_issue_pop(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(id))
			return compare_values(tval[0], population::is_dominant_issue(ws, id, trigger_payload(tval[2]).small.values.option), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_pop_majority_ideology_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->dominant_ideology == trigger_payload(tval[2]).small.values.ideology, true);
	}
	ui::xy_pair tf_pop_majority_ideology_state(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->dominant_ideology == trigger_payload(tval[2]).small.values.ideology, true);
	}
	ui::xy_pair tf_pop_majority_ideology_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((provinces::province_state*)primary_slot)->dominant_ideology == trigger_payload(tval[2]).small.values.ideology, true);
	}
	ui::xy_pair tf_pop_majority_ideology_pop(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(id))
			return compare_values(tval[0], population::is_dominant_ideology(ws, id, trigger_payload(tval[2]).small.values.ideology), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_poor_strata_militancy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.nation_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.nation_demographics.get(id, population::poor_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_militancy_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.state_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.state_demographics.get(id, population::poor_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_militancy_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto pop_size = float(ws.w.province_s.province_demographics.get(id, population::total_population_tag));
		auto mil = 10.0f * float(ws.w.province_s.province_demographics.get(id, population::poor_militancy_demo_tag(ws)));
		return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_militancy_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)primary_slot)->type;
		if(is_valid_index(type) && (ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor)
			return compare_values(tval[0], float(((population::pop*)primary_slot)->militancy) * 10.0f / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_militancy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.nation_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.nation_demographics.get(id, population::middle_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_militancy_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.state_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.state_demographics.get(id, population::middle_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_militancy_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto pop_size = float(ws.w.province_s.province_demographics.get(id, population::total_population_tag));
		auto mil = 10.0f * float(ws.w.province_s.province_demographics.get(id, population::middle_militancy_demo_tag(ws)));
		return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_militancy_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)primary_slot)->type;
		if(is_valid_index(type) && (ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle)
			return compare_values(tval[0], float(((population::pop*)primary_slot)->militancy) * 10.0f / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_militancy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.nation_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.nation_demographics.get(id, population::rich_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_militancy_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto pop_size = float(ws.w.nation_s.state_demographics.get(id, population::total_population_tag));
			auto mil = 10.0f * float(ws.w.nation_s.state_demographics.get(id, population::rich_militancy_demo_tag(ws)));
			return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_militancy_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto pop_size = float(ws.w.province_s.province_demographics.get(id, population::total_population_tag));
		auto mil = 10.0f * float(ws.w.province_s.province_demographics.get(id, population::rich_militancy_demo_tag(ws)));
		return compare_values(tval[0], (pop_size != 0.0f) ? mil / pop_size : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_militancy_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)primary_slot)->type;
		if(is_valid_index(type) && (ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich)
			return compare_values(tval[0], float(((population::pop*)primary_slot)->militancy) * 10.0f / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_tax_above_poor(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->rich_tax > ((nations::nation*)primary_slot)->poor_tax, true);
	}
	ui::xy_pair tf_culture_has_union_tag_pop(TRIGGER_DISPLAY_PARAMS) {
		auto pop_culture = ((population::pop*)primary_slot)->culture;
		if(is_valid_index(pop_culture)) {
			auto group = ws.s.culture_m.culture_container[pop_culture].group;
			return compare_values(tval[0], is_valid_index(ws.s.culture_m.culture_groups[group].union_tag), true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_culture_has_union_tag_nation(TRIGGER_DISPLAY_PARAMS) {
		auto primary_culture = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(primary_culture)) {
			auto group = ws.s.culture_m.culture_container[primary_culture].group;
			return compare_values(tval[0], is_valid_index(ws.s.culture_m.culture_groups[group].union_tag), true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_this_culture_union_tag(TRIGGER_DISPLAY_PARAMS) {
		auto primary_culture = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(primary_culture)) {
			auto group = ws.s.culture_m.culture_container[primary_culture].group;
			return compare_values(tval[0], ws.s.culture_m.culture_groups[group].union_tag == trigger_payload(tval[2]).tag, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_this_culture_union_from(TRIGGER_DISPLAY_PARAMS) {
		auto primary_culture = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(primary_culture)) {
			auto group = ws.s.culture_m.culture_container[primary_culture].group;
			return compare_values(tval[0], ws.s.culture_m.culture_groups[group].union_tag == ((nations::nation*)from_slot)->tag, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_this_culture_union_this_nation(TRIGGER_DISPLAY_PARAMS) {
		auto primary_culture = ((nations::nation*)primary_slot)->primary_culture;
		if(is_valid_index(primary_culture)) {
			auto group = ws.s.culture_m.culture_container[primary_culture].group;
			return compare_values(tval[0], ws.s.culture_m.culture_groups[group].union_tag == ((nations::nation*)this_slot)->tag, true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_this_culture_union_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_this_culture_union_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_this_culture_union_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_this_culture_union_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_this_culture_union_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_this_culture_union_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_this_culture_union_this_union_nation(TRIGGER_DISPLAY_PARAMS) {
		auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
		auto this_culture = ((nations::nation*)this_slot)->primary_culture;
		return compare_values(tval[0],
			is_valid_index(prim_culture) &&
			is_valid_index(this_culture) &&
			ws.s.culture_m.culture_container[prim_culture].group == ws.s.culture_m.culture_container[this_culture].group, true);
	}
	ui::xy_pair tf_this_culture_union_this_union_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_this_culture_union_this_union_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_this_culture_union_this_union_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_this_culture_union_this_union_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_this_culture_union_this_union_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_this_culture_union_this_union_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_minorities_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			auto accepted_pop = ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, ((nations::nation*)primary_slot)->primary_culture));
			auto accepted_range = get_range(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures);
			for(auto c : accepted_range)
				accepted_pop += ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, c));
			return compare_values(tval[0], total_pop != accepted_pop, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_minorities_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(ws.w.nation_s.states.is_valid_index(id) && owner) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			auto accepted_pop = ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, owner->primary_culture));
			auto accepted_range = get_range(ws.w.culture_s.culture_arrays, owner->accepted_cultures);
			for(auto c : accepted_range)
				accepted_pop += ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, c));
			return compare_values(tval[0], total_pop != accepted_pop, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_minorities_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner) {
			auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
			auto accepted_pop = ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, owner->primary_culture));
			auto accepted_range = get_range(ws.w.culture_s.culture_arrays, owner->accepted_cultures);
			for(auto c : accepted_range)
				accepted_pop += ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, c));
			return compare_values(tval[0], total_pop != accepted_pop, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_revanchism_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->revanchism, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_revanchism_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return compare_values(tval[0], owner->revanchism, read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_has_crime(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], is_valid_index(((provinces::province_state*)primary_slot)->crime), true);
	}
	ui::xy_pair tf_num_of_substates(TRIGGER_DISPLAY_PARAMS) {
		auto vassal_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->vassals);
		uint32_t count_substates = 0ui32;
		for(auto v : vassal_range) {
			if(is_valid_index(v) && 0 != (ws.w.nation_s.nations[v].flags & nations::nation::is_substate))
				++count_substates;
		}
		return compare_values(tval[0], count_substates, uint32_t(tval[2]));
	}
	ui::xy_pair tf_num_of_vassals_no_substates(TRIGGER_DISPLAY_PARAMS) {
		auto vassal_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->vassals);
		uint32_t count_non_substates = 0ui32;
		for(auto v : vassal_range) {
			if(is_valid_index(v) && 0 == (ws.w.nation_s.nations[v].flags & nations::nation::is_substate))
				++count_non_substates;
		}
		return compare_values(tval[0], count_non_substates, uint32_t(tval[2]));
	}
	ui::xy_pair tf_brigades_compare_this(TRIGGER_DISPLAY_PARAMS) {
		auto main_brigades = military::total_active_divisions(ws, *((nations::nation*)primary_slot));
		auto this_brigades = military::total_active_divisions(ws, *((nations::nation*)this_slot));
		return compare_values(tval[0],
			this_brigades != 0 ? float(main_brigades) / float(this_brigades) : 1'000'000.0f,
			read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_brigades_compare_from(TRIGGER_DISPLAY_PARAMS) {
		auto main_brigades = military::total_active_divisions(ws, *((nations::nation*)primary_slot));
		auto from_brigades = military::total_active_divisions(ws, *((nations::nation*)from_slot));
		return compare_values(tval[0],
			from_brigades != 0 ? float(main_brigades) / float(from_brigades) : 1'000'000.0f,
			read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_constructing_cb_tag(TRIGGER_DISPLAY_PARAMS) {
		auto tag_holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(tag_holder)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == tag_holder->id, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_constructing_cb_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == ((nations::nation*)from_slot)->id, true);
	}
	ui::xy_pair tf_constructing_cb_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == ((nations::nation*)this_slot)->id, true);
	}
	ui::xy_pair tf_constructing_cb_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == owner->id, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_constructing_cb_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == owner->id, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_constructing_cb_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_target == owner->id, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_constructing_cb_discovered(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_cb_construction_discovered), true);
	}
	ui::xy_pair tf_constructing_cb_progress(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_progress, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_civilization_progress(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->modifier_values[modifiers::national_offsets::civilization_progress_modifier], read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_constructing_cb_type(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->cb_construction_type == trigger_payload(tval[2]).small.values.cb_type, true);
	}
	ui::xy_pair tf_is_our_vassal_tag(TRIGGER_DISPLAY_PARAMS) {
		auto tag_holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(tag_holder)
			return compare_values(tval[0], tag_holder->overlord == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_our_vassal_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)from_slot)->overlord == primary_slot, true);
	}
	ui::xy_pair tf_is_our_vassal_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)this_slot)->overlord == primary_slot, true);
	}
	ui::xy_pair tf_is_our_vassal_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->overlord == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_our_vassal_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->overlord == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_our_vassal_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], owner->overlord == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_substate(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], 0 != (((nations::nation*)primary_slot)->flags & nations::nation::is_substate), true);
	}
	ui::xy_pair tf_great_wars_enabled(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.great_wars_enabled, true);
	}
	ui::xy_pair tf_can_nationalize(TRIGGER_DISPLAY_PARAMS) {
		auto influencer_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->influencers);
		auto this_id = ((nations::nation*)primary_slot)->id;
		for(auto c : influencer_range) {
			if(ws.w.nation_s.nations.is_valid_index(c) && nations::get_foreign_investment(ws, ws.w.nation_s.nations[c], this_id) != 0.0f)
				return compare_values(tval[0], true, true);
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_part_of_sphere(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->sphere_leader != nullptr, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
		if(holder)
			return compare_values(tval[0], holder->sphere_leader == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_from(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)from_slot)->sphere_leader == primary_slot, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)this_slot)->sphere_leader == primary_slot, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->sphere_leader == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return compare_values(tval[0], owner->sphere_leader == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_sphere_leader_of_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return compare_values(tval[0], owner->sphere_leader == primary_slot, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_number_of_states(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], get_size(ws.w.nation_s.state_arrays, ((nations::nation*)primary_slot)->member_states), uint32_t(tval[2]));
	}
	ui::xy_pair tf_war_score(TRIGGER_DISPLAY_PARAMS) {
		//stub for apparently unused trigger
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_is_releasable_vassal_from(TRIGGER_DISPLAY_PARAMS) {
		auto tag = ((nations::nation*)from_slot)->tag;
		if(is_valid_index(tag))
			return compare_values(tval[0], !(ws.w.culture_s.national_tags_state[tag].is_not_releasable), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_is_releasable_vassal_other(TRIGGER_DISPLAY_PARAMS) {
		auto tag = ((nations::nation*)primary_slot)->tag;
		if(is_valid_index(tag))
			return compare_values(tval[0], !(ws.w.culture_s.national_tags_state[tag].is_not_releasable), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_recent_imigration(TRIGGER_DISPLAY_PARAMS) {
		auto last_immigration = ((provinces::province_state*)primary_slot)->last_immigration;
		if(!is_valid_index(last_immigration))
			return compare_values(tval[0], std::numeric_limits<uint16_t>::max(), tval[2]);
		else
			return compare_values(tval[0], to_index(ws.w.current_date) - to_index(last_immigration), int32_t(tval[2]));
	}
	ui::xy_pair tf_province_control_days(TRIGGER_DISPLAY_PARAMS) {
		auto last_control_change = ((provinces::province_state*)primary_slot)->last_controller_change;
		if(!is_valid_index(last_control_change))
			return compare_values(tval[0], std::numeric_limits<uint16_t>::max(), tval[2]);
		else
			return compare_values(tval[0], to_index(ws.w.current_date) - to_index(last_control_change), int32_t(tval[2]));
	}
	ui::xy_pair tf_is_disarmed(TRIGGER_DISPLAY_PARAMS) {
		auto disarmed_date = ((nations::nation*)primary_slot)->disarmed_until;
		return compare_values(tval[0], is_valid_index(disarmed_date) && ws.w.current_date < disarmed_date, true);
	}
	ui::xy_pair tf_big_producer(TRIGGER_DISPLAY_PARAMS) {
		// stub: used only rarely in ai chances and would be expensive to test
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_someone_can_form_union_tag_from(TRIGGER_DISPLAY_PARAMS) {
		// stub: apparently unused
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_someone_can_form_union_tag_other(TRIGGER_DISPLAY_PARAMS) {
		// stub: apparently unused
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_social_movement_strength(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->social_movement_support, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_political_movement_strength(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->political_movement_support, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_can_build_factory_in_capital_state(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_social_movement(TRIGGER_DISPLAY_PARAMS) {
		auto mt = ((population::pop*)primary_slot)->movement;
		if(is_valid_index(mt))
			return compare_values(tval[0], ws.w.population_s.pop_movements[mt].type == population::movement_type::social, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_political_movement(TRIGGER_DISPLAY_PARAMS) {
		auto mt = ((population::pop*)primary_slot)->movement;
		if(is_valid_index(mt))
			return compare_values(tval[0], ws.w.population_s.pop_movements[mt].type == population::movement_type::political, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_cultural_sphere(TRIGGER_DISPLAY_PARAMS) {
		auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
		auto culture_group = is_valid_index(prim_culture) ? ws.s.culture_m.culture_container[prim_culture].group : cultures::culture_group_tag();
		auto sphere_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->sphere_members);
		for(auto c : sphere_range) {
			if(is_valid_index(c)) {
				auto sc = ws.w.nation_s.nations[c].primary_culture;
				if(is_valid_index(sc) && culture_group == ws.s.culture_m.culture_container[sc].group)
					return compare_values(tval[0], true, true);
			}
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_world_wars_enabled(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.world_wars_enabled, true);
	}
	ui::xy_pair tf_has_pop_culture_pop_this_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->culture == ((population::pop*)this_slot)->culture, true);
	}
	ui::xy_pair tf_has_pop_culture_state_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto culture = ((population::pop*)this_slot)->culture;
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(is_valid_index(culture) && ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, culture)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_culture_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto culture = ((population::pop*)this_slot)->culture;
		auto id = ((provinces::province_state*)primary_slot)->id;
		if(is_valid_index(culture))
			return compare_values(tval[0], 0 != ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, culture)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_culture_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto culture = ((population::pop*)this_slot)->culture;
		auto id = ((nations::nation*)primary_slot)->id;
		if(is_valid_index(culture) && ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, culture)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_culture_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->culture == trigger_payload(tval[2]).culture, true);
	}
	ui::xy_pair tf_has_pop_culture_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).culture)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_culture_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		return compare_values(tval[0], 0 != ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).culture)), true);
	}
	ui::xy_pair tf_has_pop_culture_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).culture)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_religion_pop_this_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == ((population::pop*)this_slot)->religion, true);
	}
	ui::xy_pair tf_has_pop_religion_state_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto religion = ((population::pop*)this_slot)->religion;
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(is_valid_index(religion) && ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, religion)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_religion_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto religion = ((population::pop*)this_slot)->religion;
		auto id = ((provinces::province_state*)primary_slot)->id;
		if(is_valid_index(religion))
			return compare_values(tval[0], 0 != ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, religion)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_religion_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto religion = ((population::pop*)this_slot)->religion;
		auto id = ((nations::nation*)primary_slot)->id;
		if(is_valid_index(religion) && ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, religion)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_religion_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_has_pop_religion_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.religion)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_pop_religion_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		return compare_values(tval[0], 0 != ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.religion)), true);
	}
	ui::xy_pair tf_has_pop_religion_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], 0 != ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.religion)), true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_life_needs(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->needs_satisfaction, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_everyday_needs(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->needs_satisfaction, read_float_from_payload(tval + 2) + 1.0f);
	}
	ui::xy_pair tf_luxury_needs(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((population::pop*)primary_slot)->needs_satisfaction, read_float_from_payload(tval + 2) + 2.0f);
	}
	ui::xy_pair tf_consciousness_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], float(((population::pop*)primary_slot)->consciousness) * 10.0f / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_consciousness_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		auto con = ws.w.province_s.province_demographics.get(id, population::consciousness_demo_tag(ws));
		return compare_values(tval[0], total_pop != 0 ? (float(con) * 10.0f / float(total_pop)) : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_consciousness_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.state_demographics.get(id, population::consciousness_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_consciousness_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.nation_demographics.get(id, population::consciousness_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_literacy_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], float(((population::pop*)primary_slot)->literacy) / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_literacy_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		auto con = ws.w.province_s.province_demographics.get(id, population::literacy_demo_tag(ws));
		return compare_values(tval[0], total_pop != 0 ? (float(con) / float(total_pop)) : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_literacy_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.state_demographics.get(id, population::literacy_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_literacy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.nation_demographics.get(id, population::literacy_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_militancy_pop(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], float(((population::pop*)primary_slot)->militancy) * 10.0f / float(std::numeric_limits<uint16_t>::max()), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_militancy_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		auto con = ws.w.province_s.province_demographics.get(id, population::militancy_demo_tag(ws));
		return compare_values(tval[0], total_pop != 0 ? (float(con) * 10.0f / float(total_pop)) : 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_militancy_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.state_demographics.get(id, population::militancy_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_militancy_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			auto con = ws.w.nation_s.nation_demographics.get(id, population::militancy_demo_tag(ws));
			if(total_pop != 0)
				return compare_values(tval[0], float(con) * 10.0f / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_trade_goods_in_state_state(TRIGGER_DISPLAY_PARAMS) {
		auto state_region = ((nations::state_instance*)primary_slot)->region_id;
		if(is_valid_index(state_region)) {
			auto prov_range = ws.s.province_m.states_to_province_index.get_row(state_region);
			for(auto p : prov_range) {
				auto& ps = ws.w.province_s.province_state_container[p];
				if(ps.state_instance == primary_slot && ps.rgo_production == trigger_payload(tval[2]).small.values.good)
					return compare_values(tval[0], true, true);
			}
		}
		return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_trade_goods_in_state_province(TRIGGER_DISPLAY_PARAMS) {
		auto si = ((provinces::province_state*)primary_slot)->state_instance;
		if(si)
			return tf_trade_goods_in_state_state(tval, ws, si, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_has_flashpoint(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], is_valid_index(((nations::state_instance*)primary_slot)->crisis_tag), true);
	}
	ui::xy_pair tf_flashpoint_tension(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::state_instance*)primary_slot)->current_tension, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_crisis_exist(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.current_crisis_type != current_state::crisis_type::none, true);
	}
	ui::xy_pair tf_is_liberation_crisis(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.current_crisis_type == current_state::crisis_type::liberation, true);
	}
	ui::xy_pair tf_is_claim_crisis(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.current_crisis_type == current_state::crisis_type::claim, true);
	}
	ui::xy_pair tf_crisis_temperature(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.crisis_temperature, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_involved_in_crisis_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		return compare_values(tval[0],
			contains_item(ws.w.nation_s.nations_arrays, ws.w.crisis_attackers, id) ||
			contains_item(ws.w.nation_s.nations_arrays, ws.w.crisis_defenders, id) ||
			contains_item(ws.w.nation_s.nations_arrays, ws.w.crisis_interested, id),
			true);
	}
	ui::xy_pair tf_involved_in_crisis_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_involved_in_crisis_nation(nullptr, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_rich_strata_life_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::rich_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_life_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::rich_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_life_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::rich_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_life_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_rich_strata_life_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_everyday_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::rich_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_everyday_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::rich_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_everyday_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::rich_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_everyday_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_rich_strata_everyday_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_luxury_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::rich_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_luxury_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::rich_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_luxury_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::rich_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_rich_strata_luxury_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_rich_strata_luxury_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_life_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::middle_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_life_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::middle_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_life_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::middle_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_life_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_middle_strata_life_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_everyday_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::middle_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_everyday_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::middle_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_everyday_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::middle_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_everyday_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_middle_strata_everyday_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_luxury_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::middle_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_luxury_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::middle_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_luxury_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::middle_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_middle_strata_luxury_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_middle_strata_luxury_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_life_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::poor_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_life_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::poor_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_life_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::poor_life_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_life_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_poor_strata_life_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_everyday_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::poor_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_everyday_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::poor_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_everyday_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::poor_everyday_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_everyday_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_poor_strata_everyday_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_luxury_needs_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.nation_demographics.get(id, population::poor_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_luxury_needs_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0], float(ws.w.nation_s.state_demographics.get(id, population::poor_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_luxury_needs_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0], float(ws.w.province_s.province_demographics.get(id, population::poor_luxury_needs_demo_tag(ws))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_poor_strata_luxury_needs_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_poor_strata_luxury_needs_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_diplomatic_influence_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[3]).tag].holder;
		if(holder)
			return compare_values(tval[0], nations::get_influence_value(ws, *((nations::nation*)primary_slot), holder->id), int32_t(tval[2]));
		else
			return compare_values(tval[0], 0ui16, tval[2]);
	}
	ui::xy_pair tf_diplomatic_influence_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_influence_value(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->id), int32_t(tval[2]));
	}
	ui::xy_pair tf_diplomatic_influence_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_diplomatic_influence_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], 0ui16, tval[2]);
	}
	ui::xy_pair tf_diplomatic_influence_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_influence_value(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id), int32_t(tval[2]));
	}
	ui::xy_pair tf_diplomatic_influence_from_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)from_slot)->owner;
		if(owner)
			return tf_diplomatic_influence_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], 0ui16, tval[2]);
	}
	ui::xy_pair tf_pop_unemployment_nation(TRIGGER_DISPLAY_PARAMS) {
		auto type = trigger_payload(tval[4]).small.values.pop_type;
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, type));
			if(total_pop != 0)
				return compare_values(tval[0], 1.0f - float(ws.w.nation_s.nation_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_state(TRIGGER_DISPLAY_PARAMS) {
		auto type = trigger_payload(tval[4]).small.values.pop_type;
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, type));
			if(total_pop != 0)
				return compare_values(tval[0], 1.0f - float(ws.w.nation_s.state_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_province(TRIGGER_DISPLAY_PARAMS) {
		auto type = trigger_payload(tval[4]).small.values.pop_type;
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, type));
		if(total_pop != 0)
			return compare_values(tval[0], 1.0f - float(ws.w.province_s.province_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_pop(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(id)) {
			auto total_size = ws.w.population_s.pop_demographics.get(id, population::total_population_tag);
			if(total_size != 0)
				return compare_values(tval[0], 1.0f - float(ws.w.population_s.pop_demographics.get(id, population::total_employment_tag)) / float(total_size), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)this_slot)->type;
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, type));
			if(total_pop != 0)
				return compare_values(tval[0], 1.0f - float(ws.w.nation_s.nation_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_state_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)this_slot)->type;
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, type));
			if(total_pop != 0)
				return compare_values(tval[0], 1.0f - float(ws.w.nation_s.state_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_pop_unemployment_province_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto type = ((population::pop*)this_slot)->type;
		auto id = ((provinces::province_state*)primary_slot)->id;
		auto total_pop = ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, type));
		if(total_pop != 0)
			return compare_values(tval[0], 1.0f - float(ws.w.province_s.province_demographics.get(id, population::to_employment_demo_tag(ws, type))) / float(total_pop), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_relation_tag(TRIGGER_DISPLAY_PARAMS) {
		auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[3]).tag].holder;
		if(holder)
			return compare_values(tval[0], nations::get_relationship(ws, *((nations::nation*)primary_slot), holder->id), int32_t(trigger_payload(tval[2]).signed_value));
		else
			return compare_values(tval[0], 0i16, trigger_payload(tval[2]).signed_value);
	}
	ui::xy_pair tf_relation_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_relationship(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->id), int32_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_relation_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_relation_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], 0i16, trigger_payload(tval[2]).signed_value);
	}
	ui::xy_pair tf_relation_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], nations::get_relationship(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id), int32_t(trigger_payload(tval[2]).signed_value));
	}
	ui::xy_pair tf_relation_from_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)from_slot)->owner;
		if(owner)
			return tf_relation_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr);
		else
			return compare_values(tval[0], 0i16, trigger_payload(tval[2]).signed_value);
	}
	ui::xy_pair tf_check_variable(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], ws.w.nation_s.national_variables.get(id, triggers::trigger_payload(tval[3]).nat_var), read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_upper_house(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], float(ws.w.nation_s.upper_house.get(id, triggers::trigger_payload(tval[3]).small.values.ideology)) / 100.0f, read_float_from_payload(tval + 2));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_unemployment_by_type_nation(TRIGGER_DISPLAY_PARAMS) {
		return tf_pop_unemployment_nation(tval, ws, primary_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_unemployment_by_type_state(TRIGGER_DISPLAY_PARAMS) {
		return tf_pop_unemployment_state(tval, ws, primary_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_unemployment_by_type_province(TRIGGER_DISPLAY_PARAMS) {
		return tf_pop_unemployment_province(tval, ws, primary_slot, nullptr, nullptr, nullptr);
	}
	ui::xy_pair tf_unemployment_by_type_pop(TRIGGER_DISPLAY_PARAMS) {
		auto location = ((population::pop*)primary_slot)->location;
		if(is_valid_index(location)) {
			auto si = ws.w.province_s.province_state_container[location].state_instance;
			if(si)
				return tf_unemployment_by_type_state(tval, ws, si, nullptr, nullptr, nullptr);
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_party_loyalty_nation_province_id(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(provinces::province_tag(tval[2]), trigger_payload(tval[5]).small.values.ideology), read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_party_loyalty_from_nation_province_id(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(provinces::province_tag(tval[2]), trigger_payload(tval[5]).small.values.ideology), read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_party_loyalty_province_province_id(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(provinces::province_tag(tval[2]), trigger_payload(tval[5]).small.values.ideology), read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_party_loyalty_from_province_province_id(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(provinces::province_tag(tval[2]), trigger_payload(tval[5]).small.values.ideology), read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_party_loyalty_nation_from_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(((provinces::province_state*)from_slot)->id, trigger_payload(tval[4]).small.values.ideology), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_party_loyalty_generic(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(((provinces::province_state*)primary_slot)->id, trigger_payload(tval[4]).small.values.ideology), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_party_loyalty_from_nation_scope_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(((provinces::province_state*)primary_slot)->id, trigger_payload(tval[4]).small.values.ideology), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_party_loyalty_from_province_scope_province(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ws.w.province_s.party_loyalty.get(((provinces::province_state*)primary_slot)->id, trigger_payload(tval[4]).small.values.ideology), read_float_from_payload(tval + 2));
	}
	ui::xy_pair tf_can_build_in_province_railroad_no_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->railroad_level) +
			((provinces::province_state*)primary_slot)->modifier_values[modifiers::provincial_offsets::min_build_railroad] <
			((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_railroad],
			true);
	}
	ui::xy_pair tf_can_build_in_province_railroad_yes_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->railroad_level) +
			((provinces::province_state*)primary_slot)->modifier_values[modifiers::provincial_offsets::min_build_railroad] <
			((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_railroad],
			true);
	}
	ui::xy_pair tf_can_build_in_province_railroad_no_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->railroad_level) +
			((provinces::province_state*)primary_slot)->modifier_values[modifiers::provincial_offsets::min_build_railroad] <
			((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_railroad],
			true);
	}
	ui::xy_pair tf_can_build_in_province_railroad_yes_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->railroad_level) +
			((provinces::province_state*)primary_slot)->modifier_values[modifiers::provincial_offsets::min_build_railroad] <
			((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_railroad],
			true);
	}
	ui::xy_pair tf_can_build_in_province_fort_no_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->fort_level) <
			((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_fort],
			true);
	}
	ui::xy_pair tf_can_build_in_province_fort_yes_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->fort_level) <
			((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_fort],
			true);
	}
	ui::xy_pair tf_can_build_in_province_fort_no_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->fort_level) <
			((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_fort],
			true);
	}
	ui::xy_pair tf_can_build_in_province_fort_yes_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0],
			technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->fort_level) <
			((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_fort],
			true);
	}
	ui::xy_pair tf_can_build_in_province_naval_base_no_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		if(((provinces::province_state*)primary_slot)->naval_base_level != 0ui8) {
			return compare_values(tval[0],
				technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->naval_base_level) <
				((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_naval_base],
				true);
		} else if(auto si = ((provinces::province_state*)primary_slot)->state_instance; si) {
			return compare_values(tval[0],
				(si->flags & nations::state_instance::contains_naval_base) == 0 &&
				(ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].flags & provinces::province::coastal) != 0 &&
				((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_naval_base] >= technologies::tech_attribute_type(1),
				true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_can_build_in_province_naval_base_yes_limit_from_nation(TRIGGER_DISPLAY_PARAMS) {
		if(((provinces::province_state*)primary_slot)->naval_base_level != 0ui8) {
			return compare_values(tval[0],
				technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->naval_base_level) <
				((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_naval_base],
				true);
		} else if(auto si = ((provinces::province_state*)primary_slot)->state_instance; si) {
			return compare_values(tval[0],
				(si->flags & nations::state_instance::contains_naval_base) == 0 &&
				(ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].flags & provinces::province::coastal) != 0 &&
				((nations::nation*)from_slot)->tech_attributes[technologies::tech_offset::max_naval_base] >= technologies::tech_attribute_type(1),
				true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_can_build_in_province_naval_base_no_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		if(((provinces::province_state*)primary_slot)->naval_base_level != 0ui8) {
			return compare_values(tval[0],
				technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->naval_base_level) <
				((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_naval_base],
				true);
		} else if(auto si = ((provinces::province_state*)primary_slot)->state_instance; si) {
			return compare_values(tval[0],
				(si->flags & nations::state_instance::contains_naval_base) == 0 &&
				(ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].flags & provinces::province::coastal) != 0 &&
				((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_naval_base] >= technologies::tech_attribute_type(1),
				true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_can_build_in_province_naval_base_yes_limit_this_nation(TRIGGER_DISPLAY_PARAMS) {
		if(((provinces::province_state*)primary_slot)->naval_base_level != 0ui8) {
			return compare_values(tval[0],
				technologies::tech_attribute_type(((provinces::province_state*)primary_slot)->naval_base_level) <
				((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_naval_base],
				true);
		} else if(auto si = ((provinces::province_state*)primary_slot)->state_instance; si) {
			return compare_values(tval[0],
				(si->flags & nations::state_instance::contains_naval_base) == 0 &&
				(ws.s.province_m.province_container[((provinces::province_state*)primary_slot)->id].flags & provinces::province::coastal) != 0 &&
				((nations::nation*)this_slot)->tech_attributes[technologies::tech_offset::max_naval_base] >= technologies::tech_attribute_type(1),
				true);
		} else {
			return compare_values(tval[0], false, true);
		}
	}
	ui::xy_pair tf_can_build_railway_in_capital_yes_whole_state_yes_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_railway_in_capital_yes_whole_state_no_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_railway_in_capital_no_whole_state_yes_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_railway_in_capital_no_whole_state_no_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_fort_in_capital_yes_whole_state_yes_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_fort_in_capital_yes_whole_state_no_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_fort_in_capital_no_whole_state_yes_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_can_build_fort_in_capital_no_whole_state_no_limit(TRIGGER_DISPLAY_PARAMS) {
		// stub: virtually unused
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_work_available_nation(TRIGGER_DISPLAY_PARAMS) {
		auto count_workers = tval[1] - 1;
		auto id = ((nations::nation*)primary_slot)->id;

		if(!ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], false, true);

		for(int32_t i = 0; i < count_workers; ++i) {
			auto type = trigger_payload(tval[2 + i]).small.values.pop_type;
			if(ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, type)) != 0 &&
				ws.w.nation_s.nation_demographics.get(id, population::to_employment_demo_tag(ws, type)) == 0) {
				return compare_values(tval[0], false, true);
			}
		}
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_work_available_state(TRIGGER_DISPLAY_PARAMS) {
		auto count_workers = tval[1] - 1;
		auto id = ((nations::state_instance*)primary_slot)->id;

		if(!ws.w.nation_s.states.is_valid_index(id))
			return compare_values(tval[0], false, true);

		for(int32_t i = 0; i < count_workers; ++i) {
			auto type = trigger_payload(tval[2 + i]).small.values.pop_type;
			if(ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, type)) != 0 &&
				ws.w.nation_s.state_demographics.get(id, population::to_employment_demo_tag(ws, type)) == 0) {
				return compare_values(tval[0], false, true);
			}
		}
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_work_available_province(TRIGGER_DISPLAY_PARAMS) {
		auto count_workers = tval[1] - 1;
		auto id = ((provinces::province_state*)primary_slot)->id;

		for(int32_t i = 0; i < count_workers; ++i) {
			auto type = trigger_payload(tval[2 + i]).small.values.pop_type;
			if(ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, type)) != 0 &&
				ws.w.province_s.province_demographics.get(id, population::to_employment_demo_tag(ws, type)) == 0) {
				return compare_values(tval[0], false, true);
			}
		}
		return compare_values(tval[0], true, true);
	}
	ui::xy_pair tf_variable_ideology_name_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_ideology_name_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_ideology_name_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;

		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0],
				float(ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology))) / float(total_pop),
				read_float_from_payload(tval + 3));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_ideology_name_pop(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(id)) {
			auto total_pop = ws.w.population_s.pop_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.population_s.pop_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_issue_name_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_issue_name_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_issue_name_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;

		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0],
				float(ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option))) / float(total_pop),
				read_float_from_payload(tval + 3));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_issue_name_pop(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((population::pop*)primary_slot)->id;
		if(ws.w.population_s.pops.is_valid_index(id)) {
			auto total_pop = ws.w.population_s.pop_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.population_s.pop_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_issue_group_name_nation(TRIGGER_DISPLAY_PARAMS) {
		auto option = trigger_payload(tval[2]).small.values.option;
		auto issue = ws.s.issues_m.options[option].parent_issue;

		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], ws.w.nation_s.active_issue_options.get(id, issue) == option, true);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_variable_issue_group_name_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)primary_slot)->owner;
		if(owner)
			return tf_variable_issue_group_name_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_variable_issue_group_name_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)primary_slot)->owner;
		if(owner)
			return tf_variable_issue_group_name_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_variable_issue_group_name_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
		if(owner)
			return tf_variable_issue_group_name_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_variable_pop_type_name_nation(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.nation_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.nation_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_pop_type_name_state(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::state_instance*)primary_slot)->id;
		if(ws.w.nation_s.states.is_valid_index(id)) {
			auto total_pop = ws.w.nation_s.state_demographics.get(id, population::total_population_tag);
			if(total_pop != 0)
				return compare_values(tval[0],
					float(ws.w.nation_s.state_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type))) / float(total_pop),
					read_float_from_payload(tval + 3));
		}
		return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_pop_type_name_province(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((provinces::province_state*)primary_slot)->id;

		auto total_pop = ws.w.province_s.province_demographics.get(id, population::total_population_tag);
		if(total_pop != 0)
			return compare_values(tval[0],
				float(ws.w.province_s.province_demographics.get(id, population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.pop_type))) / float(total_pop),
				read_float_from_payload(tval + 3));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_pop_type_name_pop(TRIGGER_DISPLAY_PARAMS) {
		if(((population::pop*)primary_slot)->type == trigger_payload(tval[2]).small.values.pop_type)
			return compare_values(tval[0], 1.0f, read_float_from_payload(tval + 3));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_variable_good_name(TRIGGER_DISPLAY_PARAMS) {
		auto id = ((nations::nation*)primary_slot)->id;
		if(ws.w.nation_s.nations.is_valid_index(id))
			return compare_values(tval[0], float(ws.w.nation_s.national_stockpiles.get(id, trigger_payload(tval[2]).small.values.good)), read_float_from_payload(tval + 3));
		else
			return compare_values(tval[0], 0.0f, read_float_from_payload(tval + 3));
	}
	ui::xy_pair tf_religion_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->national_religion == trigger_payload(tval[2]).small.values.religion, true);
	}
	ui::xy_pair tf_religion_nation_reb(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->national_religion == rebel_slot->religion, true);
	}
	ui::xy_pair tf_religion_nation_from_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->national_religion == ((nations::nation*)from_slot)->national_religion, true);
	}
	ui::xy_pair tf_religion_nation_this_nation(TRIGGER_DISPLAY_PARAMS) {
		return compare_values(tval[0], ((nations::nation*)primary_slot)->national_religion == ((nations::nation*)this_slot)->national_religion, true);
	}
	ui::xy_pair tf_religion_nation_this_state(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((nations::state_instance*)this_slot)->owner;
		if(owner)
			return tf_religion_nation_this_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_religion_nation_this_province(TRIGGER_DISPLAY_PARAMS) {
		auto owner = ((provinces::province_state*)this_slot)->owner;
		if(owner)
			return tf_religion_nation_this_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}
	ui::xy_pair tf_religion_nation_this_pop(TRIGGER_DISPLAY_PARAMS) {
		auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
		if(owner)
			return tf_religion_nation_this_nation(tval, ws, owner, nullptr, nullptr, nullptr);
		else
			return compare_values(tval[0], false, true);
	}

	ui::xy_pair make_trigger_description(
		world_state& ws,
		ui::tagged_gui_object container,
		ui::xy_pair cursor_in,
		ui::unlimited_line_manager& lm,
		ui::text_format const& fmt,
		uint16_t const* tval,
		void* primary_slot,
		void* this_slot,
		void* from_slot,
		population::rebel_faction* rebel_slot
	) {
		if(primary_slot) {
			if(triggers::test_trigger(tval, ws, primary_slot, this_slot, from_slot, rebel_slot)) {
				ui::text_format local_fmt{ui::text_color::green, fmt.font_handle, fmt.font_size};
				cursor_in = ui::text_chunk_to_instances(ws.s.gui_m, ws.w.gui_m, vector_backed_string<char16_t>(u"\u2713 "), container, cursor_in, local_fmt, lm);
			} else {
				ui::text_format local_fmt{ ui::text_color::red, fmt.font_handle, fmt.font_size };
				cursor_in = ui::text_chunk_to_instances(ws.s.gui_m, ws.w.gui_m, vector_backed_string<char16_t>(u"\u274C "), container, cursor_in, local_fmt, lm);
			}
		}
		//cursor_in = ui::advance_cursor_to_newline(cursor_in, ws.s.gui_m, fmt);
		//lm.finish_current_line();
		return cursor_in;
	}
}
