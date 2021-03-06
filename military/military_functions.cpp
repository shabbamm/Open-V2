#include "common\\common.h"
#include "military_functions.h"
#include "world_state\\world_state.h"
#include <random>
#include "nations\\nations_functions.hpp"
#include "provinces\\province_functions.hpp"
#include "economy\\economy_functions.h"
#include "world_state\\messages.h"
#include "military_internals.hpp"

namespace military {
	void init_military_state(world_state& ws) {
		
	}

	void reset_state(military_state& s) {
		s.leader_arrays.reset();
		s.army_arrays.reset();
		s.orders_arrays.reset();
		s.fleet_arrays.reset();
		s.war_arrays.reset();
		s.war_goal_arrays.reset();
		s.fleet_presence_arrays.reset();
		s.naval_control_arrays.reset();
		s.cb_arrays.reset();
	}

	inline leader_tag new_leader_fn(world_state& ws) {
		return ws.w.military_s.leaders.get_new();
	}

	leader_tag make_empty_leader(world_state& ws, cultures::culture_tag culture, bool is_general) {
		return internal_make_empty_leader<world_state,
			new_leader_fn,
			internal_get_leader_picture<world_state>,
			internal_get_leader_name<world_state>>(ws, culture, is_general);
	}

	leader_tag make_auto_leader(world_state& ws, cultures::culture_tag culture, bool is_general, date_tag creation_date) {
		return internal_make_auto_leader<world_state, make_empty_leader, calculate_leader_traits>(ws, culture, is_general, creation_date);
	}

	void calculate_leader_traits(world_state& ws, leader_tag l) {
		internal_calculate_leader_traits(ws, l);
	}

	inline army_tag new_army_fn(world_state& ws) {
		return ws.w.military_s.armies.get_new();
	}

	army_tag make_army(world_state& ws, nations::country_tag n, provinces::province_tag location) {
		return internal_make_army<world_state, new_army_fn>(ws, n, location);
	}

	inline fleet_tag new_fleet_fn(world_state& ws) {
		return ws.w.military_s.fleets.get_new();
	}


	fleet_tag make_fleet(world_state& ws, nations::country_tag n, provinces::province_tag location) {
		return internal_make_fleet<world_state, new_fleet_fn>(ws, n, location);
	}

	bool in_war_with(world_state const& ws, nations::country_tag this_nation, nations::country_tag nation_with) {
		return internal_in_war_with(ws, this_nation, nation_with);
	}

	bool in_war_against(world_state const& ws, nations::country_tag this_nation, nations::country_tag nation_against) {
		return internal_in_war_against(ws, this_nation, nation_against);
	}

	inline bool check_war_tag(world_state const& ws, war_tag wid) {
		return ws.w.military_s.wars.is_valid_index(wid);
	}

	void update_at_war_with_and_against(world_state& ws, nations::country_tag this_nation) {
		internal_update_at_war_with_and_against<world_state, check_war_tag>(ws, this_nation);
	}

	inline bool generic_test_trigger(world_state const& ws, triggers::trigger_tag t, triggers::const_parameter a, triggers::const_parameter b, triggers::const_parameter c) {
		return triggers::test_trigger(ws.s.trigger_m.trigger_data.data() + to_index(t), ws, a, b, c);
	}

	bool can_use_cb_against(world_state const& ws, nations::country_tag nation_by, nations::country_tag nation_target) {
		return internal_can_use_cb_against<world_state, generic_test_trigger>(ws, nation_by, nation_target);
	}

	inline bool internal_wrapper(world_state const& ws, nations::country_tag nation_by, nations::country_tag nation_target, cb_type_tag c) {
		return internal_test_cb_conditions<world_state, nations::owns_releasable_core, generic_test_trigger>(ws, nation_by, nation_target, c);
	}

	bool is_cb_construction_valid_against(world_state const& ws, cb_type_tag cb, nations::country_tag nation_by, nations::country_tag nation_target) {
		return internal_is_cb_construction_valid_against<
			world_state,
			generic_test_trigger,
			internal_wrapper>(
				ws, cb, nation_by, nation_target);
	}

	inline auto get_player(world_state const& ws) {
		return ws.w.local_player_nation;
	}

	void init_player_cb_state(world_state& ws) {
		internal_init_player_cb_state<world_state, nations::nation_exists<nations::country_tag>, get_player, generic_test_trigger>(ws);
	}

	void post_cb_change_message(bool new_state, world_state& ws, nations::country_tag player, nations::country_tag n, cb_type_tag cb_id) {
		if(new_state)
			messages::acquired_cb(ws, player, n, cb_id);
		else
			messages::lost_cb(ws, player, n, cb_id);
	}

	void update_player_cb_state(world_state& ws) {
		internal_update_player_cb_state<world_state, nations::nation_exists<nations::country_tag>, get_player, generic_test_trigger, post_cb_change_message>(ws);
	}

	bool has_units_in_province(world_state const& ws, nations::country_tag this_nation, provinces::province_tag ps) {
		return internal_has_units_in_province(ws, this_nation, ps);
	}

	inline float regiment_size(world_state const& ws) {
		return ws.s.modifiers_m.global_defines.pop_size_per_regiment;
	}

	uint32_t total_units_in_province(world_state const& ws, provinces::province_tag ps) {
		return internal_total_units_in_province<world_state, regiment_size>(ws, ps);
	}

	uint32_t total_active_divisions(world_state const& ws, nations::country_tag this_nation) {
		return internal_total_active_divisions<world_state, regiment_size>(ws, this_nation);
	}
	uint32_t total_active_ships(world_state const& ws, nations::country_tag this_nation) {
		return internal_total_active_ships(ws, this_nation);
	}

	population::demo_tag soldier_demo_tag(world_state const& ws) {
		return population::to_demo_tag(ws, ws.s.population_m.soldier);
	}

	float recruited_pop_fraction(world_state const& ws, nations::country_tag this_nation) {
		return internal_recruited_pop_fraction<world_state, soldier_demo_tag>(ws, this_nation);
	}

	bool has_named_leader(world_state const&, nations::country_tag, text_data::text_tag) {
		return true;
	}

	float get_warscore(world_state const& ws, war_tag wr) {
		return ws.get<war::current_war_score>(wr);
	}

	void empty_left_war_message(world_state& ws, war_tag this_war, nations::country_tag to_remove) {

	}

	void left_war_message(world_state& ws, war_tag this_war, nations::country_tag to_remove) {

	}

	void silent_remove_from_war(world_state& ws, war_tag this_war, nations::country_tag to_remove) {
		internal_remove_from_war<world_state, empty_left_war_message>(ws, this_war, to_remove);
	}

	void remove_from_war(world_state& ws, war_tag this_war, nations::country_tag to_remove) {
		internal_remove_from_war<world_state, left_war_message>(ws, this_war, to_remove);
	}

	void add_to_war_message(world_state& ws, war_tag this_war, bool as_attacker, nations::country_tag to_add) {

	}

	void add_to_war(world_state& ws, war_tag this_war, bool attacker, nations::country_tag to_add) {
		internal_add_to_war<world_state, add_to_war_message>(ws, this_war, attacker, to_add);
	}

	bool is_target_of_war_goal(world_state const& ws, war_tag this_war, nations::country_tag target) {
		return internal_is_target_of_war_goal(ws, this_war, target);
	}

	void release_army(world_state& ws, army_tag a) {
		ws.w.military_s.armies.release(a);
	}

	void destroy_army(world_state& ws, army_tag a) {
		internal_destroy_army<world_state, release_army, true>(ws, a);
	}

	void release_fleet(world_state& ws, fleet_tag f) {
		ws.w.military_s.fleets.release(f);
	}

	void destroy_fleet(world_state& ws, fleet_tag f, nations::country_tag owner) {
		internal_destroy_fleet<world_state, release_fleet, true>(ws, f, owner);
	}

	void partial_destroy_fleet(world_state& ws, fleet_tag f) {
		internal_destroy_fleet<world_state, release_fleet, false>(ws, f, nations::country_tag());
	}

	void partial_destroy_army(world_state& ws, army_tag a) {
		internal_destroy_army<world_state, release_army, false>(ws, a);
	}

	void release_orders(world_state& ws, army_orders_tag o) {
		ws.w.military_s.army_orders.release(o);
	}

	void destroy_orders(world_state& ws, army_orders_tag o, nations::country_tag owner) {
		internal_destroy_orders<world_state, release_orders, true>(ws, o, owner);
	}

	void partial_destroy_orders(world_state& ws, army_orders_tag o) {
		internal_destroy_orders<world_state, release_orders, false>(ws, o, nations::country_tag());
	}

	void release_hq(world_state& ws, strategic_hq_tag o) {
		ws.w.military_s.strategic_hqs.release(o);
	}

	void destroy_hq(world_state& ws, strategic_hq_tag o, nations::country_tag owner) {
		internal_destroy_hq<world_state, release_hq, destroy_army, true>(ws, o, owner);
	}
	void partial_destroy_hq(world_state& ws, strategic_hq_tag o) {
		internal_destroy_hq<world_state, release_hq, destroy_army, false>(ws, o, nations::country_tag());
	}

	void release_leader(world_state& ws, leader_tag o) {
		ws.w.military_s.leaders.release(o);
	}

	void destroy_admiral(world_state& ws, leader_tag l, nations::country_tag owner) {
		internal_destroy_admiral<world_state, release_leader>(ws, l, owner);
	}

	void destroy_general(world_state& ws, leader_tag l, nations::country_tag owner) {
		internal_destroy_general<world_state, release_leader>(ws, l, owner);
	}

	void rebuild_fleet_presence(world_state&, nations::country_tag) {
		//stub
	}

	void release_war(world_state& ws, war_tag o) {
		ws.w.military_s.wars.release(o);
	}

	void destroy_war(world_state& ws, war_tag this_war) {
		internal_destroy_war<world_state, release_war>(ws, this_war);
	}

	war_tag get_war_between(world_state& ws, nations::country_tag a, nations::country_tag b) {
		return internal_get_war_between(ws, a, b);
	}

	date_tag get_date(world_state const& ws) {
		return ws.w.current_date;
	}
	war_tag allocate_war(world_state& ws) {
		return ws.w.military_s.wars.get_new();
	}

	war_tag create_war(world_state& ws, nations::country_tag attacker, nations::country_tag defender, bool call_willing_attacker_allies) {
		return internal_create_war<world_state, allocate_war, get_date>(ws, attacker, defender, call_willing_attacker_allies);
	}


	float single_state_war_score(world_state const& ws, nations::state_tag si, float owner_total_pop) {
		return internal_single_state_war_score<
			world_state,
			economy::count_factories_in_state,
			nations::is_colonial_or_protectorate<nations::state_tag>>(
				ws, si, owner_total_pop);
	}

	auto const& setting_struct(world_state const& ws) {
		return ws.s.modifiers_m.global_defines;
	}

	float calculate_base_war_score_cost(world_state const& ws, war_goal const& wg) {
		return internal_calculate_base_war_score_cost<
			world_state,
			setting_struct,
			nations::nation_exists<nations::country_tag>,
			single_state_war_score,
			generic_test_trigger>(
				ws, wg);
	}

	float total_attacker_demands_war_score(world_state const& ws, war_tag w) {
		return internal_total_attacker_demands_war_score<world_state, calculate_base_war_score_cost>(ws, w);
	}
	float total_defender_demands_war_score(world_state const& ws, war_tag w) {
		return internal_total_defender_demands_war_score<world_state, calculate_base_war_score_cost>(ws, w);
	}

	float base_cb_infamy(world_state const& ws, cb_type_tag cb) {
		return internal_base_cb_infamy(ws, cb);
	}

	float daily_cb_progress(world_state const& ws, nations::country_tag n, cb_type_tag type) {
		return internal_daily_cb_progress<world_state, setting_struct>(ws, n, type);
	}

	void update_cb_construction(world_state& ws) {
		internal_update_cb_construction<
			world_state,
			setting_struct,
			is_cb_construction_valid_against,
			messages::acquired_cb,
			messages::cb_detected,
			messages::cb_construction_invalid>(ws);
	}

	auto change_army_location(world_state& ws, army_tag t, provinces::province_tag new_location) -> void {
		internal_change_army_location(ws, t, new_location);
	}

	void init_strat_hq_from_province(world_state& ws, nations::country_tag n, provinces::province_tag c) {
		auto prov_set = provinces::get_connected_owned_provinces(ws, c);
		auto const cap = ws.get<nation::current_capital>(n);

		if(prov_set.size() > 1 || cap == c) {
			auto new_hq = ws.w.military_s.strategic_hqs.get_new();
			ws.add_item(ws.get<nation::strategic_hqs>(n), new_hq);

			float s_count = 0.0f;
			float t_count = 0.0f;

			auto best_prov = c;
			for(auto p : prov_set) {
				ws.set<province_state::strat_hq>(p, new_hq);
				auto const s = ws.get<province::demographics>(p, population::to_demo_tag(ws, ws.s.population_m.soldier));
				auto const t = ws.get<province_state::total_population>(p);
				s_count += s;
				t_count += (t - s);

				if(t > ws.get<province_state::total_population>(best_prov))
					best_prov = p;

			}

			ws.set<strategic_hq::location>(new_hq, cap == c ? cap : best_prov);
			ws.set<strategic_hq::total_soldier_pops>(new_hq, s_count);
			ws.set<strategic_hq::total_non_soldier_pops>(new_hq, t_count);
			ws.set<strategic_hq::reserve_soldiers>(new_hq, s_count);
		}
	}

	void init_strategic_hqs(world_state& ws) {
		ws.w.nation_s.nations.for_each([&ws](nations::country_tag n) {
			if(auto c = ws.get<nation::current_capital>(n); c) {
				init_strat_hq_from_province(ws, n, c);
				nations::for_each_province(ws, n, [&ws, n](provinces::province_tag p) {
					if(!is_valid_index(ws.get<province_state::strat_hq>(p))) {
						init_strat_hq_from_province(ws, n, p);
					}
				});

				auto cap_hq = ws.get<province_state::strat_hq>(c);

				auto armies = ws.get_range(ws.get<nation::armies>(n));
				army_tag* local_copy = (army_tag*)_alloca((armies.second - armies.first) * sizeof(army_tag));
				memcpy(local_copy, armies.first, (armies.second - armies.first) * sizeof(army_tag));

				for(int32_t i = 0; i < (armies.second - armies.first); ++i) {
					if(auto local_hq = ws.get<province_state::strat_hq>(ws.get<army::location>(local_copy[i]));
						local_hq && ws.get<strategic_hq::reserve_soldiers>(local_hq) >= ws.get<army::target_soldiers>(local_copy[i])) {
						ws.get<strategic_hq::reserve_soldiers>(local_hq) -= ws.get<army::target_soldiers>(local_copy[i]);
						ws.set<army::current_soldiers>(local_copy[i], ws.get<army::target_soldiers>(local_copy[i]));
						ws.set<army::hq>(local_copy[i], local_hq);
						ws.add_item(ws.get<strategic_hq::army_set>(local_hq), local_copy[i]);
					} else if(ws.get<strategic_hq::reserve_soldiers>(cap_hq) >= ws.get<army::target_soldiers>(local_copy[i])) {
						ws.get<strategic_hq::reserve_soldiers>(cap_hq) -= ws.get<army::target_soldiers>(local_copy[i]);
						ws.set<army::current_soldiers>(local_copy[i], ws.get<army::target_soldiers>(local_copy[i]));
						ws.set<army::hq>(local_copy[i], cap_hq);
						ws.add_item(ws.get<strategic_hq::army_set>(cap_hq), local_copy[i]);
					} else {
						destroy_army(ws, local_copy[i]);
					}
				}
			} else {
				auto armies = ws.get_range(ws.get<nation::armies>(n));
				army_tag* local_copy = (army_tag*)_alloca((armies.second - armies.first) * sizeof(army_tag));
				memcpy(local_copy, armies.first, (armies.second - armies.first) * sizeof(army_tag));

				for(int32_t i = 0; i < (armies.second - armies.first); ++i) {
					destroy_army(ws, local_copy[i]);
				}
			}
		});
	}

	constexpr float distance_limit = 0.99877f; //cos(100.0 * 6.2831853071f / 40'075.0f);

	auto province_in_range_of_army(world_state const& ws, army_tag t, provinces::province_tag p) -> bool {
		auto location = ws.get<army::location>(t);
		if(location == p)
			return true;
		if(provinces::provinces_are_same_type_adjacent(ws, location, p))
			return true;

		auto owner = ws.get<army::owner>(t);
		if(ws.get<province::centroid>(location).dot(ws.get<province::centroid>(p)) >= distance_limit) {
			return provinces::provinces_are_connected(ws, p, location, distance_limit, [&ws, owner](provinces::province_tag q){
				auto const c = ws.get<province_state::controller>(q);
				return owner == c || has_military_access_with(ws, owner, c);
			});
		}
		
		return false;
	}

	auto move_troops_to_border(world_state& ws, military::border_information_tag b) -> void {
		
	}

	auto move_troops_to_borders(world_state& ws) -> void {
		concurrency::combinable<moveable_concurrent_cache_aligned_buffer<float, military::strategic_hq_tag, true, province::container_size>>
			reserve_adjustment;

		ws.w.military_s.borders.parallel_for_each([&ws, &reserve_adjustment](military::border_information_tag b) {
			auto hq_info_range = ws.get_range(ws.get<border_information::hqs>(b));

			float total_new_to_allocate = 0.0f;
			auto& reserve_adjust = reserve_adjustment.local();

			for(auto& info : hq_info_range) {
				auto const fp_target = float(info.target_percent) / 100.0f;
				auto const old_hq_reserve = ws.get<strategic_hq::reserve_soldiers>(info.id);
				auto const total_available = ws.get<strategic_hq::allocated_soldiers>(info.id) + old_hq_reserve;

				if(info.active_soldiers > total_available * fp_target) {
					reserve_adjust[info.id] += info.active_soldiers - total_available * fp_target;
					info.active_soldiers = total_available * fp_target;
				} else {
					total_new_to_allocate += std::min(total_available * fp_target - info.active_soldiers, old_hq_reserve);
				}
			}
		});

		reserve_adjustment.combine_each([&ws](moveable_concurrent_cache_aligned_buffer<float, military::strategic_hq_tag, true, province::container_size> const& buffer) {
			ve::execute_serial_fast<military::strategic_hq_tag>(ws.w.military_s.strategic_hqs.vector_size(), [
				rrow = ws.get_row<strategic_hq::reserve_soldiers>(),
				arow = ws.get_row<strategic_hq::allocated_soldiers>(),
				drow = buffer.view()
			](auto exec) {
					auto const diff = ve::load(exec, drow);
					ve::store(exec, rrow, ve::load(exec, rrow) + diff);
					ve::store(exec, arow, ve::load(exec, arow) - diff);
				});
		});
	}

	auto fill_hq() -> void {

	}

	auto fill_hqs() -> void {

	}

	auto update_national_borders(world_state& ws, nations::country_tag n, uint16_t* counts, provinces::province_tag* rep_provinces) -> void {
		auto control_set = ws.get_range(ws.get<nation::controlled_provinces>(n));

		auto const nation_count = ws.size<nation::name>();
		auto const nation_2byte_count = ve::to_2byte_vector_size(nation_count);

		std::vector<Eigen::Vector3f, concurrent_allocator<Eigen::Vector3f>> centroids(nation_count);
		std::vector<float, concurrent_allocator<float>> best_distance(nation_count);

		std::fill_n(counts, nation_2byte_count, 0ui16);
		std::fill_n(best_distance.data(), nation_count, -1.0f);

		for(auto p : control_set) {
			auto const adj_row = ws.get_row<province::same_type_adjacency>(p);
			auto const adj_size = ws.size<province::same_type_adjacency>(p);
			for(int32_t i = 0; i < adj_size; ++i) {
				auto adj_controller = ws.get<province_state::controller>(adj_row[i]);
				if(adj_controller && adj_controller != n) {
					counts[to_index(adj_controller)] += 1ui16;
					centroids[to_index(adj_controller)] += ws.get<province::centroid>(p);
				}
			}
		}

		for(int32_t j = 0; j < nation_count; ++j) {
			if(counts[j] != 0) {
				centroids[j].normalize();
			}
		}

		for(auto p : control_set) {
			auto const adj_row = ws.get_row<province::same_type_adjacency>(p);
			auto const adj_size = ws.size<province::same_type_adjacency>(p);
			for(int32_t i = 0; i < adj_size; ++i) {
				auto adj_controller = ws.get<province_state::controller>(adj_row[i]);
				if(adj_controller && adj_controller != n) {
					float const distance = ws.get<province::centroid>(p).dot(centroids[to_index(adj_row[i])]);
					if(distance > best_distance[to_index(adj_row[i])]) {
						best_distance[to_index(adj_row[i])] = distance;
						rep_provinces[to_index(adj_controller)] = adj_row[i];
					}
				}
			}
		}
	}
}
