#include "parsing.h"
#include "buffer.h"
#include <ch_stl/time.h>

namespace parsing {
// This is a column-reduction table to map 128 ASCII values to a 11-input space.
static const u8 char_type[] = {
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, NEWLINE,
    WHITE, WHITE, NEWLINE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    OP, DOUBLEQUOTE, OP, IDENT, OP, OP, SINGLEQUOTE, OP, OP, STAR,
    OP, OP, OP, OP, SLASH, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT,
    DIGIT, DIGIT, DIGIT, DIGIT, OP, OP, OP, OP, OP, OP, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, OP, BS, OP, OP, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, OP, OP, OP, OP, WHITE,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
};
// This is a state transition table for the deterministic finite
// automaton (DFA) lexer. Overtop this DFA runs a block-comment scanner.
static const u8 lex_table[] = {
    // WHITE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_WHITE,         // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_NEWLINE,       // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE,         // DFA_SLASH
    DFA_WHITE,         // DFA_IDENT
    DFA_WHITE,         // DFA_OP
    DFA_WHITE,         // DFA_OP2
    DFA_WHITE,         // DFA_NUMLIT
    // NEWLINE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_NEWLINE,       // DFA_LINE_COMMENT
    DFA_NEWLINE,       // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_NEWLINE,       // DFA_NEWLINE
    DFA_NEWLINE,       // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_NEWLINE,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NEWLINE,       // DFA_SLASH
    DFA_NEWLINE,       // DFA_IDENT
    DFA_NEWLINE,       // DFA_OP
    DFA_NEWLINE,       // DFA_OP2
    DFA_NEWLINE,       // DFA_NUMLIT
    // IDENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_IDENT,         // DFA_WHITE
    DFA_IDENT,         // DFA_WHITE_BS
    DFA_IDENT,         // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_IDENT,         // DFA_SLASH
    DFA_IDENT,         // DFA_IDENT
    DFA_IDENT,         // DFA_OP
    DFA_IDENT,         // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DOUBLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_WHITE
    DFA_STRINGLIT,     // DFA_WHITE_BS
    DFA_STRINGLIT,     // DFA_NEWLINE
    DFA_WHITE,         // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_STRINGLIT,     // DFA_SLASH
    DFA_STRINGLIT,     // DFA_IDENT
    DFA_STRINGLIT,     // DFA_OP
    DFA_STRINGLIT,     // DFA_OP2
    DFA_STRINGLIT,     // DFA_NUMLIT
    // SINGLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_CHARLIT,       // DFA_WHITE
    DFA_CHARLIT,       // DFA_WHITE_BS
    DFA_CHARLIT,       // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_WHITE,         // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_CHARLIT,       // DFA_SLASH
    DFA_CHARLIT,       // DFA_IDENT
    DFA_CHARLIT,       // DFA_OP
    DFA_CHARLIT,       // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DIGIT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_NUMLIT,        // DFA_WHITE
    DFA_NUMLIT,        // DFA_WHITE_BS
    DFA_NUMLIT,        // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NUMLIT,        // DFA_SLASH
    DFA_IDENT,         // DFA_IDENT
    DFA_NUMLIT,        // DFA_OP
    DFA_NUMLIT,        // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // SLASH
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_WHITE,         // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_SLASH,         // DFA_WHITE
    DFA_SLASH,         // DFA_WHITE_BS
    DFA_SLASH,         // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_LINE_COMMENT,  // DFA_SLASH
    DFA_SLASH,         // DFA_IDENT
    DFA_SLASH,         // DFA_OP
    DFA_SLASH,         // DFA_OP2
    DFA_SLASH,         // DFA_NUMLIT
    // STAR
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,       // DFA_LINE_COMMENT
    DFA_OP,                 // DFA_WHITE
    DFA_OP,                 // DFA_WHITE_BS
    DFA_OP,                 // DFA_NEWLINE
    DFA_STRINGLIT,          // DFA_STRINGLIT
    DFA_STRINGLIT,          // DFA_STRINGLIT_BS
    DFA_CHARLIT,            // DFA_CHARLIT
    DFA_CHARLIT,            // DFA_CHARLIT_BS
    DFA_BLOCK_COMMENT,      // DFA_SLASH
    DFA_OP,                 // DFA_IDENT
    DFA_OP2,                // DFA_OP
    DFA_OP,                 // DFA_OP2
    DFA_OP,                 // DFA_NUMLIT
    // BS
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_WHITE_BS,      // DFA_WHITE
    DFA_WHITE_BS,      // DFA_WHITE_BS
    DFA_WHITE_BS,      // DFA_NEWLINE
    DFA_STRINGLIT_BS,  // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT_BS,    // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE_BS,      // DFA_SLASH
    DFA_WHITE_BS,      // DFA_IDENT
    DFA_WHITE_BS,      // DFA_OP
    DFA_WHITE_BS,      // DFA_OP2
    DFA_WHITE_BS,      // DFA_NUMLIT
    // OP
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_OP,            // DFA_WHITE
    DFA_OP,            // DFA_WHITE_BS
    DFA_OP,            // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_OP,            // DFA_SLASH
    DFA_OP,            // DFA_IDENT
    DFA_OP2,           // DFA_OP
    DFA_OP,            // DFA_OP2
    DFA_OP,            // DFA_NUMLIT
};

static bool table_initialized = false;
static u8 char_type_premultiplied[256];

static u16 lex_table4[DFA_NUM_STATES * NUM_CHAR_TYPES * NUM_CHAR_TYPES * NUM_CHAR_TYPES * NUM_CHAR_TYPES];
static u32 char_type4_0[256];
static u16 char_type4_1[256];
static u16 char_type4_2[256];

void init_tables() {
    for (int i = 0; i < 256; i += 1) {
        char_type_premultiplied[i] = char_type[i] * DFA_NUM_STATES;

        char_type4_0[i] = char_type[i] * DFA_NUM_STATES * NUM_CHAR_TYPES * NUM_CHAR_TYPES * NUM_CHAR_TYPES;
        char_type4_1[i] = char_type[i] * DFA_NUM_STATES * NUM_CHAR_TYPES * NUM_CHAR_TYPES;
        char_type4_2[i] = char_type[i] * DFA_NUM_STATES * NUM_CHAR_TYPES;
    }
    
    for (int i = 0; i < DFA_NUM_STATES; i += 1) {
        const u8 dfa = i;
        for (int j = 0; j < NUM_CHAR_TYPES; j += 1) {
            for (int k = 0; k < NUM_CHAR_TYPES; k += 1) {
                for (int l = 0; l < NUM_CHAR_TYPES; l += 1) {
                    for (int m = 0; m < NUM_CHAR_TYPES; m += 1) {
                        u8 dfa0 = lex_table[dfa + j * DFA_NUM_STATES];
                        u8 dfa1 = lex_table[dfa0 + k * DFA_NUM_STATES];
                        u8 dfa2 = lex_table[dfa1 + l * DFA_NUM_STATES];
                        u8 dfa3 = lex_table[dfa2 + m * DFA_NUM_STATES];
                        u16 result = dfa0 | (dfa1 << 4) | (dfa2 << 8) | (dfa3 << 12);
                        if (result == dfa * 0x1111) {
                            result = 0xffff;
                        }
                        if (dfa0 == dfa) {
                            result |= 0x000f;
                        }
                        if (dfa1 == dfa0) {
                            result |= 0x00f0;
                        }
                        if (dfa2 == dfa1) {
                            result |= 0x0f00;
                        }
                        if (dfa3 == dfa2) {
                            result |= 0xf000;
                        }
                        result = ~result;
                        lex_table4[i +
                                   j * DFA_NUM_STATES * NUM_CHAR_TYPES * NUM_CHAR_TYPES * NUM_CHAR_TYPES +
                                   k * DFA_NUM_STATES * NUM_CHAR_TYPES * NUM_CHAR_TYPES +
                                   l * DFA_NUM_STATES * NUM_CHAR_TYPES +
                                   m * DFA_NUM_STATES] = result;
                    }
                }
            }
        }
    }
}
u8 lex(u8 dfa, const u8* p, const u8* const end, Lexeme*& lexemes) {
#define BYTES 4
    while (p < end && ((u64)p % BYTES != 0)) {
        u8 new_dfa = lex_table[dfa + char_type_premultiplied[*p]];
        if (new_dfa != dfa) {
            lexemes->i = p;
            lexemes->dfa = (Lex_Dfa)new_dfa;
            lexemes->cached_first = *p;
            lexemes++;
            dfa = new_dfa;
        }
        p++;
    }
    assert((u64)p % BYTES == 0);
    const u8* const end_aligned = (const u8*)((u64)end / BYTES * BYTES);
    while (p < end_aligned) {
    #if BYTES == 1
        u8 new_dfa = lex_table[dfa + char_type_premultiplied[*p]];
        if (new_dfa != dfa) {
            lexemes->i = p;
            lexemes->dfa = new_dfa;
            lexemes->cached_first = *p;
            lexemes++;
            dfa = new_dfa;
        }
        p++;
    #elif BYTES == 4
        int ch0 = char_type4_0[p[0]];
        int ch1 = char_type4_1[p[1]];
        int ch2 = char_type4_2[p[2]];
        int ch3 = char_type_premultiplied[p[3]];
        u16 new_dfa = lex_table4[dfa + ch0 + ch1 + ch2 + ch3];
        if (new_dfa) {
            if (new_dfa & 15) {
                lexemes->i = p;
                lexemes->dfa = ~new_dfa & 15;
                lexemes->cached_first = *p;
                lexemes++;
                dfa = ~new_dfa & 15;
            }
            if (new_dfa >> 4 & 15) {
                lexemes->i = p + 1;
                lexemes->dfa = ~(new_dfa >> 4) & 15;
                lexemes->cached_first = p[1];
                lexemes++;
                dfa = ~(new_dfa >> 4) & 15;
            }
            if (new_dfa >> 8 & 15) {
                lexemes->i = p + 2;
                lexemes->dfa = ~(new_dfa >> 8) & 15;
                lexemes->cached_first = p[2];
                lexemes++;
                dfa = ~(new_dfa >> 8) & 15;
            }
            new_dfa = ~new_dfa;
            if (new_dfa >> 12 != 15) {
                lexemes->i = p + 3;
                lexemes->dfa = new_dfa >> 12;
                lexemes->cached_first = p[3];
                lexemes++;
                dfa = new_dfa >> 12;
            }
        }
        p += 4;
    #endif
    }
    while (p < end) {
        u8 new_dfa = lex_table[dfa + char_type_premultiplied[*p]];
        if (new_dfa != dfa) {
            lexemes->i = p;
            lexemes->dfa = (Lex_Dfa)new_dfa;
            lexemes->cached_first = *p;
            lexemes++;
            dfa = new_dfa;
        }
        p++;
    }
    return dfa;
}

const u8* temp_parser_gap;
u64 temp_parser_gap_size;

static const u8 lexeme_sentinel_buffer[16];
u64 toklen(Lexeme* l) {
    const u8* next_i = l[1].i;
    if (next_i == lexeme_sentinel_buffer) next_i = l[2].i;
    if (next_i == temp_parser_gap + temp_parser_gap_size) next_i = temp_parser_gap;
    return next_i - l->i;
}

//bool nested = false; // JUST for debugging

void parse(Lexeme* l, Lexeme* end);

static const u64 chunk_mask[] = {
    0x0000000000000000ull, 0x00000000000000ffull, 0x000000000000ffffull, 0x0000000000ffffffull,
    0x00000000ffffffffull, 0x000000ffffffffffull, 0x0000ffffffffffffull, 0x00ffffffffffffffull,
    0xffffffffffffffffull,
};

#define C_KEYWORDS(X) \
    X(do) \
    X(if) \
    X(for) \
    X(else) \
    X(const) \
    X(union) \
    X(while) \
    X(define) \
    X(extern) \
    X(return) \
    X(sizeof) \
    X(static) \
    X(struct) \
    X(switch) \
    X(include) \
    X(typedef) \

#define C_KW_DECL(name) C##name,
enum C_Keyword {
    NOT_KEYWORD,
    C_KEYWORDS(C_KW_DECL)
};

#define KW_CHUNK_(s, offset, I)                                                \
    ((I) + offset < (sizeof(s) - 1)                                            \
         ? (u64)(s)[(I) + offset] << (u64)((I)*8ull)                           \
         : 0ull)

#define KW_CHUNK__(s, offset)                                                  \
    (KW_CHUNK_(s, offset, 0) | KW_CHUNK_(s, offset, 1) |                       \
     KW_CHUNK_(s, offset, 2) | KW_CHUNK_(s, offset, 3) |                       \
     KW_CHUNK_(s, offset, 4) | KW_CHUNK_(s, offset, 5) |                       \
     KW_CHUNK_(s, offset, 6) | KW_CHUNK_(s, offset, 7))

#define KW_CHUNK0(s) KW_CHUNK__(s, 0)
#define KW_CHUNK1(s) KW_CHUNK__(s, 8)
#define KW_CASE_RETURN(x)                                                      \
    case KW_CHUNK0(#x):                                                        \
        return C##x;

#define KW_CHUNK(x) KW_CHUNK0(x)

// Loading byte-wise so that we don't encroach on the end of the buffer.
// This is a whole bunch of unnecessary typing that I shouldn't need to write,
// but I have no guarantees that it will actually coalesce the reads properly otherwise.
// In fact, even with all THIS you can't guarantee it, you need something even gnarlier
// to get the right reads. But I trust that release build will merge these. (Maybe I shouldn't.)
#define Load1(x)            (u64)x[0]
#define Load2(x) Load1(x) | (u64)x[1] << 8
#define Load3(x) Load2(x) | (u64)x[2] << 16
#define Load4(x) Load3(x) | (u64)x[3] << 24
#define Load5(x) Load4(x) | (u64)x[4] << 32ull
#define Load6(x) Load5(x) | (u64)x[5] << 40ull
#define Load7(x) Load6(x) | (u64)x[6] << 48ull
#define Load8(x) Load7(x) | (u64)x[7] << 56ull

// This is a nasty macro to be able to switch on tokens in a fast and intelligent way.
// Obviously C++ sucks, so there's no way to switch on strings smartly,
// so I have to construct this monstrosity of a macro to get the desired behaviour.
// Still, it's got a speed advantage, and speed is paramount.
#define switch_on_token(l_, fordef, for2, for3, for4, for5, for6, for7, for8)  \
    do {                                                                       \
        Lexeme *switch_on_token_lexeme = (l_);                                 \
        const u8* swchp = switch_on_token_lexeme->i;                           \
        switch (u64 len = toklen(switch_on_token_lexeme)) {                    \
        default: def: { fordef; } break;                                       \
        case 2: switch (Load2(swchp)) { default: goto def; { for2; } } break;  \
        case 3: switch (Load3(swchp)) { default: goto def; { for3; } } break;  \
        case 4: switch (Load4(swchp)) { default: goto def; { for4; } } break;  \
        case 5: switch (Load5(swchp)) { default: goto def; { for5; } } break;  \
        case 6: switch (Load6(swchp)) { default: goto def; { for6; } } break;  \
        case 7: switch (Load7(swchp)) { default: goto def; { for7; } } break;  \
        case 8: switch (Load8(swchp)) { default: goto def; { for8; } } break;  \
        }                                                                      \
    } while (0);

C_Keyword match_token(Lexeme *l) {
#if 1
    switch_on_token(l,
        {
            return NOT_KEYWORD;
        },
        {
            KW_CASE_RETURN(if);
            KW_CASE_RETURN(do);
        },
        {
            KW_CASE_RETURN(for);
        },
        {
            KW_CASE_RETURN(else);
        },
        {
            KW_CASE_RETURN(const);
            KW_CASE_RETURN(union);
            KW_CASE_RETURN(while);
        },
        {
            KW_CASE_RETURN(define);
            KW_CASE_RETURN(extern);
            KW_CASE_RETURN(return);
            KW_CASE_RETURN(sizeof);
            KW_CASE_RETURN(static);
            KW_CASE_RETURN(struct);
            KW_CASE_RETURN(switch);
        },
        {
            KW_CASE_RETURN(include);
            KW_CASE_RETURN(typedef);
        },
    )
#elif 0
    switch (u64 len = toklen(l)) {
    default:
        return NOT_KEYWORD;
    case 2:
        switch (Load2(l->i)) {
            KW_CASE_RETURN(if);
            KW_CASE_RETURN(do);
        }
        return NOT_KEYWORD;
    case 3:
        switch (Load3(l->i)) {
            KW_CASE_RETURN(for);
        }
        return NOT_KEYWORD;
    case 4:
        switch (Load4(l->i)) {
            KW_CASE_RETURN(else);
        }
        return NOT_KEYWORD;
    case 5:
        switch (Load5(l->i)) {
            KW_CASE_RETURN(const);
            KW_CASE_RETURN(union);
            KW_CASE_RETURN(while);
        }
        return NOT_KEYWORD;
    case 6:
        switch (Load6(l->i)) {
            KW_CASE_RETURN(define);
            KW_CASE_RETURN(extern);
            KW_CASE_RETURN(return );
            KW_CASE_RETURN(sizeof);
            KW_CASE_RETURN(static);
            KW_CASE_RETURN(struct);
            KW_CASE_RETURN(switch);
        }
        return NOT_KEYWORD;
    case 7:
        switch (Load7(l->i)) {
            KW_CASE_RETURN(include);
            KW_CASE_RETURN(typedef);
        }
        return NOT_KEYWORD;
    }
#else
    u64 len = toklen(l);
    if (len < 2 || len > 8) return NOT_KEYWORD;
    switch (*(u64*)l->i & chunk_mask[len]) {
        C_KEYWORDS(KW_CASE);
    }
#endif
    return NOT_KEYWORD;
}

static Lexeme* skip_comments_in_line(Lexeme* l, Lexeme* end) {
    while (l < end && (l->dfa < DFA_NEWLINE || l->dfa == DFA_SLASH && l[1].dfa <= DFA_LINE_COMMENT)) l++;
    return l;
}
static Lexeme* parse_preproc(Lexeme* l, Lexeme* end) {
    l->dfa = DFA_PREPROC;
    l++;
    l = skip_comments_in_line(l, end);
    if (l->dfa == DFA_IDENT) {
        Lexeme* directive = l;
        l->dfa = DFA_PREPROC;
        l++;
        l = skip_comments_in_line(l, end);
        switch_on_token(directive,,,,,,
        case KW_CHUNK("define"):
            if (l->dfa == DFA_IDENT) {
                l->dfa = DFA_MACRO;
                l++;
                if (l->c() == '(') {
                    while (l < end && l->dfa != DFA_NEWLINE) {
                        l++;
                        if (l->c() == ')') {
                            l++;
                            break;
                        }
                    }
                }
            }
            break,
        case KW_CHUNK("include"):
            if (l->c() == '<') {
                l->dfa = DFA_STRINGLIT;
                while (l < end && l->dfa != DFA_NEWLINE) {
                    l->dfa = DFA_STRINGLIT;
                    l++;
                    if (l->c() == '>') {
                        l->dfa = DFA_STRINGLIT;
                        l++;
                        break;
                    }
                }
            }
            break,);
    }
    Lexeme* preproc_begin = l;
    while (l < end && l->dfa != DFA_NEWLINE) l++;
    //nested = true;
    parse(preproc_begin, l);
    //nested = false;
    return l;
}
static Lexeme* next_token(Lexeme* l, Lexeme* end) {
    while (true) {
        l = skip_comments_in_line(l, end);
        if (l < end && l->dfa == DFA_NEWLINE) {
            l++;
            if (l->c() == '#') {
                //assert(!nested);
                l = parse_preproc(l, end);
            }
            continue;
        }
        break;
    }
    return l;
}

static bool at_token(Lexeme* l, Lexeme* end) {
    Lexeme* r = skip_comments_in_line(l, end);
    return r == l && (!(r < end) || r->c() != '#');
}

// Brace nesting precedence: {for(;[;{;];)}}
// 1. {} - Curlies: Most important
// 2. () - STATEMENT parentheses: for(;;) etc.
//    ;  - Semicolon supersedes everything below
// 3. () - EXPRESSION/PARAMETER parentheses: call(x, y); int func(int a); etc.
// 4. [] - Square brackets.
//    ,  - Comma supersedes almost nothing.
// 5. <> - Greater than/less than: least important.

static Lexeme* parse_stmt(Lexeme* l, Lexeme* end, Lex_Dfa var_name = DFA_IDENT);
static Lexeme* parse_braces(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '{');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_stmt(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')') {
            l++;
        }
        if (l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_stmt_parens(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_stmt(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_expr(Lexeme* l, Lexeme* end);
static Lexeme* parse_exprs_til_semi(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    while (l < end) {
        l = parse_expr(l, end);
        if (l->c() == ',' ||
            l->c() == ']') {
            l++;
        }
        if (l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_exprs_til_comma(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    while (l < end) {
        l = parse_expr(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_expr_parens(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_exprs_til_comma(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}
static Lexeme* parse_expr_sqr(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '[');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_exprs_til_comma(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_param(Lexeme* l, Lexeme* end);
static Lexeme* parse_params(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_param(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_expr(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    if (!(l < end)) return l;
    if (l->c() == '#') {
        if (l[1].i[0] == '#') {
            l->dfa = DFA_IDENT;
            l++;
            l->dfa = DFA_IDENT;
            l++;
            l = next_token(l, end);
            if (l->dfa == DFA_NUMLIT) {
                l->dfa = DFA_IDENT;
                l++;
            }
        } else {
            l->dfa = DFA_PREPROC;
            l++;
        }
        l = next_token(l, end);
    }
    /*switch_on_token(l,,,,,,
    case KW_CHUNK("sizeof"): {
            l++;
            l = next_token(l, end);
            l = parse_expr(l, end);
        }
        break,,);*/
    if (l->c() == '~' ||
        l->c() == '!' ||
        l->c() == '&' ||
        l->c() == '*' ||
        l->c() == '-' ||
        l->c() == '+') {
        l++;
        l = next_token(l, end);
        return parse_expr(l, end);
    }
    if (l->c() == '{') {
        l = parse_braces(l, end);
        if (l->c() == '}') {
            l++;
            l = next_token(l, end);
        }
    } else if (l->c() == '(') {
        l = parse_expr_parens(l, end);
        if (l->c() == ')') {
            l++;
            l = next_token(l, end);
        }
    } else if (l->c() == '[') {
        l = parse_expr_sqr(l, end);
        if (l->c() == ']') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == '(') {
            l = parse_params(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
            if (l->c() == '{') {
                l = parse_braces(l, end);
                if (l->c() == '}') {
                    l++;
                    l = next_token(l, end);
                }
            }
        }
    } else if (l->dfa == DFA_IDENT) {
        Lexeme* ident = l;
        l++;
        l = next_token(l, end);
        if (l->c() == '(') {
            ident->dfa = DFA_FUNCTION;
            l = parse_expr_parens(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
        } else if (l->c() == '{') {
            ident->dfa = DFA_TYPE;
            l = parse_braces(l, end);
            if (l->c() == '}') {
                l++;
                l = next_token(l, end);
            }
        }
    } else if (l->dfa == DFA_NUMLIT) {
        l++;
        l = next_token(l, end);
    } else if (l->dfa == DFA_STRINGLIT) {
        while (l < end && (l->dfa == DFA_STRINGLIT || l->dfa == DFA_STRINGLIT_BS)) {
            l++;
            l = next_token(l, end);
            // lets us properly parse this:
            // char*x = "string literal but there's "
            //          #pragma once
            //          "more more more";
        }
    } else if (l->dfa == DFA_CHARLIT) {
        while (l < end && (l->dfa == DFA_CHARLIT || l->dfa == DFA_CHARLIT_BS)) {
            l++;
            l = next_token(l, end);
            // lets us properly parse this:
            // char*x = "string literal but there's "
            //          #pragma once
            //          "more more more";
        }
    }
    if (l->c() == '[' ||
        l->c() == '(') {
        l = parse_expr(l, end);
        l = next_token(l, end);
    }
    if (l->c() == '%' ||
        l->c() == '^' ||
        l->c() == '&' ||
        l->c() == '*' ||
        l->c() == '-' ||
        l->c() == '=' ||
        l->c() == '+' ||
        l->c() == '|' ||
        l->c() == ':' ||
        l->c() == '<' ||
        l->c() == '.' ||
        l->c() == '>' ||
        l->c() == '/' ||
        l->c() == '?') {
        l++;
        l = next_token(l, end);
        l = parse_expr(l, end);
    }
    return l;
}

static Lexeme* parse_param(Lexeme* l, Lexeme* end) { return parse_stmt(l, end, DFA_PARAM); }

static Lexeme* parse_stmt(Lexeme* l, Lexeme* end, Lex_Dfa var_name_type) {
    if (l->dfa != DFA_IDENT) return parse_exprs_til_semi(l, end);
    switch (match_token(l)) {
    default: {
        l->dfa = DFA_TYPE;
        l++;
        l = next_token(l, end);
    } break;
    case Ctypedef: {
        l++;
        l = next_token(l, end);
        return parse_stmt(l, end, DFA_TYPE);
    } break;
    case Creturn: {
        l++;
        l = next_token(l, end);
        return parse_exprs_til_semi(l, end);
    } break;
    case Cdo: {
        l++;
        l = next_token(l, end);
        return parse_stmt(l, end);
    } break;
    case Cif:
    case Cswitch:
    case Cwhile:
    case Cfor: {
        l++;
        l = next_token(l, end);
        if (l->c() == '(') {
            l = parse_stmt_parens(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
        }
        return parse_stmt(l, end);
    } break;
    case Celse: {
        l++;
        l = next_token(l, end);
        return parse_stmt(l, end);
    } break;
    case Cstruct:
    case Cunion: {
        l++;
        l = next_token(l, end);
        if (l->dfa == DFA_IDENT) {
            l->dfa = DFA_TYPE;
            l++;
            l = next_token(l, end);
        }
        if (l->c() == '{') {
            l = parse_braces(l, end);
            if (l->c() == '}') {
                l++;
                l = next_token(l, end);
            }
        }
    } break;
    }
    bool is_likely_function = false;
more_decls:
    bool seen_closing_paren = false;
    int paren_nesting = 0;
    Lexeme* last = nullptr;
    Lexeme* func = nullptr;
    while (l < end &&
           (l->dfa == DFA_IDENT ||
            l->c() == '*' ||
            l->c() == '&' ||
            l->c() == '(' ||
            l->c() == ')' ||
            paren_nesting)) {
        if (l->c() == '(') {
            if (seen_closing_paren || func) {
                if (func) {
                    func->dfa = DFA_FUNCTION;
                }
                is_likely_function = true;
                l = parse_params(l, end);
                if (l->c() == ')') {
                    l++;
                    l = next_token(l, end);
                }
                continue;
            } else {
                paren_nesting++;
            }
            func = nullptr;
        } else if (l->c() == ')') {
            paren_nesting--;
            if (paren_nesting < 0) break;
            seen_closing_paren = true;
            func = nullptr;
        } else if (l->c() == '[') {
            l = parse_expr_sqr(l, end);
            if (l->c() == ']') {
                l++;
                l = next_token(l, end);
            }
            continue;
        } else if (l->c() == '*' ||
                   l->c() == '&') {
            l->dfa = DFA_TYPE;
        } else if (l->dfa == DFA_IDENT) {
            last = l;
            func = l;
            l->dfa = DFA_TYPE;
        } else if (l->c() == ':' && l[1].c() == ':') {
            // do nothing
        } else {
            while (l < end && paren_nesting && l->c() != ';') {
                l = parse_exprs_til_semi(l, end);
                if (l->c() == ')') {
                    paren_nesting--;
                    l++;
                    l = next_token(l, end);
                }
            }
            return l;
        }
        l++;
        l = next_token(l, end);
    }
    if (last && last->dfa != DFA_FUNCTION) last->dfa = var_name_type;
    if (l < end) {
        if (var_name_type != DFA_PARAM) {
            //assert(at_token(l, end));
            l = next_token(l, end);
            if (is_likely_function && l->c() == '{') {
                l = parse_braces(l, end);
                if (l->c() == '}') {
                    l++;
                    l = next_token(l, end);
                }
                return l;
            } else {
                l = parse_exprs_til_comma(l, end);
                if (l->c() == ',') {
                    l++;
                    l = next_token(l, end);
                    goto more_decls;
                }
            }
        } else {
            l = parse_exprs_til_comma(l, end);
        }
    }

    return l;
}

static void parse(Lexeme* l, Lexeme* end) {
    while (l < end) {
        //if (l->dfa == DFA_IDENT) {
        //    Lexeme* m = l;
        //    do {
        //        l++;
        //        l = skip_comments_in_line(l, end);
        //    } while (l->dfa == DFA_NEWLINE);
        //    if (l->c() == '(') m->dfa = DFA_FUNCTION;
        //} else l++;

        l = next_token(l, end);
        l = parse_stmt(l, end);
        assert(l->c() != ',');
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            l++;
        }
    }
}

void parse_cpp(Buffer* buf) {
    if (!buf->syntax_dirty || buf->disable_parse) return;
    buf->syntax_dirty = false;
    ch::Gap_Buffer<u8>& b = buf->gap_buffer;
    usize buffer_count = b.count();

    //{
    //    // buffer start should at least be 8 byte aligned
    //    assert((u64)b.data % 8 == 0);
    //    // align gap to 4 bytes to simplify multi-byte lexing
    //    usize index_of_gap = b.gap - b.data;
    //    index_of_gap &= ~7ull;
    //    b.move_gap_to_index(index_of_gap); // @Volatile: this ONLY works because index_of_gap <= gap.
    //}
    if (!table_initialized) {
        init_tables();
        table_initialized = true;
    }

    // Three extra lexemes:
    // One extra lexeme at the front.
    // One at the back to indicate buffer end to the parser, pointing to safe
    // scratch data.
    // One after that pointing to the the real position of the (unreadable) buffer
    // end, so that identifier lengths can be correctly computed.
    buf->lexemes.reserve((1 + buffer_count + 1 + 1) - buf->lexemes.allocated);
    buf->lexemes.count = 1;
    if (buffer_count) {
        u8 lexer = DFA_NEWLINE;
        Lexeme* lex_seeker = buf->lexemes.begin();
        {
            lex_seeker->i = b.data;
            lex_seeker->dfa = (Lex_Dfa)lexer;
            lex_seeker++;
        }
        
        f64 lex_time = -ch::get_time_in_seconds();
        lexer = lex(lexer, b.data, b.gap, lex_seeker);
        Lexeme* lexeme_at_gap = lex_seeker - 1;
        lexer = lex(lexer, b.gap + b.gap_size, b.data + b.allocated, lex_seeker);
        lex_time += ch::get_time_in_seconds();

        if (lexeme_at_gap > buf->lexemes.begin() && lexeme_at_gap < lex_seeker) {
            assert(lexeme_at_gap->i < b.gap);
            if (lexeme_at_gap + 1 < lex_seeker) {
                assert(lexeme_at_gap[1].i >= b.gap + b.gap_size);
            }
            b.move_gap_to_index(lexeme_at_gap->i - b.data);
            assert(lexeme_at_gap->i == b.gap);
            lexeme_at_gap->i += b.gap_size;
        }

        {
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->dfa = DFA_NUM_STATES;
            lex_seeker->i = lexeme_sentinel_buffer; // So the parser can safely read from here.
            lex_seeker++;
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->dfa = DFA_NUM_STATES;
            lex_seeker->i = b.data + b.allocated; // So the parser knows the real end position.
            lex_seeker++;
        }
        buf->lexemes.count = lex_seeker - buf->lexemes.begin();

        temp_parser_gap = b.gap;
        temp_parser_gap_size = b.gap_size;

        f64 parse_time = -ch::get_time_in_seconds();
        parse(buf->lexemes.begin(), buf->lexemes.end() - 2);
        parse_time += ch::get_time_in_seconds();
        buf->lexemes.end()[-2] = buf->lexemes.end()[-1];
        buf->lex_time += lex_time;
        //buf->lex_time /= 2;
        buf->parse_time += parse_time;
        //buf->parse_time /= 2;
        buf->lex_parse_count++;
    }
}
} // namespace parsing
