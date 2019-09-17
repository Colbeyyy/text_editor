#include "parsing.h"
#include "buffer.h"
#include <ch_stl\time.h>

namespace parsing {
static const u8 char_type[] = {
    // Column-reduction table to map 128 ASCII values to a 24-input space.
    IDENT, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, NEWLINE,
    WHITE, WHITE, NEWLINE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    OP, DOUBLEQUOTE, POUND, IDENT, OP, AND, SINGLEQUOTE, LPAREN, RPAREN, STAR,
    PLUS, COMMA, MINUS, DOT, SLASH, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT,
    DIGIT, DIGIT, DIGIT, DIGIT, COLON, SEMI, LT, EQU, GT, OP, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, LSQR, BS, RSQR, OP, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, LCURLY, OP, RCURLY, OP, WHITE,
};
static const u8 lex_table[] = {
    // State transition table for deterministic finite automaton (DFA) lexer.
    // Overtop this DFA runs a comment-scanner and preprocessor directive
    // scanner.

    // State: DFA_STRINGLIT
    DFA_STRINGLIT,    // WHITE
    DFA_STRINGLIT,    // NEWLINE
    DFA_STRINGLIT,    // IDENT
    DFA_STRINGLIT,    // POUND
    DFA_WHITE,        // DOUBLEQUOTE
    DFA_STRINGLIT,    // SINGLEQUOTE
    DFA_STRINGLIT,    // DIGIT
    DFA_STRINGLIT,    // SLASH
    DFA_STRINGLIT_BS, // BS
    DFA_STRINGLIT,    // OP
    // State: DFA_STRINGLIT_BS
    DFA_STRINGLIT, // WHITE
    DFA_STRINGLIT, // NEWLINE
    DFA_STRINGLIT, // IDENT
    DFA_STRINGLIT, // POUND
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_STRINGLIT, // SINGLEQUOTE
    DFA_STRINGLIT, // DIGIT
    DFA_STRINGLIT, // SLASH
    DFA_STRINGLIT, // BS
    DFA_STRINGLIT, // OP
    // State: DFA_CHARLIT
    DFA_CHARLIT,    // WHITE
    DFA_CHARLIT,    // NEWLINE
    DFA_CHARLIT,    // IDENT
    DFA_CHARLIT,    // POUND
    DFA_CHARLIT,    // DOUBLEQUOTE
    DFA_WHITE,      // SINGLEQUOTE
    DFA_CHARLIT,    // DIGIT
    DFA_CHARLIT,    // SLASH
    DFA_CHARLIT_BS, // BS
    DFA_CHARLIT,    // OP
    // State: DFA_CHARLIT_BS
    DFA_CHARLIT, // WHITE
    DFA_CHARLIT, // NEWLINE
    DFA_CHARLIT, // IDENT
    DFA_CHARLIT, // POUND
    DFA_CHARLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_CHARLIT, // DIGIT
    DFA_CHARLIT, // SLASH
    DFA_CHARLIT, // BS
    DFA_CHARLIT, // OP
    // State: DFA_PREPROC_SLASH
    DFA_PREPROC,      // WHITE
    DFA_WHITE,        // NEWLINE
    DFA_PREPROC,      // IDENT
    DFA_PREPROC,      // POUND
    DFA_PREPROC,      // DOUBLEQUOTE
    DFA_PREPROC,      // SINGLEQUOTE
    DFA_PREPROC,      // DIGIT
    DFA_LINE_COMMENT, // SLASH
    DFA_PREPROC_BS,   // BS
    DFA_PREPROC,      // OP
    // State: DFA_SLASH
    DFA_WHITE,        // WHITE
    DFA_WHITE,        // NEWLINE
    DFA_IDENT,        // IDENT
    DFA_PREPROC,      // POUND
    DFA_STRINGLIT,    // DOUBLEQUOTE
    DFA_CHARLIT,      // SINGLEQUOTE
    DFA_NUMLIT,       // DIGIT
    DFA_LINE_COMMENT, // SLASH
    DFA_OP,           // BS
    DFA_OP,           // OP
    // State: DFA_LINE_COMMENT
    DFA_LINE_COMMENT, // WHITE
    DFA_WHITE,        // NEWLINE
    DFA_LINE_COMMENT, // IDENT
    DFA_LINE_COMMENT, // POUND
    DFA_LINE_COMMENT, // DOUBLEQUOTE
    DFA_LINE_COMMENT, // SINGLEQUOTE
    DFA_LINE_COMMENT, // DIGIT
    DFA_LINE_COMMENT, // SLASH
    DFA_LINE_COMMENT, // BS
    DFA_LINE_COMMENT, // OP
    // State: DFA_WHITE
    DFA_WHITE,     // WHITE
    DFA_WHITE,     // NEWLINE
    DFA_IDENT,     // IDENT
    DFA_PREPROC,   // POUND
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT,   // SINGLEQUOTE
    DFA_NUMLIT,    // DIGIT
    DFA_SLASH,     // SLASH
    DFA_OP,        // BS
    DFA_OP,        // OP
    // State: DFA_IDENT
    DFA_WHITE,     // WHITE
    DFA_WHITE,     // NEWLINE
    DFA_IDENT,     // IDENT
    DFA_PREPROC,   // POUND
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT,   // SINGLEQUOTE
    DFA_IDENT,     // DIGIT
    DFA_SLASH,     // SLASH
    DFA_OP,        // BS
    DFA_OP,        // OP
    // State: DFA_OP
    DFA_WHITE,     // WHITE
    DFA_WHITE,     // NEWLINE
    DFA_IDENT,     // IDENT
    DFA_PREPROC,   // POUND
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT,   // SINGLEQUOTE
    DFA_NUMLIT,    // DIGIT
    DFA_SLASH,     // SLASH
    DFA_OP,        // BS
    DFA_OP,        // OP
    // State: DFA_NUMLIT
    DFA_WHITE,     // WHITE
    DFA_WHITE,     // NEWLINE
    DFA_NUMLIT,    // IDENT
    DFA_PREPROC,   // POUND
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT,   // SINGLEQUOTE
    DFA_NUMLIT,    // DIGIT
    DFA_SLASH,     // SLASH
    DFA_OP,        // BS
    DFA_OP,        // OP
    // State: DFA_PREPROC
    DFA_PREPROC,       // WHITE
    DFA_WHITE,         // NEWLINE
    DFA_PREPROC,       // IDENT
    DFA_PREPROC,       // POUND
    DFA_PREPROC,       // DOUBLEQUOTE
    DFA_PREPROC,       // SINGLEQUOTE
    DFA_PREPROC,       // DIGIT
    DFA_PREPROC_SLASH, // SLASH
    DFA_PREPROC_BS,    // BS
    DFA_PREPROC,       // OP
    // State: DFA_PREPROC_BS
    DFA_PREPROC,       // WHITE
    DFA_PREPROC,       // NEWLINE
    DFA_PREPROC,       // IDENT
    DFA_PREPROC,       // POUND
    DFA_PREPROC,       // DOUBLEQUOTE
    DFA_PREPROC,       // SINGLEQUOTE
    DFA_PREPROC,       // DIGIT
    DFA_PREPROC_SLASH, // SLASH
    DFA_PREPROC_BS,    // BS
    DFA_PREPROC,       // OP
};

struct Lexer_Data {
    int state = 0;
    Lex_Dfa dfa = {};
};

static const u32 *lex(Lexer_Data& l, const u32* p, const u32* const end,
                      Lexeme *&lexemes) {
#define get64(p) (*reinterpret_cast<const u64*>(p))
#define as64(low, high) ((low) | static_cast<u64>(high) << 32)
    
    while (p < end) {
        if (l.state) {
            if (get64(p) == as64('*', '/')) {
                l.state = 0;
                lexemes->p = p + 2;
                lexemes->dfa = l.dfa;
                lexemes++;
                p += 2;
                continue;
            }
        } else {
            if (get64(p) == as64('/', '*')) {
                if (l.dfa > DFA_LINE_COMMENT) {
                    l.state = 1;
                    lexemes->p = p;
                    lexemes->lex_state = 1;
                    lexemes++;
                    p += 2;
                    continue;
                }
            }
            int c = (int)*p;
            c &= (c - 128) >> 31;
            Char_Type ch = (Char_Type)char_type[c];
            Char_Type ch_to_lex = ch;
            if (ch_to_lex > OP) {
                ch_to_lex = OP;
            }
            Lex_Dfa new_dfa = (Lex_Dfa)lex_table[l.dfa * NUM_CHAR_TYPES_IN_LEXER + ch_to_lex];
            if (new_dfa != l.dfa) {
                lexemes->p = p;
                lexemes->ch = ch;
                lexemes->dfa = new_dfa;
                lexemes++;
                l.dfa = new_dfa;
            }
        }
        p++;
    }
    return p;
}

void parse_cpp(Buffer* buf) {
    if (!buf->syntax_dirty) return;
    buf->syntax_dirty = false;
    ch::Gap_Buffer<u32>& b = buf->gap_buffer;
    usize buffer_count = b.count();

    buf->lexemes.reserve((buffer_count + 1) - buf->lexemes.allocated);
    buf->lexemes.count = 1;
    ch::mem_zero(buf->lexemes.data, buf->lexemes.allocated * sizeof(Lexeme));
    {
        buf->parse_time = -ch::get_time_in_seconds();
        defer(buf->parse_time += ch::get_time_in_seconds());

        u32* gap_end = b.gap + b.gap_size;
        u32* buffer_end = b.data + b.allocated;
        if (gap_end < buffer_end) {
            if (buffer_count <= 1) {
                b.gap[0] = 0;
            } else {
                b.gap[0] = gap_end[0];
            }
        }
        Lexer_Data lexer = {};
        lexer.dfa = DFA_WHITE;
        Lexeme* lex_seeker = buf->lexemes.begin();
        const u32* const finished_on = lex(lexer, b.data, b.gap, lex_seeker);
        const usize skip_ahead = finished_on - b.gap;
        lex(lexer, gap_end + skip_ahead, buffer_end, lex_seeker);
        {
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->p = buffer_end;
            lex_seeker->ch = WHITE;
            lex_seeker->dfa = DFA_WHITE;
            lex_seeker->lex_state = IN_NO_COMMENT;
            lex_seeker++;
        }
        buf->lexemes.count = lex_seeker - buf->lexemes.begin();
    }
}
} // namespace parsing
