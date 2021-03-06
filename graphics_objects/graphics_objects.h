#pragma once
#include <stdint.h>
#include <functional>
#include "common\\common.h"
#include "common\\shared_tags.h"

namespace graphics {
	enum class errors {
		unknown_attribute,
		non_square_border_size,
		invalid_number_of_frames,
		invalid_click_sound,
		unknown_file_level_type,
		unknown_sprite_type
	};

	const char* format_error(errors  e);

	enum class object_type : uint8_t {
		 generic_sprite = 0x00,
		 bordered_rect = 0x01,
		 horizontal_progress_bar = 0x02,
		 vertical_progress_bar = 0x03,
		 flag_mask = 0x04,
		 tile_sprite = 0x05,
		 text_sprite = 0x06,
		 barchart = 0x07,
		 piechart = 0x08,
		 linegraph = 0x09
	};

	struct xy_pair {
		int16_t x = 0;
		int16_t y = 0;
	};

	struct object {
		constexpr static uint8_t always_transparent = 0x10;
		constexpr static uint8_t flip_v = 0x20;
		constexpr static uint8_t has_click_sound = 0x40;
		constexpr static uint8_t do_transparency_check = 0x80;

		constexpr static uint8_t type_mask = 0x0F;

		xy_pair size; //4bytes
		
		texture_tag primary_texture_handle; //6bytes
		uint16_t type_dependant = 0; // secondary texture handle or border size -- 8bytes

		uint8_t flags = 0; //9bytes
		uint8_t number_of_frames = 1; //10bytes
	};

	struct name_maps {
		boost::container::flat_map<std::string, obj_definition_tag> names;
	};

	struct object_definitions {
		tagged_vector<object, obj_definition_tag> definitions;

		obj_definition_tag standard_text_background;
		obj_definition_tag small_text_background;
		obj_definition_tag edit_cursor;
	};

	obj_definition_tag reserve_graphics_object(name_maps& nmaps, const char* name_start, const char* name_end);

	using texture_lookup = std::function<texture_tag(const char*, const char*)>;
};

