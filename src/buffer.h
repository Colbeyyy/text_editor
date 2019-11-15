#pragma once

#include <ch_stl/filesystem.h>
#include <ch_stl/gap_buffer.h>
#include <ch_stl/hash.h>
#include "draw.h"
#include "parsing.h"

using Buffer_ID = usize;

CH_FORCEINLINE u64 hash(Buffer_ID id) {
	return ch::fnv1_hash(&id, sizeof(Buffer_ID));
}

struct Buffer {
	Buffer_ID id;
	ch::Gap_Buffer<u8> gap_buffer;
	ch::Path full_path;
	ch::Array<usize> eol_table;

	// @TODO(CHall): Find a better way to do this???
	
	ch::Array<usize> line_column_table;

	bool disable_parse = false;
    bool syntax_dirty = true;
    ch::Array<parsing::Lexeme> lexemes;
    f64 lex_time = 0;
    f64 parse_time = 0;
    u64 lex_parse_count = 0;

	Buffer();
	Buffer(Buffer_ID _id);

	void add_char(u32 c, usize index);
	void remove_char(usize index);

	void print_to(const char* fmt, ...);

	void refresh_eol_table();
	void refresh_line_column_table();

	u64 get_index_from_line(u64 line) const;
	u64 get_line_from_index(u64 index) const;
};
