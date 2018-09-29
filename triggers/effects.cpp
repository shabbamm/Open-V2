#include "effects.h"
#include "codes.h"
#include <random>
#include "world_state\\world_state.h"
#include "nations\\nations_functions.hpp"
#include "provinces\\province_functions.hpp"
#include "population\\population_function.h"
#include "governments\\governments_functions.h"
#include "cultures\\cultures_functions.h"
#include "military\\military_functions.h"
#include "modifiers\\modifier_functions.h"
#include "issues\\issues_functions.h"
#include "technologies\\technologies_functions.h"
#include "economy\\economy_functions.h"

namespace triggers {
	int32_t get_effect_payload_size(const uint16_t* data) {
		const auto payload_mask = uint16_t(-int16_t((effect_codes::no_payload & data[0]) == 0));
		return payload_mask & data[1];
	}
	int32_t effect_scope_data_payload(uint16_t code) {
		const auto masked_code = code & effect_codes::code_mask;
		if ((masked_code == effect_codes::tag_scope) |
			(masked_code == effect_codes::integer_scope) |
			(masked_code == effect_codes::pop_type_scope_nation) |
			(masked_code == effect_codes::pop_type_scope_state) |
			(masked_code == effect_codes::pop_type_scope_province) |
			(masked_code == effect_codes::region_scope) |
			(masked_code == effect_codes::random_scope))
			return 1 + ((code & effect_codes::scope_has_limit) != 0);
		return 0 + ((code & effect_codes::scope_has_limit) != 0);
	}

	int32_t get_random_list_effect_chances_sum(const uint16_t* data) {
		const auto source_size = 1 + get_effect_payload_size(data);
		int32_t sum = 0;

		auto sub_units_start = data + 3;
		while (sub_units_start < data + source_size) {
			sum += sub_units_start[0];
			sub_units_start += 2 + get_effect_payload_size(sub_units_start + 1);
		}

		return sum;
	}

	const uint16_t* get_random_list_effect(const uint16_t* data, int32_t random_value) {
		const auto source_size = 1 + get_effect_payload_size(data);

		auto sub_units_start = data + 3;
		while (sub_units_start < data + source_size) {
			random_value -= static_cast<int32_t>(sub_units_start[0]);
			if (random_value < 0)
				return sub_units_start + 1;
			sub_units_start += 2 + get_effect_payload_size(sub_units_start + 1);
		}

		return data + 2; // default to first member 
	}

#ifdef __llvm__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

	namespace {
		void _execute_effect(
			uint16_t const* tval,
			world_state& ws,
			void* primary_slot,
			void* this_slot,
			void* from_slot,
			population::rebel_faction* rebel_slot,
			jsf_prng& generator_copy);

		inline void apply_subeffects(uint16_t const* source, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			const auto source_size = 1 + get_effect_payload_size(source);
			auto sub_units_start = source + 2 + effect_scope_data_payload(source[0]);

			while(sub_units_start < source + source_size) {
				_execute_effect(sub_units_start, ws, primary_slot, this_slot, from_slot, rebel_slot, gen);
				sub_units_start += 1 + get_effect_payload_size(sub_units_start);
			}
		}

		void es_generic_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, primary_slot, this_slot, from_slot, rebel_slot, gen);
		}

		void es_x_neighbor_province_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto neighbor_range = ws.s.province_m.same_type_adjacency.get_row(((provinces::province_state*)primary_slot)->id);

			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<provinces::province_state*, 16, concurrent_allocator<provinces::province_state*>> rlist;
				
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner && test_trigger(limit, ws, &ps, this_slot, from_slot, rebel_slot))
							rlist.push_back(&ps);
					}
				} else {
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner)
							rlist.push_back(&ps);
					}
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);

					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner && test_trigger(limit, ws, &ps, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &ps, this_slot, from_slot, rebel_slot, gen);
					}
				} else {
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner)
							apply_subeffects(tval, ws, &ps, this_slot, from_slot, rebel_slot, gen);
					}
				}
			}
		}
		void es_x_neighbor_country_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto neighbor_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->neighboring_nations);

			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<nations::nation*, 16, concurrent_allocator<nations::nation*>> rlist;
				
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					for(auto n : neighbor_range) {
						auto& ns = ws.w.nation_s.nations[n];
						if(test_trigger(limit, ws, &ns, this_slot, from_slot, rebel_slot))
							rlist.push_back(&ns);
					}
				} else {
					for(auto n : neighbor_range)
						rlist.push_back(&(ws.w.nation_s.nations[n]));
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);

					for(auto n : neighbor_range) {
						auto& ns = ws.w.nation_s.nations[n];
						if(test_trigger(limit, ws, &ns, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &ns, this_slot, from_slot, rebel_slot, gen);
					}
				} else {
					for(auto n : neighbor_range) {
						apply_subeffects(tval, ws, &(ws.w.nation_s.nations[n]), this_slot, from_slot, rebel_slot, gen);
					}
				}
			}
		}
		void es_x_country_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<nations::nation*, 64, concurrent_allocator<nations::nation*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					ws.w.nation_s.nations.for_each([&rlist, limit, &ws, primary_slot, this_slot, from_slot, rebel_slot](nations::nation& n) {
						if(&n != primary_slot && get_size(ws.w.province_s.province_arrays, n.owned_provinces) != 0 && test_trigger(limit, ws, &n, this_slot, from_slot, rebel_slot))
							rlist.push_back(&n);
					});
				} else {
					ws.w.nation_s.nations.for_each([&rlist, &ws, primary_slot](nations::nation& n) {
						if(&n != primary_slot && get_size(ws.w.province_s.province_arrays, n.owned_provinces) != 0)
							rlist.push_back(&n);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					ws.w.nation_s.nations.for_each([tval, &gen, limit, &ws, this_slot, from_slot, rebel_slot](nations::nation& n) {
						if(test_trigger(limit, ws, &n, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &n, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					ws.w.nation_s.nations.for_each([tval, &gen, &ws, this_slot, from_slot, rebel_slot](nations::nation& n) {
						apply_subeffects(tval, ws, &n, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_country_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			es_x_country_scope_nation(tval, ws, nullptr, this_slot, from_slot, rebel_slot, gen);
		}
		void es_x_empty_neighbor_province_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto neighbor_range = ws.s.province_m.same_type_adjacency.get_row(((provinces::province_state*)primary_slot)->id);

			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<provinces::province_state*, 16, concurrent_allocator<provinces::province_state*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner == nullptr && test_trigger(limit, ws, &ps, this_slot, from_slot, rebel_slot))
							rlist.push_back(&ps);
					}
				} else {
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner == nullptr)
							rlist.push_back(&ps);
					}
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);

					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner == nullptr && test_trigger(limit, ws, &ps, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &ps, this_slot, from_slot, rebel_slot, gen);
					}
				} else {
					for(auto p : neighbor_range) {
						auto& ps = ws.w.province_s.province_state_container[p];
						if(ps.owner == nullptr)
							apply_subeffects(tval, ws, &ps, this_slot, from_slot, rebel_slot, gen);
					}
				}
			}
		}
		void es_x_greater_power_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ranked_range = get_range(ws.w.nation_s.nations_arrays, ws.w.nation_s.nations_by_rank);
			int32_t great_nations_count = int32_t(ws.s.modifiers_m.global_defines.great_nations_count);
			int32_t count = 0;

			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<nations::nation*, 16, concurrent_allocator<nations::nation*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					for(auto n = ranked_range.first; (n != ranked_range.second) & (count < great_nations_count); ++n) {
						if(is_valid_index(*n)) {
							auto& nation = ws.w.nation_s.nations[*n];
							if(nations::is_great_power(ws, nation) && test_trigger(limit, ws, &nation, this_slot, from_slot, rebel_slot)) {
								++count;
								rlist.push_back(&nation);
							}
						}
					}
				} else {
					for(auto n = ranked_range.first; (n != ranked_range.second) & (count < great_nations_count); ++n) {
						if(is_valid_index(*n)) {
							auto& nation = ws.w.nation_s.nations[*n];
							if(nations::is_great_power(ws, nation)) {
								++count;
								rlist.push_back(&nation);
							}
						}
					}
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					for(auto n = ranked_range.first; (n != ranked_range.second) & (count < great_nations_count); ++n) {
						if(is_valid_index(*n)) {
							auto& nation = ws.w.nation_s.nations[*n];
							if(nations::is_great_power(ws, nation) && test_trigger(limit, ws, &nation, this_slot, from_slot, rebel_slot)) {
								++count;
								apply_subeffects(tval, ws, &nation, this_slot, from_slot, rebel_slot, gen);
							}
						}
					}
				} else {
					for(auto n = ranked_range.first; (n != ranked_range.second) & (count < great_nations_count); ++n) {
						if(is_valid_index(*n)) {
							auto& nation = ws.w.nation_s.nations[*n];
							if(nations::is_great_power(ws, nation)) {
								++count;
								apply_subeffects(tval, ws, &nation, this_slot, from_slot, rebel_slot, gen);
							}
						}
					}
				}
			}
		}
		void es_poor_strata_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj){
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_poor_strata_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_poor_strata_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_poor) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_middle_strata_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_middle_strata_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_middle_strata_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_middle) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_rich_strata_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_rich_strata_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_rich_strata_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich &&
						test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot)) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			} else {
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
					auto type = pobj.type;
					if(is_valid_index(type) &&
						(ws.s.population_m.pop_types[type].flags & population::pop_type::strata_mask) == population::pop_type::strata_rich) {

						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					}
				});
			}
		}
		void es_x_pop_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<population::pop*, 16, concurrent_allocator<population::pop*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&rlist](population::pop& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_pop_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<population::pop*, 16, concurrent_allocator<population::pop*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&rlist](population::pop& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_pop_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<population::pop*, 16, concurrent_allocator<population::pop*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&rlist](population::pop& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](population::pop& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_owned_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<provinces::province_state*, 16, concurrent_allocator<provinces::province_state*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_province(ws, *((nations::nation*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](provinces::province_state& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					nations::for_each_province(ws, *((nations::nation*)primary_slot), [&rlist](provinces::province_state& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_province(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](provinces::province_state& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					nations::for_each_province(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](provinces::province_state& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_owned_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<provinces::province_state*, 16, concurrent_allocator<provinces::province_state*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_province(ws, *((nations::state_instance*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](provinces::province_state& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					nations::for_each_province(ws, *((nations::state_instance*)primary_slot), [&rlist](provinces::province_state& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_province(ws, *((nations::state_instance*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](provinces::province_state& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					nations::for_each_province(ws, *((nations::state_instance*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](provinces::province_state& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_x_core_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = ((nations::nation*)primary_slot)->tag;
			if(is_valid_index(tag)) {
				auto cores_range = get_range(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces);

				if((tval[0] & effect_codes::is_random_scope) != 0) {
					boost::container::small_vector<provinces::province_state*, 16, concurrent_allocator<provinces::province_state*>> rlist;

					if((tval[0] & effect_codes::scope_has_limit) != 0) {
						auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
						for(auto p : cores_range) {
							if(is_valid_index(p)) {
								auto& pobj = ws.w.province_s.province_state_container[p];
								if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
									rlist.push_back(&pobj);
							}
						}
					} else {
						for(auto p : cores_range) {
							if(is_valid_index(p))
								rlist.push_back(&(ws.w.province_s.province_state_container[p]));
						}
					}

					if(rlist.size() != 0) {
						std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
						apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
					}
				} else {
					if((tval[0] & effect_codes::scope_has_limit) != 0) {
						auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
						for(auto p : cores_range) {
							if(is_valid_index(p)) {
								auto& pobj = ws.w.province_s.province_state_container[p];
								if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
									apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
							}
						}
					} else {
						for(auto p : cores_range) {
							if(is_valid_index(p)) 
								apply_subeffects(tval, ws, &(ws.w.province_s.province_state_container[p]), this_slot, from_slot, rebel_slot, gen);
						}
					}
				}
			}
		}
		void es_x_state_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::is_random_scope) != 0) {
				boost::container::small_vector<nations::state_instance*, 16, concurrent_allocator<nations::state_instance*>> rlist;

				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_state(ws, *((nations::nation*)primary_slot), [&ws, &rlist, limit, this_slot, from_slot, rebel_slot](nations::state_instance& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							rlist.push_back(&pobj);
					});
				} else {
					nations::for_each_state(ws, *((nations::nation*)primary_slot), [&rlist](nations::state_instance& pobj) {
						rlist.push_back(&pobj);
					});
				}

				if(rlist.size() != 0) {
					std::uniform_int_distribution<int32_t> dist(0, int32_t(rlist.size()) - 1);
					apply_subeffects(tval, ws, rlist[uint32_t(dist(gen))], this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				if((tval[0] & effect_codes::scope_has_limit) != 0) {
					auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
					nations::for_each_state(ws, *((nations::nation*)primary_slot), [&ws, limit, tval, this_slot, from_slot, rebel_slot, &gen](nations::state_instance& pobj) {
						if(test_trigger(limit, ws, &pobj, this_slot, from_slot, rebel_slot))
							apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				} else {
					nations::for_each_state(ws, *((nations::nation*)primary_slot), [&ws, tval, this_slot, from_slot, rebel_slot, &gen](nations::state_instance& pobj) {
						apply_subeffects(tval, ws, &pobj, this_slot, from_slot, rebel_slot, gen);
					});
				}
			}
		}
		void es_random_list_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			const auto source_size = 1 + get_effect_payload_size(tval);
			auto chances_total = tval[2];

			std::uniform_int_distribution<int32_t> dist(0, int32_t(chances_total) - 1);
			auto chance_taken = dist(gen);

			auto sub_units_start = tval + 3; // [code] + [payload size] + [chances total] + [first sub effect chance]
			while(sub_units_start < tval + source_size) {
				chance_taken -= *sub_units_start;
				if(chance_taken < 0) {
					_execute_effect(sub_units_start + 1, ws, primary_slot, this_slot, from_slot, rebel_slot, gen);
					return;
				}
				sub_units_start += 2 + get_effect_payload_size(sub_units_start + 1); // each member preceeded by uint16_t
			}
		}
		void es_random_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto chance = tval[2];
			std::uniform_int_distribution<int32_t> dist(0, 99);
			if(dist(gen) < chance)
				apply_subeffects(tval, ws, primary_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_owner_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)primary_slot)->owner;
			if(owner)
				apply_subeffects(tval, ws, owner, this_slot, from_slot, rebel_slot, gen);
		}
		void es_owner_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)primary_slot)->owner;
			if(owner)
				apply_subeffects(tval, ws, owner, this_slot, from_slot, rebel_slot, gen);
		}
		void es_controller_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto controller = ((provinces::province_state*)primary_slot)->controller;
			if(controller)
				apply_subeffects(tval, ws, controller, this_slot, from_slot, rebel_slot, gen);
		}
		void es_location_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto location = ((population::pop*)primary_slot)->location;
			if(is_valid_index(location))
				apply_subeffects(tval, ws, &(ws.w.province_s.province_state_container[location]), this_slot, from_slot, rebel_slot, gen);
		}
		void es_country_scope_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot));
			if(owner)
				apply_subeffects(tval, ws, owner, this_slot, from_slot, rebel_slot, gen);
		}
		void es_country_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)primary_slot)->owner;
			if(owner)
				apply_subeffects(tval, ws, owner, this_slot, from_slot, rebel_slot, gen);
		}
		void es_capital_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital))
				apply_subeffects(tval, ws, &(ws.w.province_s.province_state_container[capital]), this_slot, from_slot, rebel_slot, gen);
		}
		void es_this_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, this_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_this_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, this_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_this_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, this_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_this_scope_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, this_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_from_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, from_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_from_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, from_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_from_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, from_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_from_scope_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			apply_subeffects(tval, ws, from_slot, this_slot, from_slot, rebel_slot, gen);
		}
		void es_sea_zone_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto sea_zones = ws.s.province_m.coastal_adjacency.get_row(((provinces::province_state*)primary_slot)->id);
			if(sea_zones.first != sea_zones.second)
				apply_subeffects(tval, ws, &(ws.w.province_s.province_state_container[*sea_zones.first]), this_slot, from_slot, rebel_slot, gen);
		}
		void es_cultural_union_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
			if(!is_valid_index(prim_culture))
				return;

			auto culture_group = ws.s.culture_m.culture_container[prim_culture].group;
			if(is_valid_index(culture_group)) {
				auto union_tag = ws.s.culture_m.culture_groups[culture_group].union_tag;
				if(is_valid_index(union_tag)) {
					auto union_holder = ws.w.culture_s.national_tags_state[union_tag].holder;
					if(union_holder)
						apply_subeffects(tval, ws, union_holder, this_slot, from_slot, rebel_slot, gen);
				}
			}
		}
		void es_overlord_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto overlord = ((nations::nation*)primary_slot)->overlord;
			if(overlord)
				apply_subeffects(tval, ws, overlord, this_slot, from_slot, rebel_slot, gen);
		}
		void es_sphere_owner_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto sphere_leader = ((nations::nation*)primary_slot)->sphere_leader;
			if(sphere_leader)
				apply_subeffects(tval, ws, sphere_leader, this_slot, from_slot, rebel_slot, gen);
		}
		void es_independence_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto rtag = rebel_slot->independence_tag;
			if(is_valid_index(rtag)) {
				auto ination = ws.w.culture_s.national_tags_state[rtag].holder;
				if(ination)
					apply_subeffects(tval, ws, ination, this_slot, from_slot, rebel_slot, gen);
			}
		}
		void es_flashpoint_tag_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ctag = ((nations::state_instance*)primary_slot)->crisis_tag;
			if(is_valid_index(ctag)) {
				auto ctag_holder = ws.w.culture_s.national_tags_state[ctag].holder;
				if(ctag_holder)
					apply_subeffects(tval, ws, ctag_holder, this_slot, from_slot, rebel_slot, gen);
			}
		}
		void es_crisis_state_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto cstate = ws.w.current_crisis.state;
			if(is_valid_index(cstate))
				apply_subeffects(tval, ws, &(ws.w.nation_s.states.get(cstate)), this_slot, from_slot, rebel_slot, gen);
		}
		void es_state_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto state = ((provinces::province_state*)primary_slot)->state_instance;
			if(state)
				apply_subeffects(tval, ws, state, this_slot, from_slot, rebel_slot, gen);
		}
		void es_state_scope_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto pop_province = ((population::pop*)primary_slot)->location;
			if(is_valid_index(pop_province)) {
				auto prov_state = &(ws.w.province_s.province_state_container[pop_province]);
				es_state_scope_province(tval, ws, prov_state, this_slot, from_slot, rebel_slot, gen);
			}
		}
		void es_tag_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				auto tag = trigger_payload(tval[3]).tag;
				auto tag_holder = ws.w.culture_s.national_tags_state[tag].holder;
				if(tag_holder && test_trigger(limit, ws, tag_holder, this_slot, from_slot, rebel_slot))
					apply_subeffects(tval, ws, tag_holder, this_slot, from_slot, rebel_slot, gen);
			} else {
				auto tag = trigger_payload(tval[2]).tag;
				auto tag_holder = ws.w.culture_s.national_tags_state[tag].holder;
				if(tag_holder)
					apply_subeffects(tval, ws, tag_holder, this_slot, from_slot, rebel_slot, gen);
			}
		}
		void es_integer_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				provinces::province_tag ptag(tval[3]);
				provinces::province_state* pstate = &(ws.w.province_s.province_state_container[ptag]);
				if(test_trigger(limit, ws, pstate, this_slot, from_slot, rebel_slot))
					apply_subeffects(tval, ws, pstate, this_slot, from_slot, rebel_slot, gen);
			} else {
				provinces::province_tag ptag(tval[2]);
				provinces::province_state* pstate = &(ws.w.province_s.province_state_container[ptag]);
				apply_subeffects(tval, ws, pstate, this_slot, from_slot, rebel_slot, gen);
			}
		}
		void es_pop_type_scope_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				auto type = trigger_payload(tval[3]).small.values.pop_type;

				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [limit, type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type && test_trigger(limit, ws, &p, this_slot, from_slot, rebel_slot))
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			} else {
				auto type = trigger_payload(tval[2]).small.values.pop_type;

				nations::for_each_pop(ws, *((nations::nation*)primary_slot), [type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type)
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			}
		}
		void es_pop_type_scope_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				auto type = trigger_payload(tval[3]).small.values.pop_type;

				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [limit, type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type && test_trigger(limit, ws, &p, this_slot, from_slot, rebel_slot))
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			} else {
				auto type = trigger_payload(tval[2]).small.values.pop_type;

				nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type)
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			}
		}
		void es_pop_type_scope_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				auto type = trigger_payload(tval[3]).small.values.pop_type;

				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [limit, type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type && test_trigger(limit, ws, &p, this_slot, from_slot, rebel_slot))
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			} else {
				auto type = trigger_payload(tval[2]).small.values.pop_type;

				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [type, tval, &ws, this_slot, from_slot, rebel_slot, &gen](population::pop& p) {
					if(p.type == type)
						apply_subeffects(tval, ws, &p, this_slot, from_slot, rebel_slot, gen);
				});
			}
		}
		void es_region_scope(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if((tval[0] & effect_codes::scope_has_limit) != 0) {
				auto region = trigger_payload(tval[3]).state;
				auto provinces = ws.s.province_m.states_to_province_index.get_row(region);

				auto limit = ws.s.trigger_m.trigger_data.data() + to_index(trigger_payload(tval[2]).trigger);
				for(auto p : provinces) {
					auto& ps = ws.w.province_s.province_state_container[p];
					if(test_trigger(limit, ws, &ps, this_slot, from_slot, rebel_slot))
						apply_subeffects(tval, ws, &ps, this_slot, from_slot, rebel_slot, gen);
				}
			} else {
				auto region = trigger_payload(tval[2]).state;
				auto provinces = ws.s.province_m.states_to_province_index.get_row(region);

				for(auto p : provinces)
					apply_subeffects(tval, ws, &(ws.w.province_s.province_state_container[p]), this_slot, from_slot, rebel_slot, gen);
			}
		}

		static void(*effect_scope_functions[])(uint16_t const*, world_state&, void*, void*, void*, population::rebel_faction*, jsf_prng&) = {
			es_generic_scope,
			es_x_neighbor_province_scope ,
			es_x_neighbor_country_scope ,
			es_x_country_scope ,
			es_x_country_scope_nation,
			es_x_empty_neighbor_province_scope,
			es_x_greater_power_scope ,
			es_poor_strata_scope_nation ,
			es_poor_strata_scope_state ,
			es_poor_strata_scope_province ,
			es_middle_strata_scope_nation,
			es_middle_strata_scope_state ,
			es_middle_strata_scope_province,
			es_rich_strata_scope_nation,
			es_rich_strata_scope_state ,
			es_rich_strata_scope_province ,
			es_x_pop_scope_nation ,
			es_x_pop_scope_state ,
			es_x_pop_scope_province,
			es_x_owned_scope_nation ,
			es_x_owned_scope_state,
			es_x_core_scope ,
			es_x_state_scope,
			es_random_list_scope,
			es_random_scope ,
			es_owner_scope_state ,
			es_owner_scope_province,
			es_controller_scope,
			es_location_scope,
			es_country_scope_pop ,
			es_country_scope_state,
			es_capital_scope ,
			es_this_scope_nation ,
			es_this_scope_state ,
			es_this_scope_province ,
			es_this_scope_pop ,
			es_from_scope_nation ,
			es_from_scope_state,
			es_from_scope_province ,
			es_from_scope_pop ,
			es_sea_zone_scope ,
			es_cultural_union_scope ,
			es_overlord_scope ,
			es_sphere_owner_scope ,
			es_independence_scope ,
			es_flashpoint_tag_scope,
			es_crisis_state_scope,
			es_state_scope_pop ,
			es_state_scope_province,
			es_tag_scope,
			es_integer_scope ,
			es_pop_type_scope_nation ,
			es_pop_type_scope_state,
			es_pop_type_scope_province ,
			es_region_scope,
		};

		void ef_none(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {}
		void ef_capital(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::province_tag new_capital(tval[2]);
			if(ws.w.province_s.province_state_container[new_capital].owner == primary_slot)
				((nations::nation*)primary_slot)->current_capital = new_capital;
		}
		void ef_add_core_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = trigger_payload(tval[2]).tag;
			add_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
			add_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
		}
		void ef_add_core_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = ((nations::nation*)primary_slot)->tag;
			provinces::province_tag prov(tval[2]);
			if(is_valid_index(tag)) {
				add_item(ws.w.province_s.core_arrays, ws.w.province_s.province_state_container[prov].cores, tag);
				add_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, prov);
			}
		}
		void ef_add_core_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = ((nations::nation*)this_slot)->tag;
			if(is_valid_index(tag)) {
				add_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
				add_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
			}
		}
		void ef_add_core_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_add_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_add_core_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner)
				ef_add_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_add_core_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner)
				ef_add_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_add_core_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_add_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_add_core_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_add_core_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_add_core_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = rebel_slot->independence_tag;
			if(is_valid_index(tag)) {
				add_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
				add_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
			}
		}
		void ef_remove_core_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = trigger_payload(tval[2]).tag;
			remove_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
			remove_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
		}
		void ef_remove_core_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = ((nations::nation*)primary_slot)->tag;
			provinces::province_tag prov(tval[2]);
			if(is_valid_index(tag)) {
				remove_item(ws.w.province_s.core_arrays, ws.w.province_s.province_state_container[prov].cores, tag);
				remove_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, prov);
			}
		}
		void ef_remove_core_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = ((nations::nation*)this_slot)->tag;
			if(is_valid_index(tag)) {
				remove_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
				remove_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
			}
		}
		void ef_remove_core_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_remove_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_remove_core_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner)
				ef_remove_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_remove_core_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner)
				ef_remove_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_remove_core_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_remove_core_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_remove_core_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_remove_core_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_remove_core_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto tag = rebel_slot->independence_tag;
			if(is_valid_index(tag)) {
				remove_item(ws.w.province_s.core_arrays, ((provinces::province_state*)primary_slot)->cores, tag);
				remove_item(ws.w.province_s.province_arrays, ws.w.culture_s.national_tags_state[tag].core_provinces, ((provinces::province_state*)primary_slot)->id);
			}
		}
		void ef_change_region_name_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::state_instance*)primary_slot)->name = trigger_payload(tval[2]).text;
		}
		void ef_change_region_name_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto si = ((provinces::province_state*)primary_slot)->state_instance;
			if(si)
				si->name = trigger_payload(tval[2]).text;
		}
		void ef_trade_goods(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->rgo_production = trigger_payload(tval[2]).small.values.good;
			economy::match_rgo_worker_type(ws, *((provinces::province_state*)primary_slot));
		}
		void ef_add_accepted_culture(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			add_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, trigger_payload(tval[2]).culture);
		}
		void ef_add_accepted_culture_union(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
			if(is_valid_index(prim_culture)) {
				auto cg_t = ws.s.culture_m.culture_container[prim_culture].group;
				auto g_range = ws.s.culture_m.culture_by_culture_group.get_row(cg_t);
				for(auto c : g_range)
					add_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, c);
			}
		}
		void ef_primary_culture(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->primary_culture = trigger_payload(tval[2]).culture;
		}
		void ef_primary_culture_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->primary_culture = ((nations::nation*)this_slot)->primary_culture;
		}
		void ef_primary_culture_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner)
				((nations::nation*)primary_slot)->primary_culture = owner->primary_culture;
		}
		void ef_primary_culture_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				((nations::nation*)primary_slot)->primary_culture = owner->primary_culture;
		}
		void ef_primary_culture_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner)
				((nations::nation*)primary_slot)->primary_culture = owner->primary_culture;
		}
		void ef_primary_culture_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->primary_culture = ((nations::nation*)from_slot)->primary_culture;
		}
		void ef_remove_accepted_culture(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			remove_item(ws.w.culture_s.culture_arrays, ((nations::nation*)primary_slot)->accepted_cultures, trigger_payload(tval[2]).culture);
		}
		void ef_life_rating(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->base_life_rating += trigger_payload(tval[2]).signed_value;
		}
		void ef_religion(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->national_religion = trigger_payload(tval[2]).small.values.religion;
		}
		void ef_is_slave_state_yes(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::make_slave_state(ws, *((nations::state_instance*)primary_slot));
		}
		void ef_is_slave_pop_yes(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::change_pop_type(ws, *((population::pop*)primary_slot), ws.s.population_m.slave);
		}
		void ef_research_points(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->research_points += trigger_payload(tval[2]).signed_value;
		}
		void ef_tech_school(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->tech_school = trigger_payload(tval[2]).nat_mod;
		}
		void ef_government(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			governments::silent_set_government(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).small.values.government);
		}
		void ef_government_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			governments::silent_set_government(ws, *((nations::nation*)primary_slot),
				ws.s.population_m.rebel_change_government_to.get(rebel_slot->type, ((nations::nation*)primary_slot)->current_government));
		}
		void ef_treasury(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.nation_s.national_stockpiles.get(((nations::nation*)primary_slot)->id, economy::money_good) += economy::goods_qnty_type(read_float_from_payload(tval + 2));
		}
		void ef_war_exhaustion(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->war_exhaustion = std::clamp(((nations::nation*)primary_slot)->war_exhaustion + read_float_from_payload(tval + 2), 0.0f, 1.0f);
		}
		void ef_prestige(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto amount = read_float_from_payload(tval + 2);
			if(amount >= 0.0f) {
				((nations::nation*)primary_slot)->base_prestige = std::max(
					0.0f,
					((nations::nation*)primary_slot)->base_prestige + amount * (((nations::nation*)primary_slot)->tech_attributes[technologies::tech_offset::prestige] + 1.0f));
			} else {
				((nations::nation*)primary_slot)->base_prestige = std::max(
					0.0f,
					((nations::nation*)primary_slot)->base_prestige + amount);
			}
		}
		void ef_change_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(is_valid_index(((nations::nation*)primary_slot)->tag))
				cultures::replace_cores(ws, ((nations::nation*)primary_slot)->tag, trigger_payload(tval[2]).tag);
			nations::change_tag(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tag);
		}
		void ef_change_tag_culture(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
			if(is_valid_index(prim_culture)) {
				auto cg_t = ws.s.culture_m.culture_container[prim_culture].group;
				auto u = ws.s.culture_m.culture_groups[cg_t].union_tag;
				if(is_valid_index(u)) {
					if(is_valid_index(((nations::nation*)primary_slot)->tag))
						cultures::replace_cores(ws, ((nations::nation*)primary_slot)->tag, u);
					nations::change_tag(ws, *((nations::nation*)primary_slot), u);
				}
			}
		}
		void ef_change_tag_no_core_switch(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(ws.w.local_player_nation == primary_slot) {
				if(auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder; holder) {
					ws.w.local_player_nation->flags &= ~nations::nation::is_not_ai_controlled;
					holder->flags |= nations::nation::is_not_ai_controlled;
					ws.w.local_player_nation = holder;
				}
			}
		}
		void ef_change_tag_no_core_switch_culture(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(ws.w.local_player_nation == primary_slot) {
				auto prim_culture = ((nations::nation*)primary_slot)->primary_culture;
				if(is_valid_index(prim_culture)) {
					auto cg_t = ws.s.culture_m.culture_container[prim_culture].group;
					auto u = ws.s.culture_m.culture_groups[cg_t].union_tag;
					if(is_valid_index(u)) {
						if(auto holder = ws.w.culture_s.national_tags_state[u].holder; holder) {
							ws.w.local_player_nation->flags &= ~nations::nation::is_not_ai_controlled;
							holder->flags |= nations::nation::is_not_ai_controlled;
							ws.w.local_player_nation = holder;
						}
					}
				}
			}
		}
		void ef_set_country_flag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			add_item(ws.w.variable_s.national_flags_arrays, ((nations::nation*)primary_slot)->national_flags, trigger_payload(tval[2]).nat_flag);
		}
		void ef_clr_country_flag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			remove_item(ws.w.variable_s.national_flags_arrays, ((nations::nation*)primary_slot)->national_flags, trigger_payload(tval[2]).nat_flag);
		}
		void ef_military_access(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_military_access_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_military_access_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_military_access_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_military_access_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_badboy(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->infamy = std::max(0.0f, ((nations::nation*)primary_slot)->infamy + read_float_from_payload(tval + 2));
		}
		void ef_secede_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = nations::make_nation_for_tag(ws, trigger_payload(tval[2]).tag);
			provinces::silent_set_province_owner(ws, *holder, *((provinces::province_state*)primary_slot));
			provinces::silent_set_province_controller(ws, *holder, *((provinces::province_state*)primary_slot));
			provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
		}
		void ef_secede_province_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::silent_set_province_owner(ws, *((nations::nation*)this_slot), *((provinces::province_state*)primary_slot));
			provinces::silent_set_province_controller(ws, *((nations::nation*)this_slot), *((provinces::province_state*)primary_slot));
			provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
		}
		void ef_secede_province_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner) {
				provinces::silent_set_province_owner(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_set_province_controller(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
			}
		}
		void ef_secede_province_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner) {
				provinces::silent_set_province_owner(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_set_province_controller(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
			}
		}
		void ef_secede_province_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner) {
				provinces::silent_set_province_owner(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_set_province_controller(ws, *owner, *((provinces::province_state*)primary_slot));
				provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
			}
		}
		void ef_secede_province_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_secede_province_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_secede_province_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_secede_province_this_province(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_secede_province_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(is_valid_index(rebel_slot->independence_tag)) {
				auto holder = nations::make_nation_for_tag(ws, rebel_slot->independence_tag);
				provinces::silent_set_province_owner(ws, *holder, *((provinces::province_state*)primary_slot));
				provinces::silent_set_province_controller(ws, *holder, *((provinces::province_state*)primary_slot));
				provinces::silent_on_conquer_province(ws, *((provinces::province_state*)primary_slot));
			}
		}
		void ef_inherit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder && primary_slot != holder) 
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *holder);
		}
		void ef_inherit_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(primary_slot != this_slot) 
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot));
		}
		void ef_inherit_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_inherit_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_inherit_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_inherit_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(primary_slot != from_slot)
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot));
		}
		void ef_inherit_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_annex_to(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder && primary_slot != holder)
				nations::annex_nation(ws, *holder, *((nations::nation*)primary_slot));
		}
		void ef_annex_to_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(primary_slot != this_slot)
				nations::annex_nation(ws, *((nations::nation*)this_slot), *((nations::nation*)primary_slot));
		}
		void ef_annex_to_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *owner, *((nations::nation*)primary_slot));
		}
		void ef_annex_to_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *owner, *((nations::nation*)primary_slot));
		}
		void ef_annex_to_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *owner, *((nations::nation*)primary_slot));
		}
		void ef_annex_to_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(primary_slot != from_slot)
				nations::annex_nation(ws, *((nations::nation*)from_slot), *((nations::nation*)primary_slot));
		}
		void ef_annex_to_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner && primary_slot != owner)
				nations::annex_nation(ws, *owner, *((nations::nation*)primary_slot));
		}
		void ef_release(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::liberate_uncored_cores(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tag);
		}
		void ef_release_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(is_valid_index(((nations::nation*)this_slot)->tag))
				nations::liberate_uncored_cores(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->tag);
		}
		void ef_release_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((nations::state_instance*)this_slot)->owner;
			if(owner)
				ef_release_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_release_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_release_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_release_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot));
			if(owner)
				ef_release_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_release_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(is_valid_index(((nations::nation*)from_slot)->tag))
				nations::liberate_uncored_cores(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->tag);
		}
		void ef_release_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_release_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_change_controller(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder)
				provinces::silent_set_province_controller(ws, *holder, *((provinces::province_state*)primary_slot));
		}
		void ef_change_controller_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::silent_set_province_controller(ws, *((nations::nation*)this_slot), *((provinces::province_state*)primary_slot));
		}
		void ef_change_controller_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				provinces::silent_set_province_controller(ws, *owner, *((provinces::province_state*)primary_slot));
		}
		void ef_change_controller_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::silent_set_province_controller(ws, *((nations::nation*)from_slot), *((provinces::province_state*)primary_slot));
		}
		void ef_change_controller_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				provinces::silent_set_province_controller(ws, *owner, *((provinces::province_state*)primary_slot));
		}
		void ef_infrastructure(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->railroad_level =
				uint8_t(std::clamp(
				int32_t(((provinces::province_state*)primary_slot)->railroad_level) + int32_t(trigger_payload(tval[2]).signed_value), 0, 255));
		}
		void ef_money(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((population::pop*)primary_slot)->money += read_float_from_payload(tval + 2);
		}
		void ef_leadership(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->leadership_points += trigger_payload(tval[2]).signed_value;
		}
		void ef_create_vassal(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder && get_size(ws.w.province_s.province_arrays, holder->owned_provinces) != 0)
				nations::make_vassal(ws, *((nations::nation*)primary_slot), *holder);
		}
		void ef_create_vassal_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(get_size(ws.w.province_s.province_arrays, ((nations::nation*)this_slot)->owned_provinces) != 0)
				nations::make_vassal(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot));
		}
		void ef_create_vassal_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_create_vassal_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_create_vassal_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(get_size(ws.w.province_s.province_arrays, ((nations::nation*)from_slot)->owned_provinces) != 0)
				nations::make_vassal(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot));
		}
		void ef_create_vassal_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_create_vassal_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_end_military_access(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_end_military_access_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_end_military_access_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_end_military_access_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_end_military_access_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_leave_alliance(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder)
				nations::end_alliance(ws, *((nations::nation*)primary_slot), *holder);
		}
		void ef_leave_alliance_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::end_alliance(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot));
		}
		void ef_leave_alliance_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				nations::end_alliance(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_leave_alliance_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::end_alliance(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot));
		}
		void ef_leave_alliance_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				nations::end_alliance(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_end_war(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder) {
				auto war_between = military::get_war_between(ws, *((nations::nation*)primary_slot), holder->id);
				if(war_between) {
					military::destroy_war(ws, *war_between);
					ws.w.military_s.wars.remove(war_between->id);
				}
			}
		}
		void ef_end_war_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto war_between = military::get_war_between(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->id);
			if(war_between) {
				military::destroy_war(ws, *war_between);
				ws.w.military_s.wars.remove(war_between->id);
			}
		}
		void ef_end_war_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_end_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_end_war_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto war_between = military::get_war_between(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id);
			if(war_between) {
				military::destroy_war(ws, *war_between);
				ws.w.military_s.wars.remove(war_between->id);
			}
		}
		void ef_end_war_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_end_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_enable_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.ideology_s.ideology_enabled[trigger_payload(tval[2]).small.values.ideology] = 1ui8;
		}
		void ef_ruling_party_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto new_party = ws.w.nation_s.active_parties.get(((nations::nation*)primary_slot)->id, trigger_payload(tval[2]).small.values.ideology);
			if(is_valid_index(new_party) && new_party != ((nations::nation*)primary_slot)->ruling_party)
				governments::silent_set_ruling_party(ws, *((nations::nation*)primary_slot), new_party);
		}
		void ef_plurality(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->plurality = std::clamp(((nations::nation*)primary_slot)->plurality + read_float_from_payload(tval + 2), 0.0f, 1.0f);
		}
		void ef_remove_province_modifier(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto pmod = trigger_payload(tval[2]).prov_mod;
			modifiers::remove_static_modifier_from_province(ws, *((provinces::province_state*)primary_slot), pmod);
			modifiers::remove_all_timed_modifiers_from_province(ws, *((provinces::province_state*)primary_slot), pmod);
		}
		void ef_remove_country_modifier(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto nmod = trigger_payload(tval[2]).nat_mod;
			modifiers::remove_all_static_modifiers_from_nation(ws, *((nations::nation*)primary_slot), nmod);
			modifiers::remove_all_timed_modifiers_from_nation(ws, *((nations::nation*)primary_slot), nmod);
		}
		void ef_create_alliance(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(holder)
				nations::make_alliance(ws, *((nations::nation*)primary_slot), *holder);
		}
		void ef_create_alliance_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::make_alliance(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot));
		}
		void ef_create_alliance_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				nations::make_alliance(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_create_alliance_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::make_alliance(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot));
		}
		void ef_create_alliance_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				nations::make_alliance(ws, *((nations::nation*)primary_slot), *owner);
		}
		void ef_release_vassal(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder; bool(holder) && get_size(ws.w.province_s.province_arrays, holder->owned_provinces) != 0) {
				nations::free_vassal(ws, *holder);
			} else {
				auto& new_nation = nations::liberate_uncored_cores(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tag);
				if(get_size(ws.w.province_s.province_arrays, new_nation.owned_provinces) != 0)
					nations::make_vassal(ws, *((nations::nation*)primary_slot), new_nation);
			}
		}
		void ef_release_vassal_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(get_size(ws.w.province_s.province_arrays, ((nations::nation*)this_slot)->owned_provinces) != 0) {
				nations::free_vassal(ws, *((nations::nation*)this_slot));
			} else {
				auto& new_nation = nations::liberate_uncored_cores(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->tag);
				if(get_size(ws.w.province_s.province_arrays, new_nation.owned_provinces) != 0)
					nations::make_vassal(ws, *((nations::nation*)primary_slot), new_nation);
			}
		}
		void ef_release_vassal_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)this_slot)->owner;
			if(owner)
				ef_release_vassal_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_release_vassal_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_release_vassal_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_release_vassal_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)from_slot)->owner;
			if(owner)
				ef_release_vassal_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_release_vassal_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::liberate_all_cores(ws, *((nations::nation*)primary_slot), rebel_slot->independence_tag);
		}
		void ef_release_vassal_random(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			//unused
		}
		void ef_change_province_name(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->name = trigger_payload(tval[2]).text;
		}
		void ef_enable_canal(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::enable_canal(ws, tval[2] - 1);
		}
		void ef_set_global_flag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.variable_s.global_variables[trigger_payload(tval[2]).global_var] = 1.0f;
		}
		void ef_clr_global_flag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.variable_s.global_variables[trigger_payload(tval[2]).global_var] = 0.0f;
		}
		void ef_nationalvalue_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto owner = ((provinces::province_state*)primary_slot)->owner;
			if(owner)
				owner->national_value = trigger_payload(tval[2]).nat_mod;
		}
		void ef_nationalvalue_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->national_value = trigger_payload(tval[2]).nat_mod;
		}
		void ef_civilized_yes(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::civilize_nation(ws, *((nations::nation*)primary_slot));
		}
		void ef_civilized_no(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::uncivilize_nation(ws, *((nations::nation*)primary_slot));
		}
		void ef_is_slave_state_no(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::unmake_slave_state(ws, *((nations::state_instance*)primary_slot));
		}
		void ef_is_slave_pop_no(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(((population::pop*)primary_slot)->type == ws.s.population_m.slave)
				population::free_slave(ws, *((population::pop*)primary_slot));
		}
		void ef_election(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(ws.s.governments_m.governments_container[((nations::nation*)primary_slot)->current_government].election)
				governments::start_election(ws, *((nations::nation*)primary_slot));
		}
		void ef_social_reform(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			issues::change_issue_option(ws, trigger_payload(tval[2]).small.values.option, *((nations::nation*)primary_slot));
		}
		void ef_political_reform(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			issues::change_issue_option(ws, trigger_payload(tval[2]).small.values.option, *((nations::nation*)primary_slot));
		}
		void ef_add_tax_relative_income(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.nation_s.national_stockpiles.get(((nations::nation*)primary_slot)->id, economy::money_good) += economy::goods_qnty_type(((nations::nation*)primary_slot)->last_income * economy::money_qnty_type(read_float_from_payload(tval + 2)));
		}
		void ef_neutrality(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto allies_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->allies);
			boost::container::small_vector<nations::country_tag, 16> allies_copy(allies_range.first, allies_range.second);

			for(auto a : allies_copy)
				nations::end_alliance(ws, *((nations::nation*)primary_slot), ws.w.nation_s.nations[a]);

			auto vassals_range = get_range(ws.w.nation_s.nations_arrays, ((nations::nation*)primary_slot)->vassals);
			boost::container::small_vector<nations::country_tag, 16> vassals_copy(vassals_range.first, vassals_range.second);

			for(auto a : vassals_copy)
				nations::free_vassal(ws, ws.w.nation_s.nations[a]);
		}
		void ef_reduce_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::change_pop_size(
				ws, *((population::pop*)primary_slot), 
				int32_t(ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag) * read_float_from_payload(tval + 2)));
		}
		void ef_move_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::change_pop_location(ws, *((population::pop*)primary_slot), provinces::province_tag(tval[2]));
		}
		void ef_pop_type(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::change_pop_type(ws, *((population::pop*)primary_slot), trigger_payload(tval[2]).small.values.pop_type);
		}
		void ef_years_of_research(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->research_points = 
				int16_t(float(((nations::nation*)primary_slot)->research_points) +
					technologies::daily_research_points(ws, *((nations::nation*)primary_slot)) * 365.0f * read_float_from_payload(tval + 2));
		}
		void ef_prestige_factor_positive(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->base_prestige +=
				(((nations::nation*)primary_slot)->base_prestige + ((nations::nation*)primary_slot)->tech_attributes[technologies::tech_offset::permanent_prestige]) *
				(((nations::nation*)primary_slot)->tech_attributes[technologies::tech_offset::prestige] + 1.0f) *
				read_float_from_payload(tval + 2);
		}
		void ef_prestige_factor_negative(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::nation*)primary_slot)->base_prestige = std::max(
				0.0f,
				((nations::nation*)primary_slot)->base_prestige +
				(((nations::nation*)primary_slot)->base_prestige + ((nations::nation*)primary_slot)->tech_attributes[technologies::tech_offset::permanent_prestige]) *
				read_float_from_payload(tval + 2));
		}
		void ef_military_reform(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			issues::change_issue_option(ws, trigger_payload(tval[2]).small.values.option, *((nations::nation*)primary_slot));
		}
		void ef_economic_reform(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			issues::change_issue_option(ws, trigger_payload(tval[2]).small.values.option, *((nations::nation*)primary_slot));
		}
		void ef_remove_random_military_reforms(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			boost::container::small_vector<issues::issue_tag, 16> active_reforms;
			auto nation_id = ((nations::nation*)primary_slot)->id;
			for(auto issue : ws.s.issues_m.military_issues) {
				if(ws.w.nation_s.active_issue_options.get(nation_id, issue) != ws.s.issues_m.issues_container[issue].options[0])
					active_reforms.push_back(issue);
			}
			for(int32_t i = tval[2] - 1; active_reforms.size() != 0 && i >= 0; --i) {
				std::uniform_int_distribution<int32_t> dist(0, int32_t(active_reforms.size()) - 1);
				auto chance_taken = uint32_t(dist(gen));
				issues::change_issue_option(ws, ws.s.issues_m.issues_container[active_reforms[chance_taken]].options[0], *((nations::nation*)primary_slot));
			}
		}
		void ef_remove_random_economic_reforms(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			boost::container::small_vector<issues::issue_tag, 16> active_reforms;
			auto nation_id = ((nations::nation*)primary_slot)->id;
			for(auto issue : ws.s.issues_m.economic_issues) {
				if(ws.w.nation_s.active_issue_options.get(nation_id, issue) != ws.s.issues_m.issues_container[issue].options[0])
					active_reforms.push_back(issue);
			}
			for(int32_t i = tval[2] - 1; active_reforms.size() != 0 && i >= 0; --i) {
				std::uniform_int_distribution<int32_t> dist(0, int32_t(active_reforms.size()) - 1);
				auto chance_taken = uint32_t(dist(gen));
				issues::change_issue_option(ws, ws.s.issues_m.issues_container[active_reforms[chance_taken]].options[0], *((nations::nation*)primary_slot));
			}
		}
		void ef_add_crime(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->crime = trigger_payload(tval[2]).prov_mod;
		}
		void ef_add_crime_none(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->crime = modifiers::provincial_modifier_tag();
		}
		void ef_nationalize(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::perform_nationalization(ws, *((nations::nation*)primary_slot));
		}
		void ef_build_factory_in_capital_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			auto factory_type = &(ws.s.economy_m.factory_types[trigger_payload(tval[2]).small.values.factory]);
			if(is_valid_index(capital)) {
				if(auto si = ws.w.province_s.province_state_container[capital].state_instance; si) {
					int32_t free_index = -1;
					for(int32_t i = int32_t(std::extent_v<decltype(si->factories)>) - 1; i >= 0; --i) {
						if(si->factories[i].type == factory_type)
							return;
						else if(si->factories[i].type == nullptr)
							free_index = i;
					}
					if(free_index != -1) {
						si->factories[free_index].type = factory_type;
						si->factories[free_index].level = 1ui16;
					}
				}
			}
		}
		void ef_activate_technology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			bit_vector_set(ws.w.nation_s.active_technologies.get_row(((nations::nation*)primary_slot)->id), to_index(trigger_payload(tval[2]).tech), true);
			technologies::apply_single_technology(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tech);
		}
		void ef_great_wars_enabled_yes(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.great_wars_enabled = true;
		}
		void ef_great_wars_enabled_no(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.great_wars_enabled = false;
		}
		void ef_world_wars_enabled_yes(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.world_wars_enabled = true;
		}
		void ef_world_wars_enabled_no(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.world_wars_enabled = false;
		}
		void ef_assimilate_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner) {
				provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [owner_culture = owner->primary_culture](population::pop& p){
					p.culture = owner_culture;
				});
			}
		}
		void ef_assimilate_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = population::get_pop_owner(ws, *((population::pop*)primary_slot)); owner) {
				((population::pop*)primary_slot)->culture = owner->primary_culture;
			}
		}
		void ef_literacy(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::set_literacy_direct(*((population::pop*)primary_slot), population::get_literacy_direct(*((population::pop*)primary_slot)) + read_float_from_payload(tval + 2));
		}
		void ef_add_crisis_interest(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(ws.w.current_crisis.type != current_state::crisis_type::none)
				add_item(ws.w.nation_s.nations_arrays, ws.w.current_crisis.interested, ((nations::nation*)primary_slot)->id);
		}
		void ef_flashpoint_tension(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((nations::state_instance*)primary_slot)->current_tension = std::clamp(((nations::state_instance*)primary_slot)->current_tension + read_float_from_payload(tval + 2), 0.0f, 100.0f);
		}
		void ef_add_crisis_temperature(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.current_crisis.temperature = std::clamp(ws.w.current_crisis.temperature + read_float_from_payload(tval + 2), 0.0f, 100.0f);
		}
		void ef_consciousness(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::set_consciousness_direct(*((population::pop*)primary_slot), population::get_consciousness_direct(*((population::pop*)primary_slot)) + read_float_from_payload(tval + 2));
		}
		void ef_militancy(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			population::set_militancy_direct(*((population::pop*)primary_slot), population::get_militancy_direct(*((population::pop*)primary_slot)) + read_float_from_payload(tval + 2));
		}
		void ef_rgo_size(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->rgo_size = 
				uint8_t(std::clamp(int32_t(((provinces::province_state*)primary_slot)->rgo_size + trigger_payload(tval[2]).signed_value), 0, 255));
		}
		void ef_fort(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->fort_level =
				uint8_t(std::clamp(
					int32_t(((provinces::province_state*)primary_slot)->fort_level) + int32_t(trigger_payload(tval[2]).signed_value), 0, 255));
		}
		void ef_naval_base(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			((provinces::province_state*)primary_slot)->naval_base_level =
				uint8_t(std::clamp(
					int32_t(((provinces::province_state*)primary_slot)->naval_base_level) + int32_t(trigger_payload(tval[2]).signed_value), 0, 255));
		}
		void ef_trigger_revolt_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto rf_range = get_range(ws.w.population_s.rebel_faction_arrays, ((nations::nation*)primary_slot)->active_rebel_factions);

			auto type = trigger_payload(tval[2]).small.values.rebel;
			auto culture = trigger_payload(tval[3]).culture;
			auto religion = trigger_payload(tval[4]).small.values.religion;
			auto ideology = trigger_payload(tval[5]).small.values.ideology;

			for(auto rf : rf_range) {
				auto& rf_obj = ws.w.population_s.rebel_factions[rf];
				if((!is_valid_index(type) || rf_obj.type == type) &&
					(!is_valid_index(culture) || rf_obj.culture == culture) &&
					(!is_valid_index(religion) || rf_obj.religion == religion) &&
					(!is_valid_index(ideology) || rf_obj.ideology == ideology)) {
					population::trigger_rising(ws, rf_obj, *((nations::nation*)primary_slot));
				}
			}
		}
		void ef_trigger_revolt_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((nations::state_instance*)primary_slot)->owner; owner)
				ef_trigger_revolt_nation(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_trigger_revolt_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_trigger_revolt_nation(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_diplomatic_influence(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder; holder)
				nations::adjust_influence(ws, *((nations::nation*)primary_slot), holder->id, trigger_payload(tval[3]).signed_value);
		}
		void ef_diplomatic_influence_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::adjust_influence(ws, *((nations::nation*)primary_slot), ((nations::nation*)this_slot)->id, trigger_payload(tval[2]).signed_value);
		}
		void ef_diplomatic_influence_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner)
				nations::adjust_influence(ws, *((nations::nation*)primary_slot), owner->id, trigger_payload(tval[2]).signed_value);
		}
		void ef_diplomatic_influence_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::adjust_influence(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id, trigger_payload(tval[2]).signed_value);
		}
		void ef_diplomatic_influence_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner)
				nations::adjust_influence(ws, *((nations::nation*)primary_slot), owner->id, trigger_payload(tval[2]).signed_value);
		}
		void ef_relation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto holder = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder; holder)
				nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *holder, trigger_payload(tval[3]).signed_value);
		}
		void ef_relation_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *((nations::nation*)this_slot), trigger_payload(tval[2]).signed_value);
		}
		void ef_relation_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner)
				nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *owner, trigger_payload(tval[2]).signed_value);
		}
		void ef_relation_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *((nations::nation*)from_slot), trigger_payload(tval[2]).signed_value);
		}
		void ef_relation_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner)
				nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *owner, trigger_payload(tval[2]).signed_value);
		}
		void ef_add_province_modifier(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			modifiers::add_timed_modifier_to_province(
				ws,
				*((provinces::province_state*)primary_slot),
				trigger_payload(tval[2]).prov_mod,
				date_tag(to_index(ws.w.current_date) + trigger_payload(tval[3]).signed_value));
		}
		void ef_add_province_modifier_no_duration(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			modifiers::add_static_modifier_to_province(ws, *((provinces::province_state*)primary_slot), trigger_payload(tval[2]).prov_mod);
		}
		void ef_add_country_modifier(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			modifiers::add_timed_modifier_to_nation(
				ws,
				*((nations::nation*)primary_slot),
				trigger_payload(tval[2]).nat_mod,
				date_tag(to_index(ws.w.current_date) + trigger_payload(tval[3]).signed_value));
		}
		void ef_add_country_modifier_no_duration(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			modifiers::add_unique_static_modifier_to_nation(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).nat_mod);
		}
		void ef_casus_belli_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;
			auto tag_target = trigger_payload(tval[4]).tag;

			if(auto holder = ws.w.culture_s.national_tags_state[tag_target].holder; holder) {
				add_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						holder->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()});
			}
		}
		void ef_casus_belli_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ws.w.province_s.province_state_container[provinces::province_tag(tval[4])].owner; owner) {
				add_item(ws.w.military_s.cb_arrays, 
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						owner->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()});
			}
		}
		void ef_casus_belli_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			add_item(ws.w.military_s.cb_arrays, 
				((nations::nation*)primary_slot)->active_cbs,
				military::pending_cb {
					((nations::nation*)this_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()});	
		}
		void ef_casus_belli_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						owner->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_casus_belli_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						owner->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_casus_belli_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner) {
				add_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						owner->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_casus_belli_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			add_item(ws.w.military_s.cb_arrays,
				((nations::nation*)primary_slot)->active_cbs,
				military::pending_cb {
					((nations::nation*)from_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
			});
		}
		void ef_casus_belli_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {
						owner->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;
			auto tag_target = trigger_payload(tval[4]).tag;

			if(auto holder = ws.w.culture_s.national_tags_state[tag_target].holder; holder) {
				add_item(ws.w.military_s.cb_arrays,
					holder->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ws.w.province_s.province_state_container[provinces::province_tag(tval[4])].owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			add_item(ws.w.military_s.cb_arrays,
				((nations::nation*)this_slot)->active_cbs,
				military::pending_cb {
					((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
			});
		}
		void ef_add_casus_belli_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner) {
				add_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_add_casus_belli_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			add_item(ws.w.military_s.cb_arrays,
				((nations::nation*)from_slot)->active_cbs,
				military::pending_cb {
					((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
			});
		}
		void ef_add_casus_belli_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto months = trigger_payload(tval[3]).signed_value;

			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner) {
				add_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb {
						((nations::nation*)primary_slot)->id, type, months > 0 ? date_tag(to_index(ws.w.current_date) + months * 30) : date_tag()
				});
			}
		}
		void ef_remove_casus_belli_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto tag_target = trigger_payload(tval[3]).tag;

			if(auto holder = ws.w.culture_s.national_tags_state[tag_target].holder; holder) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {holder->id, type, date_tag()});
			}
		}
		void ef_remove_casus_belli_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ws.w.province_s.province_state_container[provinces::province_tag(tval[3])].owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb {owner->id, type, date_tag()});
			}
		}
		void ef_remove_casus_belli_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			remove_item(ws.w.military_s.cb_arrays,
				((nations::nation*)primary_slot)->active_cbs,
				military::pending_cb {((nations::nation*)this_slot)->id, type, date_tag()});
		}
		void ef_remove_casus_belli_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb { owner->id, type, date_tag() });
			}
		}
		void ef_remove_casus_belli_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb { owner->id, type, date_tag() });
			}
		}
		void ef_remove_casus_belli_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb { owner->id, type, date_tag() });
			}
		}
		void ef_remove_casus_belli_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			remove_item(ws.w.military_s.cb_arrays,
				((nations::nation*)primary_slot)->active_cbs,
				military::pending_cb {((nations::nation*)from_slot)->id, type, date_tag()});
		}
		void ef_remove_casus_belli_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					((nations::nation*)primary_slot)->active_cbs,
					military::pending_cb { owner->id, type, date_tag() });
			}
		}
		void ef_this_remove_casus_belli_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;
			auto tag_target = trigger_payload(tval[3]).tag;

			if(auto holder = ws.w.culture_s.national_tags_state[tag_target].holder; holder) {
				remove_item(ws.w.military_s.cb_arrays,
					holder->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag()});
			}
		}
		void ef_this_remove_casus_belli_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ws.w.province_s.province_state_container[provinces::province_tag(tval[3])].owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag()});
			}
		}
		void ef_this_remove_casus_belli_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			remove_item(ws.w.military_s.cb_arrays,
				((nations::nation*)this_slot)->active_cbs,
				military::pending_cb {((nations::nation*)primary_slot)->id, type, date_tag()});
		}
		void ef_this_remove_casus_belli_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag() });
			}
		}
		void ef_this_remove_casus_belli_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag() });
			}
		}
		void ef_this_remove_casus_belli_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner) {
				remove_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag() });
			}
		}
		void ef_this_remove_casus_belli_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			remove_item(ws.w.military_s.cb_arrays,
				((nations::nation*)from_slot)->active_cbs,
				military::pending_cb {((nations::nation*)primary_slot)->id, type, date_tag()});
		}
		void ef_this_remove_casus_belli_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto type = trigger_payload(tval[2]).small.values.cb_type;

			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner) {
				remove_item(ws.w.military_s.cb_arrays,
					owner->active_cbs,
					military::pending_cb { ((nations::nation*)primary_slot)->id, type, date_tag() });
			}
		}
		void ef_war_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto target = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(target) {
				auto& new_war = military::create_war(ws, *((nations::nation*)primary_slot), *target, true);

				auto attacker_wg_count = tval[3];
				uint32_t payload_index = 4;
				for(int32_t i = 0; i < attacker_wg_count; ++i) {
					auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
					auto province_id = tval[payload_index + 1];
					auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
					auto state = [&ws, province_id]() {
						if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
							return si->id;
						else
							return nations::state_tag();
					}();

					auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

					add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
						military::war_goal{ ws.w.current_date, 0.0f, ((nations::nation*)primary_slot)->id, state, target->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
					payload_index += 3;
				}

				auto defender_wg_count = tval[payload_index];
				++payload_index;
				for(int32_t i = 0; i < defender_wg_count; ++i) {
					auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
					auto province_id = tval[payload_index + 1];
					auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
					auto state = [&ws, province_id]() {
						if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
							return si->id;
						else
							return nations::state_tag();
					}();

					auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

					add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
						military::war_goal{ ws.w.current_date, 0.0f, target->id, state, ((nations::nation*)primary_slot)->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
					payload_index += 3;
				}
			}
		}
		void ef_war_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::nation* target = ((nations::nation*)this_slot);

			auto& new_war = military::create_war(ws, *((nations::nation*)primary_slot), *target, true);

			auto attacker_wg_count = tval[2];
			uint32_t payload_index = 3;
			for(int32_t i = 0; i < attacker_wg_count; ++i) {
				auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
				auto province_id = tval[payload_index + 1];
				auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
				auto state = [&ws, province_id]() {
					if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
						return si->id;
					else
						return nations::state_tag();
				}();

				auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

				add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
					military::war_goal{ ws.w.current_date, 0.0f, ((nations::nation*)primary_slot)->id, state, target->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
				payload_index += 3;
			}

			auto defender_wg_count = tval[payload_index];
			++payload_index;
			for(int32_t i = 0; i < defender_wg_count; ++i) {
				auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
				auto province_id = tval[payload_index + 1];
				auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
				auto state = [&ws, province_id]() {
					if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
						return si->id;
					else
						return nations::state_tag();
				}();

				auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

				add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
					military::war_goal{ ws.w.current_date, 0.0f, target->id, state, ((nations::nation*)primary_slot)->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
				payload_index += 3;
			}
		}
		void ef_war_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner)
				ef_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner)
				ef_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner)
				ef_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_war_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_war_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner)
				ef_war_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_no_ally_tag(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto target = ws.w.culture_s.national_tags_state[trigger_payload(tval[2]).tag].holder;
			if(target) {
				auto& new_war = military::create_war(ws, *((nations::nation*)primary_slot), *target, false);

				auto attacker_wg_count = tval[3];
				uint32_t payload_index = 4;
				for(int32_t i = 0; i < attacker_wg_count; ++i) {
					auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
					auto province_id = tval[payload_index + 1];
					auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
					auto state = [&ws, province_id]() {
						if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
							return si->id;
						else
							return nations::state_tag();
					}();

					auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

					add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
						military::war_goal{ ws.w.current_date, 0.0f, ((nations::nation*)primary_slot)->id, state, target->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
					payload_index += 3;
				}

				auto defender_wg_count = tval[payload_index];
				++payload_index;
				for(int32_t i = 0; i < defender_wg_count; ++i) {
					auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
					auto province_id = tval[payload_index + 1];
					auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
					auto state = [&ws, province_id]() {
						if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
							return si->id;
						else
							return nations::state_tag();
					}();

					auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

					add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
						military::war_goal{ ws.w.current_date, 0.0f, target->id, state, ((nations::nation*)primary_slot)->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
					payload_index += 3;
				}
			}
		}
		void ef_war_no_ally_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			nations::nation* target = ((nations::nation*)this_slot);

			auto& new_war = military::create_war(ws, *((nations::nation*)primary_slot), *target, false);

			auto attacker_wg_count = tval[2];
			uint32_t payload_index = 3;
			for(int32_t i = 0; i < attacker_wg_count; ++i) {
				auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
				auto province_id = tval[payload_index + 1];
				auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
				auto state = [&ws, province_id]() {
					if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
						return si->id;
					else
						return nations::state_tag();
				}();

				auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

				add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
					military::war_goal{ ws.w.current_date, 0.0f, ((nations::nation*)primary_slot)->id, state, target->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
				payload_index += 3;
			}

			auto defender_wg_count = tval[payload_index];
			++payload_index;
			for(int32_t i = 0; i < defender_wg_count; ++i) {
				auto cb_type = trigger_payload(tval[payload_index + 0]).small.values.cb_type;
				auto province_id = tval[payload_index + 1];
				auto liberate_country_tag = trigger_payload(tval[payload_index + 2]).tag;
				auto state = [&ws, province_id]() {
					if(auto si = ws.w.province_s.province_state_container[provinces::province_tag(province_id)].state_instance; si)
						return si->id;
					else
						return nations::state_tag();
				}();

				auto lib_holder = ws.w.culture_s.national_tags_state[liberate_country_tag].holder;

				add_item(ws.w.military_s.war_goal_arrays, new_war.war_goals,
					military::war_goal{ ws.w.current_date, 0.0f, target->id, state, ((nations::nation*)primary_slot)->id, bool(lib_holder) ? lib_holder->id : nations::country_tag(), cb_type });
				payload_index += 3;
			}
		}
		void ef_war_no_ally_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((nations::state_instance*)this_slot)->owner; owner)
				ef_war_no_ally_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_no_ally_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)this_slot)->owner; owner)
				ef_war_no_ally_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_no_ally_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = population::get_pop_owner(ws, *((population::pop*)this_slot)); owner)
				ef_war_no_ally_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_war_no_ally_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ef_war_no_ally_this_nation(tval, ws, primary_slot, from_slot, nullptr, nullptr, gen);
		}
		void ef_war_no_ally_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)from_slot)->owner; owner)
				ef_war_no_ally_this_nation(tval, ws, primary_slot, owner, nullptr, nullptr, gen);
		}
		void ef_country_event_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((nations::nation*)primary_slot)->id, ((nations::nation*)this_slot)->id);
		}
		void ef_country_event_immediate_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((nations::nation*)primary_slot)->id, ((nations::nation*)this_slot)->id);
		}
		void ef_province_event_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((provinces::province_state*)primary_slot)->id, ((nations::nation*)this_slot)->id);
		}
		void ef_province_event_immediate_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((provinces::province_state*)primary_slot)->id, ((nations::nation*)this_slot)->id);
		}
		void ef_country_event_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((nations::nation*)primary_slot)->id, ((nations::state_instance*)this_slot)->id);
		}
		void ef_country_event_immediate_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((nations::nation*)primary_slot)->id, ((nations::state_instance*)this_slot)->id);
		}
		void ef_province_event_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((provinces::province_state*)primary_slot)->id, ((nations::state_instance*)this_slot)->id);
		}
		void ef_province_event_immediate_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((provinces::province_state*)primary_slot)->id, ((nations::state_instance*)this_slot)->id);
		}
		void ef_country_event_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((nations::nation*)primary_slot)->id, ((provinces::province_state*)this_slot)->id);
		}
		void ef_country_event_immediate_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((nations::nation*)primary_slot)->id, ((provinces::province_state*)this_slot)->id);
		}
		void ef_province_event_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((provinces::province_state*)primary_slot)->id, ((provinces::province_state*)this_slot)->id);
		}
		void ef_province_event_immediate_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((provinces::province_state*)primary_slot)->id, ((provinces::province_state*)this_slot)->id);
		}
		void ef_country_event_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((nations::nation*)primary_slot)->id, ((population::pop*)this_slot)->id);
		}
		void ef_country_event_immediate_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((nations::nation*)primary_slot)->id, ((population::pop*)this_slot)->id);
		}
		void ef_province_event_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_delayed_event(ws, trigger_payload(tval[2]).event, tval[3], ((provinces::province_state*)primary_slot)->id, ((population::pop*)this_slot)->id);
		}
		void ef_province_event_immediate_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			events::fire_event(ws, trigger_payload(tval[2]).event, ((provinces::province_state*)primary_slot)->id, ((population::pop*)this_slot)->id);
		}
		void ef_country_event_province_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_this_nation(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_immediate_province_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_immediate_this_nation(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_province_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_this_state(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_immediate_province_this_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_immediate_this_state(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_province_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_this_province(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_immediate_province_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_immediate_this_province(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_province_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_this_pop(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_country_event_immediate_province_this_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_country_event_immediate_this_pop(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		
		void ef_sub_unit_int(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_sub_unit_this(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			//do nothing
		}
		void ef_sub_unit_from(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_sub_unit_current(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			// do nothing
		}
		void ef_set_variable(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.nation_s.national_variables.get(((nations::nation*)primary_slot)->id, trigger_payload(tval[2]).nat_var) = read_float_from_payload(tval + 3);
		}
		void ef_change_variable(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.nation_s.national_variables.get(((nations::nation*)primary_slot)->id, trigger_payload(tval[2]).nat_var) += read_float_from_payload(tval + 3);
		}
		void ef_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology = trigger_payload(tval[2]).small.values.ideology;
			auto factor = read_float_from_payload(tval + 3);

			auto pop_id = ((population::pop*)primary_slot)->id;
			auto total_pop = ws.w.population_s.pop_demographics.get(pop_id, population::total_population_tag);
			auto support = ws.w.population_s.pop_demographics.get_row(pop_id) + to_index(population::to_demo_tag(ws, ideologies::ideology_tag(0)));

			support[to_index(ideology)] = std::max(0, support[to_index(ideology)] + int32_t(total_pop * factor));
			normalize_integer_vector(support, ws.s.ideologies_m.ideologies_count, total_pop);
		}
		void ef_upper_house(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology = trigger_payload(tval[2]).small.values.ideology;
			auto amount = trigger_payload(tval[3]).signed_value;
			auto upper_house = ws.w.nation_s.upper_house.get_row(((nations::nation*)primary_slot)->id);

			upper_house[to_index(ideology)] = uint8_t(std::clamp(int32_t(upper_house[to_index(ideology)]) + amount, 0, 255));
			normalize_integer_vector(upper_house, ws.s.ideologies_m.ideologies_count, 100ui8);
		}
		void ef_scaled_militancy_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto issue_demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);
			
			if(pop_size != 0) {
				auto support = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, issue_demo_tag);
				float adjustment = read_float_from_payload(tval + 3) * float(support) / float(pop_size);
				population::set_militancy_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_militancy_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_militancy_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology_demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);

			if(pop_size != 0) {
				auto support = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, ideology_demo_tag);
				float adjustment = read_float_from_payload(tval + 3) * float(support) / float(pop_size);
				population::set_militancy_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_militancy_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_militancy_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);

			if(pop_size != 0) {
				auto unemployed = pop_size - ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_employment_tag);
				float adjustment = read_float_from_payload(tval + 2) * float(unemployed) / float(pop_size);
				population::set_militancy_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_militancy_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_consciousness_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto issue_demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);

			if(pop_size != 0) {
				auto support = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, issue_demo_tag);
				float adjustment = read_float_from_payload(tval + 3) * float(support) / float(pop_size);
				population::set_consciousness_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_consciousness_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_consciousness_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology_demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);

			if(pop_size != 0) {
				auto support = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, ideology_demo_tag);
				float adjustment = read_float_from_payload(tval + 3) * float(support) / float(pop_size);
				population::set_consciousness_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_consciousness_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_consciousness_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto pop_size = ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_population_tag);

			if(pop_size != 0) {
				auto unemployed = pop_size - ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, population::total_employment_tag);
				float adjustment = read_float_from_payload(tval + 2) * float(unemployed) / float(pop_size);
				population::set_consciousness_direct(*((population::pop*)primary_slot), std::clamp(adjustment + population::get_consciousness_direct(*((population::pop*)primary_slot)), 0.0f, 1.0f));
			}
		}
		void ef_scaled_militancy_nation_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_nation_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_nation_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_nation_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_nation_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_nation_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_state_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_state_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_state_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_state_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_state_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_state_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_province_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_province_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_militancy_province_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_militancy_direct(p, std::clamp(adjustment + population::get_militancy_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_province_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto factor = read_float_from_payload(tval + 3);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_province_ideology(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto demo_tag = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.ideology);
			auto factor = read_float_from_payload(tval + 3);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, demo_tag, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = ws.w.population_s.pop_demographics.get(p.id, demo_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_scaled_consciousness_province_unemployment(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto factor = read_float_from_payload(tval + 2);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, factor](population::pop& p) {
				auto pop_size = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);

				if(pop_size != 0) {
					auto support = pop_size - ws.w.population_s.pop_demographics.get(p.id, population::total_employment_tag);
					float adjustment = factor * float(support) / float(pop_size);
					population::set_consciousness_direct(p, std::clamp(adjustment + population::get_consciousness_direct(p), 0.0f, 1.0f));
				}
			});
		}
		void ef_variable_good_name(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			ws.w.nation_s.national_stockpiles.get(((nations::nation*)primary_slot)->id, trigger_payload(tval[2]).small.values.good) += economy::goods_qnty_type(read_float_from_payload(tval + 3));
		}
		void ef_variable_good_name_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ws.w.nation_s.national_stockpiles.get(owner->id, trigger_payload(tval[2]).small.values.good) += economy::goods_qnty_type(read_float_from_payload(tval + 3));
		}
		void ef_define_general(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto& new_leader = military::make_empty_leader(ws, ((nations::nation*)primary_slot)->primary_culture, true);
			new_leader.creation_date = ws.w.current_date;
			new_leader.personality = trigger_payload(tval[3]).small.values.leader_trait;
			new_leader.background = trigger_payload(tval[4]).small.values.leader_trait;
			military::calculate_leader_traits(ws, new_leader);

			add_item(ws.w.military_s.leader_arrays, ((nations::nation*)primary_slot)->generals, new_leader.id);
		}
		void ef_define_admiral(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto& new_leader = military::make_empty_leader(ws, ((nations::nation*)primary_slot)->primary_culture, false);
			new_leader.creation_date = ws.w.current_date;
			new_leader.personality = trigger_payload(tval[3]).small.values.leader_trait;
			new_leader.background = trigger_payload(tval[4]).small.values.leader_trait;
			military::calculate_leader_traits(ws, new_leader);

			add_item(ws.w.military_s.leader_arrays, ((nations::nation*)primary_slot)->admirals, new_leader.id);
		}
		void ef_dominant_issue(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto opt = trigger_payload(tval[2]).small.values.option;
			auto factor = read_float_from_payload(tval + 3);

			auto pop_id = ((population::pop*)primary_slot)->id;
			auto total_pop = ws.w.population_s.pop_demographics.get(pop_id, population::total_population_tag);
			auto support = ws.w.population_s.pop_demographics.get_row(pop_id) + to_index(population::to_demo_tag(ws, issues::option_tag(0)));

			support[to_index(opt)] = std::max(0, support[to_index(opt)] + int32_t(total_pop * factor));
			normalize_integer_vector(support, ws.s.issues_m.tracked_options_count, total_pop);
		}
		void ef_dominant_issue_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto opt = trigger_payload(tval[2]).small.values.option;
			auto factor = read_float_from_payload(tval + 3);
			auto first_issue_index = to_index(population::to_demo_tag(ws, issues::option_tag(0)));

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, opt, factor, first_issue_index](population::pop& p) {
				auto total_pop = ws.w.population_s.pop_demographics.get(p.id, population::total_population_tag);
				auto support = ws.w.population_s.pop_demographics.get_row(p.id) + first_issue_index;

				support[to_index(opt)] = std::max(0, support[to_index(opt)] + int32_t(total_pop * factor));
				normalize_integer_vector(support, ws.s.issues_m.tracked_options_count, total_pop);
			});
		}
		void ef_add_war_goal(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto w = military::get_war_between(ws, *((nations::nation*)primary_slot), ((nations::nation*)from_slot)->id); w) {
				add_item(ws.w.military_s.war_goal_arrays, w->war_goals,
					military::war_goal{
						ws.w.current_date,
						0.0f,
						((nations::nation*)from_slot)->id,
						nations::state_tag(),
						((nations::nation*)primary_slot)->id,
						nations::country_tag(),
						trigger_payload(tval[2]).small.values.cb_type});
			}
		}
		void ef_move_issue_percentage_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto from_issue = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto to_issue = population::to_demo_tag(ws, trigger_payload(tval[3]).small.values.option);
			auto amount = read_float_from_payload(tval + 4);

			nations::for_each_pop(ws, *((nations::nation*)primary_slot), [&ws, from_issue, to_issue, amount](population::pop& p) {
				auto subtract = int32_t(ws.w.population_s.pop_demographics.get(p.id, from_issue) * amount);
				ws.w.population_s.pop_demographics.get(p.id, from_issue) -= subtract;
				ws.w.population_s.pop_demographics.get(p.id, to_issue) += subtract;
			});
		}
		void ef_move_issue_percentage_state(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto from_issue = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto to_issue = population::to_demo_tag(ws, trigger_payload(tval[3]).small.values.option);
			auto amount = read_float_from_payload(tval + 4);

			nations::for_each_pop(ws, *((nations::state_instance*)primary_slot), [&ws, from_issue, to_issue, amount](population::pop& p) {
				auto subtract = int32_t(ws.w.population_s.pop_demographics.get(p.id, from_issue) * amount);
				ws.w.population_s.pop_demographics.get(p.id, from_issue) -= subtract;
				ws.w.population_s.pop_demographics.get(p.id, to_issue) += subtract;
			});
		}
		void ef_move_issue_percentage_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto from_issue = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto to_issue = population::to_demo_tag(ws, trigger_payload(tval[3]).small.values.option);
			auto amount = read_float_from_payload(tval + 4);

			provinces::for_each_pop(ws, *((provinces::province_state*)primary_slot), [&ws, from_issue, to_issue, amount](population::pop& p) {
				auto subtract = int32_t(ws.w.population_s.pop_demographics.get(p.id, from_issue) * amount);
				ws.w.population_s.pop_demographics.get(p.id, from_issue) -= subtract;
				ws.w.population_s.pop_demographics.get(p.id, to_issue) += subtract;
			});
		}
		void ef_move_issue_percentage_pop(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto from_issue = population::to_demo_tag(ws, trigger_payload(tval[2]).small.values.option);
			auto to_issue = population::to_demo_tag(ws, trigger_payload(tval[3]).small.values.option);
			auto amount = read_float_from_payload(tval + 4);

			auto subtract = int32_t(ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, from_issue) * amount);
			ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, from_issue) -= subtract;
			ws.w.population_s.pop_demographics.get(((population::pop*)primary_slot)->id, to_issue) += subtract;
		}
		void ef_party_loyalty_nation_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology = trigger_payload(tval[2]).small.values.ideology;
			auto amount = read_float_from_payload(tval + 3);
			ws.w.province_s.party_loyalty.get(((provinces::province_state*)from_slot)->id, ideology) += amount;
		}
		void ef_party_loyalty_province_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto ideology = trigger_payload(tval[2]).small.values.ideology;
			auto amount = read_float_from_payload(tval + 3);
			ws.w.province_s.party_loyalty.get(((provinces::province_state*)primary_slot)->id, ideology) += amount;
		}
		void ef_party_loyalty_province_id_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::province_tag prov(tval[2]);
			auto ideology = trigger_payload(tval[3]).small.values.ideology;
			auto amount = read_float_from_payload(tval + 4);
			ws.w.province_s.party_loyalty.get(prov, ideology) += amount;
		}
		void ef_party_loyalty_province_id_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			provinces::province_tag prov(tval[2]);
			auto ideology = trigger_payload(tval[3]).small.values.ideology;
			auto amount = read_float_from_payload(tval + 4);
			ws.w.province_s.party_loyalty.get(prov, ideology) += amount;
		}
		void ef_build_railway_in_capital_yes_whole_state_yes_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				if(auto si = ws.w.province_s.province_state_container[capital].state_instance; si) {
					nations::for_each_province(ws, *si, [](provinces::province_state& p) {
						if(p.modifier_values[modifiers::provincial_offsets::min_build_railroad] <= 1.0f)
							++p.railroad_level;
					});
				}
			}
		}
		void ef_build_railway_in_capital_yes_whole_state_no_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				if(auto si = ws.w.province_s.province_state_container[capital].state_instance; si) {
					nations::for_each_province(ws, *si, [](provinces::province_state& p) {
						if(p.modifier_values[modifiers::provincial_offsets::min_build_railroad] <= 1.0f)
							++p.railroad_level;
					});
				}
			}
		}
		void ef_build_railway_in_capital_no_whole_state_yes_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				auto& p = ws.w.province_s.province_state_container[capital];
				if(p.modifier_values[modifiers::provincial_offsets::min_build_railroad] <= 1.0f)
					++p.railroad_level;
			}
		}
		void ef_build_railway_in_capital_no_whole_state_no_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				auto& p = ws.w.province_s.province_state_container[capital];
				if(p.modifier_values[modifiers::provincial_offsets::min_build_railroad] <= 1.0f)
					++p.railroad_level;
			}
		}
		void ef_build_fort_in_capital_yes_whole_state_yes_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				if(auto si = ws.w.province_s.province_state_container[capital].state_instance; si) {
					nations::for_each_province(ws, *si, [](provinces::province_state& p) {
						++p.fort_level;
					});
				}
			}
		}
		void ef_build_fort_in_capital_yes_whole_state_no_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				if(auto si = ws.w.province_s.province_state_container[capital].state_instance; si) {
					nations::for_each_province(ws, *si, [](provinces::province_state& p) {
						++p.fort_level;
					});
				}
			}
		}
		void ef_build_fort_in_capital_no_whole_state_yes_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				auto& p = ws.w.province_s.province_state_container[capital];
				++p.fort_level;
			}
		}
		void ef_build_fort_in_capital_no_whole_state_no_limit(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			auto capital = ((nations::nation*)primary_slot)->current_capital;
			if(is_valid_index(capital)) {
				auto& p = ws.w.province_s.province_state_container[capital];
				++p.fort_level;
			}
		}
		void ef_relation_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto holder = ws.w.culture_s.national_tags_state[rebel_slot->independence_tag].holder; holder)
				nations::adjust_relationship(ws, *((nations::nation*)primary_slot), *holder, trigger_payload(tval[2]).signed_value);
		}
		void ef_variable_tech_name(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			technologies::apply_single_technology(ws, *((nations::nation*)primary_slot), trigger_payload(tval[2]).tech);
		}
		void ef_set_country_flag_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_set_country_flag(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_add_country_modifier_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_add_country_modifier(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_add_country_modifier_province_no_duration(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_add_country_modifier_no_duration(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_relation_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation(tval, ws, owner, nullptr, nullptr, nullptr, gen);
		}
		void ef_relation_province_this_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation_this_nation(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_relation_province_this_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation_this_province(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}
		void ef_relation_province_from_nation(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation_this_nation(tval, ws, owner, from_slot, nullptr, nullptr, gen);
		}
		void ef_relation_province_from_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation_this_province(tval, ws, owner, from_slot, nullptr, nullptr, gen);
		}
		void ef_relation_province_reb(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_relation_reb(tval, ws, owner, nullptr, nullptr, rebel_slot, gen);
		}
		void ef_treasury_province(uint16_t const* tval, world_state& ws, void* primary_slot, void* this_slot, void* from_slot, population::rebel_faction* rebel_slot, jsf_prng& gen) {
			if(auto owner = ((provinces::province_state*)primary_slot)->owner; owner)
				ef_treasury(tval, ws, owner, this_slot, nullptr, nullptr, gen);
		}

		static void(*effect_functions[])(uint16_t const*, world_state&, void*, void*, void*, population::rebel_faction*, jsf_prng&) = {
			ef_none,
			ef_capital,
			ef_add_core_tag,
			ef_add_core_int,
			ef_add_core_this_nation,
			ef_add_core_this_province,
			ef_add_core_this_state,
			ef_add_core_this_pop,
			ef_add_core_from_province,
			ef_add_core_from_nation,
			ef_add_core_reb,
			ef_remove_core_tag,
			ef_remove_core_int,
			ef_remove_core_this_nation,
			ef_remove_core_this_province,
			ef_remove_core_this_state,
			ef_remove_core_this_pop,
			ef_remove_core_from_province,
			ef_remove_core_from_nation,
			ef_remove_core_reb,
			ef_change_region_name_state,
			ef_change_region_name_province,
			ef_trade_goods,
			ef_add_accepted_culture,
			ef_add_accepted_culture_union,
			ef_primary_culture,
			ef_primary_culture_this_nation,
			ef_primary_culture_this_state,
			ef_primary_culture_this_province,
			ef_primary_culture_this_pop,
			ef_primary_culture_from_nation,
			ef_remove_accepted_culture,
			ef_life_rating,
			ef_religion,
			ef_is_slave_state_yes,
			ef_is_slave_pop_yes,
			ef_research_points,
			ef_tech_school,
			ef_government,
			ef_government_reb,
			ef_treasury,
			ef_war_exhaustion,
			ef_prestige,
			ef_change_tag,
			ef_change_tag_culture,
			ef_change_tag_no_core_switch,
			ef_change_tag_no_core_switch_culture,
			ef_set_country_flag,
			ef_clr_country_flag,
			ef_military_access,
			ef_military_access_this_nation,
			ef_military_access_this_province,
			ef_military_access_from_nation,
			ef_military_access_from_province,
			ef_badboy,
			ef_secede_province,
			ef_secede_province_this_nation,
			ef_secede_province_this_state,
			ef_secede_province_this_province,
			ef_secede_province_this_pop,
			ef_secede_province_from_nation,
			ef_secede_province_from_province,
			ef_secede_province_reb,
			ef_inherit,
			ef_inherit_this_nation,
			ef_inherit_this_state,
			ef_inherit_this_province,
			ef_inherit_this_pop,
			ef_inherit_from_nation,
			ef_inherit_from_province,
			ef_annex_to,
			ef_annex_to_this_nation,
			ef_annex_to_this_state,
			ef_annex_to_this_province,
			ef_annex_to_this_pop,
			ef_annex_to_from_nation,
			ef_annex_to_from_province,
			ef_release,
			ef_release_this_nation,
			ef_release_this_state,
			ef_release_this_province,
			ef_release_this_pop,
			ef_release_from_nation,
			ef_release_from_province,
			ef_change_controller,
			ef_change_controller_this_nation,
			ef_change_controller_this_province,
			ef_change_controller_from_nation,
			ef_change_controller_from_province,
			ef_infrastructure,
			ef_money,
			ef_leadership,
			ef_create_vassal,
			ef_create_vassal_this_nation,
			ef_create_vassal_this_province,
			ef_create_vassal_from_nation,
			ef_create_vassal_from_province,
			ef_end_military_access,
			ef_end_military_access_this_nation,
			ef_end_military_access_this_province,
			ef_end_military_access_from_nation,
			ef_end_military_access_from_province,
			ef_leave_alliance,
			ef_leave_alliance_this_nation,
			ef_leave_alliance_this_province,
			ef_leave_alliance_from_nation,
			ef_leave_alliance_from_province,
			ef_end_war,
			ef_end_war_this_nation,
			ef_end_war_this_province,
			ef_end_war_from_nation,
			ef_end_war_from_province,
			ef_enable_ideology,
			ef_ruling_party_ideology,
			ef_plurality,
			ef_remove_province_modifier,
			ef_remove_country_modifier,
			ef_create_alliance,
			ef_create_alliance_this_nation,
			ef_create_alliance_this_province,
			ef_create_alliance_from_nation,
			ef_create_alliance_from_province,
			ef_release_vassal,
			ef_release_vassal_this_nation,
			ef_release_vassal_this_province,
			ef_release_vassal_from_nation,
			ef_release_vassal_from_province,
			ef_release_vassal_reb,
			ef_release_vassal_random,
			ef_change_province_name,
			ef_enable_canal,
			ef_set_global_flag,
			ef_clr_global_flag,
			ef_nationalvalue_province,
			ef_nationalvalue_nation,
			ef_civilized_yes,
			ef_civilized_no,
			ef_is_slave_state_no,
			ef_is_slave_pop_no,
			ef_election,
			ef_social_reform,
			ef_political_reform,
			ef_add_tax_relative_income,
			ef_neutrality,
			ef_reduce_pop,
			ef_move_pop,
			ef_pop_type,
			ef_years_of_research,
			ef_prestige_factor_positive,
			ef_prestige_factor_negative,
			ef_military_reform,
			ef_economic_reform,
			ef_remove_random_military_reforms,
			ef_remove_random_economic_reforms,
			ef_add_crime,
			ef_add_crime_none,
			ef_nationalize,
			ef_build_factory_in_capital_state,
			ef_activate_technology,
			ef_great_wars_enabled_yes,
			ef_great_wars_enabled_no,
			ef_world_wars_enabled_yes,
			ef_world_wars_enabled_no,
			ef_assimilate_province,
			ef_assimilate_pop,
			ef_literacy,
			ef_add_crisis_interest,
			ef_flashpoint_tension,
			ef_add_crisis_temperature,
			ef_consciousness,
			ef_militancy,
			ef_rgo_size,
			ef_fort,
			ef_naval_base,
			ef_trigger_revolt_nation,
			ef_trigger_revolt_state,
			ef_trigger_revolt_province,
			ef_diplomatic_influence,
			ef_diplomatic_influence_this_nation,
			ef_diplomatic_influence_this_province,
			ef_diplomatic_influence_from_nation,
			ef_diplomatic_influence_from_province,
			ef_relation,
			ef_relation_this_nation,
			ef_relation_this_province,
			ef_relation_from_nation,
			ef_relation_from_province,
			ef_add_province_modifier,
			ef_add_province_modifier_no_duration,
			ef_add_country_modifier,
			ef_add_country_modifier_no_duration,
			ef_casus_belli_tag,
			ef_casus_belli_int,
			ef_casus_belli_this_nation,
			ef_casus_belli_this_state,
			ef_casus_belli_this_province,
			ef_casus_belli_this_pop,
			ef_casus_belli_from_nation,
			ef_casus_belli_from_province,
			ef_add_casus_belli_tag,
			ef_add_casus_belli_int,
			ef_add_casus_belli_this_nation,
			ef_add_casus_belli_this_state,
			ef_add_casus_belli_this_province,
			ef_add_casus_belli_this_pop,
			ef_add_casus_belli_from_nation,
			ef_add_casus_belli_from_province,
			ef_remove_casus_belli_tag,
			ef_remove_casus_belli_int,
			ef_remove_casus_belli_this_nation,
			ef_remove_casus_belli_this_state,
			ef_remove_casus_belli_this_province,
			ef_remove_casus_belli_this_pop,
			ef_remove_casus_belli_from_nation,
			ef_remove_casus_belli_from_province,
			ef_this_remove_casus_belli_tag,
			ef_this_remove_casus_belli_int,
			ef_this_remove_casus_belli_this_nation,
			ef_this_remove_casus_belli_this_state,
			ef_this_remove_casus_belli_this_province,
			ef_this_remove_casus_belli_this_pop,
			ef_this_remove_casus_belli_from_nation,
			ef_this_remove_casus_belli_from_province,
			ef_war_tag,
			ef_war_this_nation,
			ef_war_this_state,
			ef_war_this_province,
			ef_war_this_pop,
			ef_war_from_nation,
			ef_war_from_province,
			ef_war_no_ally_tag,
			ef_war_no_ally_this_nation,
			ef_war_no_ally_this_state,
			ef_war_no_ally_this_province,
			ef_war_no_ally_this_pop,
			ef_war_no_ally_from_nation,
			ef_war_no_ally_from_province,
			ef_country_event_this_nation,
			ef_country_event_immediate_this_nation,
			ef_province_event_this_nation,
			ef_province_event_immediate_this_nation,
			ef_sub_unit_int,
			ef_sub_unit_this,
			ef_sub_unit_from,
			ef_sub_unit_current,
			ef_set_variable,
			ef_change_variable,
			ef_ideology,
			ef_upper_house,
			ef_scaled_militancy_issue,
			ef_scaled_militancy_ideology,
			ef_scaled_militancy_unemployment,
			ef_scaled_consciousness_issue,
			ef_scaled_consciousness_ideology,
			ef_scaled_consciousness_unemployment,
			ef_define_general,
			ef_define_admiral,
			ef_dominant_issue,
			ef_add_war_goal,
			ef_move_issue_percentage_nation,
			ef_move_issue_percentage_state,
			ef_move_issue_percentage_province,
			ef_move_issue_percentage_pop,
			ef_party_loyalty_nation_from_province,
			ef_party_loyalty_province_from_nation,
			ef_party_loyalty_province_id_nation,
			ef_party_loyalty_province_id_from_nation,
			ef_build_railway_in_capital_yes_whole_state_yes_limit,
			ef_build_railway_in_capital_yes_whole_state_no_limit,
			ef_build_railway_in_capital_no_whole_state_yes_limit,
			ef_build_railway_in_capital_no_whole_state_no_limit,
			ef_build_fort_in_capital_yes_whole_state_yes_limit,
			ef_build_fort_in_capital_yes_whole_state_no_limit,
			ef_build_fort_in_capital_no_whole_state_yes_limit,
			ef_build_fort_in_capital_no_whole_state_no_limit,
			ef_relation_reb,
			ef_variable_tech_name,
			ef_variable_good_name,
			ef_set_country_flag_province,
			ef_add_country_modifier_province,
			ef_add_country_modifier_province_no_duration,
			ef_dominant_issue_nation,
			ef_relation_province,
			ef_relation_province_this_nation,
			ef_relation_province_this_province,
			ef_relation_province_from_nation,
			ef_relation_province_from_province,
			ef_relation_province_reb,
			ef_scaled_militancy_nation_issue,
			ef_scaled_militancy_nation_ideology,
			ef_scaled_militancy_nation_unemployment,
			ef_scaled_consciousness_nation_issue,
			ef_scaled_consciousness_nation_ideology,
			ef_scaled_consciousness_nation_unemployment,
			ef_scaled_militancy_state_issue,
			ef_scaled_militancy_state_ideology,
			ef_scaled_militancy_state_unemployment,
			ef_scaled_consciousness_state_issue,
			ef_scaled_consciousness_state_ideology,
			ef_scaled_consciousness_state_unemployment,
			ef_scaled_militancy_province_issue,
			ef_scaled_militancy_province_ideology,
			ef_scaled_militancy_province_unemployment,
			ef_scaled_consciousness_province_issue,
			ef_scaled_consciousness_province_ideology,
			ef_scaled_consciousness_province_unemployment,
			ef_variable_good_name_province,
			ef_treasury_province,
			ef_country_event_this_state,
			ef_country_event_immediate_this_state,
			ef_province_event_this_state,
			ef_province_event_immediate_this_state,
			ef_country_event_this_province,
			ef_country_event_immediate_this_province,
			ef_province_event_this_province,
			ef_province_event_immediate_this_province,
			ef_country_event_this_pop,
			ef_country_event_immediate_this_pop,
			ef_province_event_this_pop,
			ef_province_event_immediate_this_pop,
			ef_country_event_province_this_nation,
			ef_country_event_immediate_province_this_nation,
			ef_country_event_province_this_state,
			ef_country_event_immediate_province_this_state,
			ef_country_event_province_this_province,
			ef_country_event_immediate_province_this_province,
			ef_country_event_province_this_pop,
			ef_country_event_immediate_province_this_pop,
		};

		void _execute_effect(
			uint16_t const* tval,
			world_state& ws,
			void* primary_slot,
			void* this_slot,
			void* from_slot,
			population::rebel_faction* rebel_slot,
			jsf_prng& generator_copy) {

			if((*tval & effect_codes::is_scope) != 0) {
				effect_scope_functions[*tval & effect_codes::code_mask](tval, ws, primary_slot, this_slot, from_slot, rebel_slot, generator_copy);
			} else {
				effect_functions[*tval & effect_codes::code_mask](tval, ws, primary_slot, this_slot, from_slot, rebel_slot, generator_copy);
			}
		}
	}

#ifdef __llvm__
#pragma clang diagnostic pop
#endif

	void execute_effect(
		uint16_t const* tval,
		world_state& ws,
		void* primary_slot,
		void* this_slot,
		void* from_slot,
		population::rebel_faction* rebel_slot,
		jsf_prng generator_copy) {

		_execute_effect(tval, ws, primary_slot, this_slot, from_slot, rebel_slot, generator_copy);
	}
}
