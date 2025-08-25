/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

namespace cimbar
{
	struct conf
	{
		unsigned color_bits = 0;
		unsigned symbol_bits = 0;
		unsigned ecc_bytes = 0;
		unsigned ecc_block_size = 0;
		unsigned image_size_x = 1;
		unsigned image_size_y = image_size_x;

		unsigned cell_size = 8;
		unsigned cell_spacing_x = cell_size+1;
		unsigned cell_spacing_y = cell_size+1;
		unsigned cell_offset = 9;
		unsigned cells_per_col_x = 0;
		unsigned cells_per_col_y = cells_per_col_x;

		int fountain_chunks_per_frame = 2;
		bool legacy_mode = false;
	};

	struct Conf5x5
	{
		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 2;
		static constexpr unsigned ecc_bytes = 40;
		static constexpr unsigned ecc_block_size = 216;
		static constexpr unsigned image_size_x = 988;
		static constexpr unsigned image_size_y = image_size_x;

		static constexpr unsigned cell_size = 5;
		static constexpr unsigned cell_spacing_x = cell_size+1;
		static constexpr unsigned cell_spacing_y = cell_size+1;
		static constexpr unsigned cell_offset = 9;
		static constexpr unsigned cells_per_col_x = 162;
		static constexpr unsigned cells_per_col_y = cells_per_col_x;

		static constexpr int fountain_chunks_per_frame = 3;
	};

	struct Conf5x5d
	{
		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 2;
		static constexpr unsigned ecc_bytes = 35;
		static constexpr unsigned ecc_block_size = 182;
		static constexpr unsigned image_size_x = 958;
		static constexpr unsigned image_size_y = image_size_x;

		static constexpr unsigned cell_size = 5;
		static constexpr unsigned cell_spacing_x = cell_size;
		static constexpr unsigned cell_spacing_y = cell_size+1;
		static constexpr unsigned cell_offset = 9;
		static constexpr unsigned cells_per_col_x = 188;
		static constexpr unsigned cells_per_col_y = 157;

		static constexpr int fountain_chunks_per_frame = 4;
	};

	struct Conf8x8 : conf
	{
		Conf8x8()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 30;
			ecc_block_size = 155;
			image_size_x = 1024;
			image_size_y = image_size_x;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 8;
			cells_per_col_x = 112;
			cells_per_col_y = cells_per_col_x;

			fountain_chunks_per_frame = 2;
		}
	};

	struct Conf8x8_mini
	{
		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 4;
		static constexpr unsigned ecc_bytes = 36;
		static constexpr unsigned ecc_block_size = 179;
		static constexpr unsigned image_size_x = 1024;
		static constexpr unsigned image_size_y = 720;

		static constexpr unsigned cell_size = 8;
		static constexpr unsigned cell_spacing_x = cell_size+1;
		static constexpr unsigned cell_spacing_y = cell_size+1;
		static constexpr unsigned cell_offset = 9;
		static constexpr unsigned cells_per_col_x = 112;
		static constexpr unsigned cells_per_col_y = 78;

		static constexpr int fountain_chunks_per_frame = 2;
	};
}
