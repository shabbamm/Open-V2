#pragma once
#include <stdint.h>
#include <variant>
#include <functional>
#include <map>
#include "common\\common.h"
#include "common\\shared_tags.h"
#include <shared_mutex>

namespace ui {
	class gui_object;
}

class world_state;

namespace text_data {
	enum class text_color {
		black, white, red, green, yellow, unspecified, light_blue, dark_blue
	};
	enum class alignment {
		left, right, center
	};
	enum class value_type {
		error_no_matching_value, action, active, actor, adj, against, agressor, allowed, amount,
		anyprovince, army, army_name, astate, attacker, attunit, bac, badboy, badword, base,
		base_percentage, bat, bld, bon, bonus, brig, bud, building, buy, cap, capital, cash,
		casus, cat, category, cb_target_name, cb_target_name_adj, chan, chance, change, commander,
		con, construction, continentname, cost, count, countries, country, country1, country2,
		countryadj, countryculture, countryname, country_adj, creator, credits, crisisarea,
		crisisattacker, crisisdefender, crisistaker, crisistaker_adj, crisistarget, crisistarget_adj,
		cul, culture, culture_group_union, curr, current, d, date, date_long_0, date_long_1,
		date_short_0, day, days, defender, defunit, desc, dest, details, direction, dist, effect,
		effects, emilist, employees, employee_max, enemy, engineermaxunits, engineerunits, escorts,
		event, eventdesc, exc, exp, faction, factory, first, focus, folder, vtype_for, fraction,
		vtype_friend, from, fromcountry, fromcountry_adj, fromprovince, fromruler, funds, goal,
		good, goods, gov, government, govt, gp, gp_adj, group, high_tax, hit, home, hull, iamount,
		icountry, ide, ideology, ilocation, immlist, impact, iname, income, indep, inf, infamy,
		input, inv, invention, invested, invested_in_us_message, issue, law, leader, lev, level,
		levels, lim, limit, list, literacy, loc, local, location, lord, lose, low_tax, lvl, m,
		max, maxloan, men, messenger, mil, militancy, min, monarchtitle, money, month, months,
		movement, much, name, nation, nationalvalue, natives, navy, navy_name, need, needed,
		negative, neut, vtype_new, newcountry, nf, now, num, number, numfactories, numspecialfactories,
		odds, old, oldcountry, vtype_operator, opinion, opposing_army, opposing_navy, opressor,
		opt, optimal, option, order, org, organisation, other, otherresult, ourcapital, ourcountry,
		ourcountry_adj, our_lead, our_num, our_res, output, overlord, param, party, passive, pay,
		pen, penalty, per, perc, perc2, percent, percentage, player, policy, pop, poptype,
		popularity, position, positive, power, pres, prescence, prestige, produced, producer,
		prog, progress, proj, prov, provinceculture, provincename, provincereligion,
		provinces, provs, province, rank, rate, rec, recipient, reconmaxunits, reconunits, reform,
		region, rel, relation, req, reqlevel, required, resource, result, rstate, rule, runs, scr,
		sea, second, second_country, self, sell, setting, ships, size, skill, source, speed,
		spheremaster, state, statename, str, strata, strings_list_4, string_0_0, string_0_1,
		string_0_2, string_0_3, string_0_4, string_9_0, sub, table, tag, tag0_0, tag_0_0, tag_0_0_,
		tag_0_0_adj, tag_0_0_upper, tag_0_1, tag_0_1_adj, tag_0_1_upper, tag_0_2, tag_0_2_adj,
		tag_0_3, tag_0_3_adj, tag_1_0, tag_2_0, tag_2_0_upper, tag_3_0, tag_3_0_upper, target,
		targetlist, target_country, tech, temperature, terms, terrain, terrainmod, text, theirlost,
		theirnum, theirship, their_lead, their_num, their_res, them, third, threat, time, title, to,
		tot, totalemi, totalimm, tra, truth, type, unemployed, vtype_union, union_adj, unit, units,
		until, usloss, usnum, val, value, value_int1, value_int_0_0, value_int_0_1, value_int_0_2,
		value_int_0_3, value_int_0_4, verb, versus, war, wargoal, we, what, where, which, who, winner,
		x, y, year, years, chief_of_navy, chief_of_staff, control, head_of_government, owner, playername
	};

	struct line_break {};
	struct color_change {
		text_color color = text_color::unspecified;
	};
	struct value_placeholder {
		value_type value = value_type::error_no_matching_value;
	};
	struct text_chunk {
		uint32_t offset = 0;
		uint16_t length = 0;

		operator vector_backed_string<char16_t>() const { return vector_backed_string<char16_t>(offset, length); }
	};
	using text_component = std::variant<line_break, color_change, value_placeholder, text_chunk>;

	struct text_sequence {
		uint16_t starting_component = 0;
		uint16_t component_count = 0;
	};

	struct text_sequences {
		std::vector<char16_t> text_data;
		std::vector<char> key_data;
		std::vector<text_component> all_components;
		tagged_vector<text_sequence, text_tag> all_sequences;

		boost::container::flat_map<vector_backed_string<char>, text_tag, vector_backed_string_less_ci> key_to_sequence_map;

		mutable std::shared_mutex text_data_mutex;

		text_sequences() : key_to_sequence_map(vector_backed_string_less_ci(key_data)) {}
	};


	struct percent {
		float value;
	};
	struct integer {
		int32_t value;
		integer(int32_t v) : value(v) {}
		integer(uint32_t v) : value(int32_t(v)) {}
		integer(float v) : value(int32_t(v + 0.5f)) {}
	};
	struct exact_integer {
		int32_t value;
		exact_integer(int32_t v) : value(v) {}
		exact_integer(uint32_t v) : value(int32_t(v)) {}
		exact_integer(float v) : value(int32_t(v + 0.5f)) {}
	};
	struct fp_three_places {
		float value;
	};
	struct fp_two_places {
		float value;
	};
	struct fp_one_place {
		float value;
	};
	struct percent_fp_one_place {
		float value;
	};
	struct currency {
		float value;
	};

	using replacement_data_variant = std::variant<std::monostate, text_data::text_tag, char16_t const*, 
		percent, integer, exact_integer, fp_three_places, fp_two_places, fp_one_place, percent_fp_one_place, currency, date_tag>;

	struct text_replacement {
		value_type key = value_type::value;
		text_color new_color = text_color::unspecified;
		replacement_data_variant data;
		std::function<void(world_state&)> click_function;

		text_replacement() = default;
		text_replacement(value_type k, replacement_data_variant dat) : key(k), data(dat) {}
		template<typename F>
		text_replacement(value_type k, replacement_data_variant dat, F const& fun) : key(k), data(dat), click_function(fun) {}
		template<typename F>
		text_replacement(value_type k, replacement_data_variant dat, F const& fun, text_color c) : key(k), new_color(c), data(dat), click_function(fun) {}
	};

	void add_win1250_text_to_container(text_sequences& container, const char* s, const char *e);
	void add_utf8_text_to_container(text_sequences& container, const char* s, const char *e);
	bool is_win1250_section(const char* start, const char* end);
	bool is_utf8_section(const char* start, const char* end);
	void add_utf8_sequence(text_sequences& container, std::map<vector_backed_string<char>, text_tag, vector_backed_string_less_ci>& temp_map, const char* key_start, const char* key_end, const char* seq_start, const char* seq_end);
	void add_win1250_sequence(text_sequences& container, std::map<vector_backed_string<char>, text_tag, vector_backed_string_less_ci>& temp_map, const char* key_start, const char* key_end, const char* seq_start, const char* seq_end);
	value_type value_type_from_name(const char* start, const char* end);
	const char16_t* name_from_value_type(value_type v);

	const text_replacement* find_replacement(value_placeholder placeholder, const text_replacement* candidates, uint32_t count);

	std::pair<int32_t, int32_t> align_in_bounds(text_data::alignment align, int32_t width, int32_t height, int32_t bound_x, int32_t bound_y);

	text_tag get_text_handle(text_data::text_sequences& container, const char* key_start, const char* key_end);
	text_tag get_thread_safe_text_handle(text_data::text_sequences& container, const char* key_start, const char* key_end);
	text_tag get_existing_text_handle(const text_data::text_sequences& container, const char* key_start, const char* key_end);
	text_tag get_thread_safe_existing_text_handle(const text_data::text_sequences& container, const char* key_start, const char* key_end);

	template<size_t N>
	text_tag get_thread_safe_existing_text_handle(const text_data::text_sequences& container, const char(&t)[N]) {
		return get_thread_safe_existing_text_handle(container, t, t + N - 1);
	}
	template<size_t N>
	text_tag get_existing_text_handle(const text_data::text_sequences& container, const char(&t)[N]) {
		return get_existing_text_handle(container, t, t + N - 1);
	}
	template<size_t N>
	text_tag get_thread_safe_text_handle(text_data::text_sequences& container, const char(&t)[N]) {
		return get_thread_safe_text_handle(container, t, t + N - 1);
	}
	template<size_t N>
	text_tag get_text_handle(text_data::text_sequences& container, const char(&t)[N]) {
		return get_text_handle(container, t, t + N - 1);
	}

	std::u16string to_string(const text_sequences& container, text_data::text_tag tag);
	vector_backed_string<char16_t> text_tag_to_backing(const text_sequences& container, text_data::text_tag tag);
	bool contains_case_insensitive(const text_sequences& container, text_data::text_tag tag, char16_t const* sequence, int32_t sequence_length);
}

