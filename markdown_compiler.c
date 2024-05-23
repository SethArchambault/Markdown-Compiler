// Adding a new token?
// For all the places you need to update
// search for :add 

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define assert(cond) {if (!(cond)){ printf("\n%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}
#define assert_d(cond, arg) {if (!(cond)){ printf("Assert Failed: %s, %d\n", #cond, arg); *(volatile int * )0 = 0;}}
#define assert_s(cond, arg) {if (!(cond)){ printf("Assert Failed: %s, %s\n", #cond, arg); *(volatile int * )0 = 0;}}

#define inc(var, inc, max) { assert_d(var < max, var); var += inc; }

#define t_sprintf(t,str){\
    int needed = (int)(strlen(str) + strlen(t));\
    assert_d(needed < TEMP_MAX, needed);\
    strcat(t, str);\
}

#define t_sprintf_2s(t,str1, str2){\
    int needed = (int)(strlen(str1) + strlen(str2));\
    assert_d(needed < TEMP_MAX, needed);\
    strcat(t, str1);\
    strcat(t, str2);\
}

#define t_sprintf_3s(t,str1, str2, str3){\
    int needed = (int)(strlen(str1) + strlen(str2) + strlen(str3) + strlen(t));\
    assert_d(needed < TEMP_MAX, needed);\
    strcat(t, str1);\
    strcat(t, str2);\
    strcat(t, str3);\
}

#define t_sprintf_4s(t,str1, str2, str3, str4){\
    int needed = (int)(strlen(str1) + strlen(str2) + strlen(str3) + \
        strlen(str4) + strlen(t));\
    assert_d(needed < TEMP_MAX, needed);\
    strcat(t, str1);\
    strcat(t, str2);\
    strcat(t, str3);\
    strcat(t, str4);\
}

// not used
#define t_sprintf_arr(t,count, vargs...){\
    const char ** args = {##vargs};\
    int needed = 0;\
    for (int i = 0; ++i; i < str_count){\
        needed += strlen(args[i]);\
        assert_d(needed < TEMP_MAX, needed);\
        strcat(t, args[i]);\
    }\
}\


void save(const char * buffer, const char * base_dir, const char * group, const char * article, const char * suffix) {
    char temp[1000]; 
    temp[0] = '\0';
    sprintf(temp, "%s%s/%s%s", base_dir, group, article, suffix);
    FILE * f = fopen (temp, "w");
    assert(f);
    fwrite(buffer, strlen(buffer), 1, f);
    fclose(f);
}
/*************************************************************
*********************** TOKENIZER **************************** 
*************************************************************/

#define CODE_FILE_MAX 600000
#define INPUT_MAX 600000
#define TOKEN_ARR_MAX 20000
#define TOKEN_RULES_MAX 6
#define TOKEN_STR_MAX 100
#define TOKEN_RULE_STR_MAX 20

#define CAPTURE_STR_MAX 20000

#define TEMP_MAX 1000000

#define CREATE_ENUM(name)   name,

// :add
#define TOKENS(t)       \
    t(eof)              \
    t(header)           \
    t(quote)            \
    t(bullet)           \
    t(hr)               \
    t(code)             \
    t(config)           \
    t(text)             \
    t(obold)            \
    t(cbold)            \
    t(oitalic)          \
    t(citalic)          \
    t(image)            \
    t(link)             \
    t(src)              \
    t(nl)               \

typedef enum {
    TOKENS(CREATE_ENUM)
    TokenTypeEnd
} TokenType;

#define CREATE_STRINGS(name) #name,
char token_type_arr [TokenTypeEnd][TOKEN_STR_MAX] = {
    TOKENS(CREATE_STRINGS)
};

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    TokenType type;
    char str[TOKEN_RULE_STR_MAX];
} TokenRule;

#define LINK_NAME_MAX       200
#define LINK_SRC_MAX        200
typedef struct {
    char name[LINK_NAME_MAX];
    char src[LINK_SRC_MAX];
} Link;

struct Global {
    Token * tokens;
    Token * prev_token;
    int token_idx;
    char * memory;
    unsigned long memory_allocated;
    unsigned long memory_idx;
    char * input;
    int input_idx;
    int input_len;
    Link header_links[100];
    int header_links_idx;
};

struct Global g = {0};

#define FLAGS(f)        \
    f(italic_opened)    \
    f(bold_opened)      \
    f(tag_text_opened)  \
    f(code_opened)

enum Flags {
    FLAGS(CREATE_ENUM)
    FlagsEnd
};

char flag_arr [FlagsEnd][20] = {
    FLAGS(CREATE_STRINGS)
};

int flags[FlagsEnd] = {0};

int is_eol(const char c) {
    if(c == '\n') return 1;
    if(c == '\0') return 1; // if this gets hit, it should be an eof token
    return 0;
}

int is_match(const char * c1, const char * c2) {
    for(int i = 0;c2[i] != '\0';++i) {
        if(c1[i] == '\0') return 0;
        if(c1[i] != c2[i]) return 0;
    }
    return 1;
}

int is_bold(const char * c) {
    if(! is_match(c, "**")) return 0;
    if (flags[bold_opened]) return 1;
    int i = 2; // skip matched **
    for (;;++i) {
        if (is_eol(c[i])) return 0;
        if (is_match(&c[i], "**")) break;
    }
    if (i <= 2) return 0;
    return 1;
}

int is_italic_char(const char * c) {
    int i = 0;
    if(c[i] != '_') return 0;
    if (flags[italic_opened]) return 1;
    ++i;
    // search eol for char
    for(;;++i) {
        if (is_eol(c[i])) return 0;
        if (c[i] == '_') break;
    }
    if (i <= 1) return 0;
    return 1; 
}

int is_inline_tag(const char * c) {
    // check if valid link exists
    int i = 0;
    if (c[i] == ']' && 
        flags[tag_text_opened]) {
            return 1;     
    }
    if (!is_match(&c[i], "![") && c[i] != '[') return 0;
    if (is_match(&c[i], "![")) ++i;
    ++i;
    // look for closing ](
    for(;;++i) {
        if (is_eol(c[i])) return 0;
        if (c[i] == '[') return 0;
        if (is_match(&c[i], "](")) break;
    }
    // look for closing )
    for(;;++i) {
        if (is_eol(c[i])) return 0;
        if (c[i] == '[') return 0;
        if (c[i] == ')') break;
    }
    return 1;
    // check for valid link
}

void * allocate(unsigned long memory_needed) {
    unsigned long memory_free = (g.memory_allocated-g.memory_idx);
    assert_d(memory_needed < memory_free, (int)(g.memory_allocated + memory_needed));
    void * block = &g.memory[g.memory_idx];
    g.memory_idx += memory_needed;
    //printf("allocated   %20d    needed %20d\n", memory_free, memory_needed);
    return block;
}

void create_token(TokenType type, char * cursor, int len) {
    //printf("creating token %d, %s %d\n", g.token_idx, token_type_arr[type], len);
    Token * token   = &g.tokens[g.token_idx];
    token->type     = type;
    assert(len < CAPTURE_STR_MAX);
    token->value    = allocate((unsigned long)len + 1);
    int i;
    for(i = 0; i < len; ++i) {
        token->value[i] = cursor[i];
    }
    token->value[i] = '\0';
    inc(g.token_idx, 1, TOKEN_ARR_MAX);
}


// Maybe add log files.
void create_blank_token(TokenType type) {
    //printf("creating token %d, %s\n", g.token_idx, token_type_arr[type]);
    Token * token   = &g.tokens[g.token_idx];
    token->type     = type;
    token->value    = NULL;
    inc(g.token_idx, 1, TOKEN_ARR_MAX);
}

int token_text() {
    TokenType token_type = text;
    char * cursor = &g.input[g.input_idx];
    // capture text until you find a link or the end of the line
    int i = 0;
    for(;;++i) {
        if (is_inline_tag(&cursor[i]))  break;
        if (is_bold(&cursor[i]))        break;
        if (is_italic_char(&cursor[i])) break;
        if (is_eol(cursor[i]))          break;
    }
    if (i == 0) return 0; // must match at least one char
    create_token(token_type, cursor, i);
    inc(g.input_idx, i, INPUT_MAX);
    return 1;
}

int token_basic(TokenType token_type, const char * str) {
    char * cursor = &g.input[g.input_idx];
    if(!is_match(cursor, str)) return 0;
    create_token(token_type, cursor, (int)strlen(str));
    inc(g.input_idx, strlen(str), INPUT_MAX);
    return 1;
}

int token_header() {
    TokenType token_type = header;
    char * cursor = &g.input[g.input_idx];
    int i = 0;
    for(;cursor[i] == '#';++i);
    if (i == 0) return 0;
    if (i > 6) return 0;
    if (cursor[i] != ' ') return 0;
    create_token(token_type, cursor, i);
    ++i;
    inc(g.input_idx, i, INPUT_MAX);
    return 1;
}

int token_eof() {
    char * cursor = &g.input[g.input_idx];
    int i = 0;
    if(cursor[i++] != '\n') return 0;
    if(cursor[i++] != '\0') return 0;
    inc(g.input_idx, i, CODE_FILE_MAX);
    create_blank_token(eof);
    return -1;
}

int token_config() {
    char * cursor = &g.input[g.input_idx];
    // capture text until you find a link or the end of the line
    int i = 0;
    if (cursor[i] != ':') return 0;
    //printf("token config match\n");
    ++i;
    for(;;++i) {
        if (is_eol(cursor[i])) break;
    }
    if (i == 1) return 0; // must match at least one char
    inc(g.input_idx, i, INPUT_MAX);
    create_token(config, &cursor[1], i-1);
    return 1;
}

int token_toggle(TokenType type, int flag, const char * match, int matching_state) {
    char * cursor = &g.input[g.input_idx];
    if(!is_match(cursor, match)) return 0;
    if(flags[flag] != matching_state) return 0;
    flags[flag] = !matching_state;
    inc(g.input_idx, strlen(match), INPUT_MAX);
    create_blank_token(type);
    return 1;
}

int token_src() {
    TokenType type = src;
    const char *front   = "](";
    const char *back    = ")";
    int front_len       = (int)strlen(front);
    int back_len        = (int)strlen(back);
    int flag            = tag_text_opened;
    char * cursor       = &g.input[g.input_idx];
    if(!is_match(cursor, front)) return 0;
    if(flags[flag] == 0) return 0;
    flags[flag] = 0;
    int i = front_len;
    for (;;++i) {
        if (is_eol(cursor[i])) return 0;
        if (is_match(&cursor[i], back)) break;
    }
    create_token(type, &cursor[front_len], i-front_len);
    i += back_len;
    inc(g.input_idx, i, INPUT_MAX);
    return 1;
}

int token_code() {
    const char * front  = "```";
    const char * back   = "```\n";
    int front_len       = (int)strlen(front);
    int back_len        = (int)strlen(back);
    char * cursor       = &g.input[g.input_idx];
    if(!is_match(cursor, front)) return 0;
    int i = front_len;
    for(;;++i) {
        if (cursor[i] == '\0') return 0;
        if (is_match(&cursor[i], back)) break;
    }
    create_token(code, &cursor[front_len], i-back_len);
    i += back_len;
    inc(g.input_idx, i, INPUT_MAX);
    return 1;
}


// :add new rules here
int match_token_rule(TokenType tk) {
    switch(tk) {
        case link:      return token_toggle(tk, tag_text_opened,  "[",  0);
        case image:     return token_toggle(tk, tag_text_opened,  "![", 0);
        case src:       return token_src();
        case oitalic:   return token_toggle(tk, italic_opened,    "_",  0);
        case citalic:   return token_toggle(tk, italic_opened,    "_",  1);
        case obold:     return token_toggle(tk, bold_opened,      "**", 0);
        case cbold:     return token_toggle(tk, bold_opened,      "**", 1);
        case quote:     return token_basic(tk, "> ");
        case bullet:    return token_basic(tk, "- ");
        case hr:        return token_basic(tk, "---");
        case nl:        return token_basic(tk, "\n");
        case config:    return token_config();
        case header:    return token_header();
        case code:      return token_code();
        case eof:       return token_eof();
        case text:      return token_text();
        case TokenTypeEnd:      break;
    }
    printf("match - unknown token '%s'\n", token_type_arr[tk]);

    // print error
    // find start of line
    int line_start = 0;
    for(int i = 0; i < (int)strlen(g.input); ++i) {
        printf("%c", g.input[i]);
        if (g.input[i] == '\n'){ 
           if(i > g.input_idx) break;
           line_start = i;
        }
    }
    // indent
    int col = g.input_idx - line_start;
    for(int i = 0; i < col ; ++i) {
        printf(" ");
    }
    printf("|<-- no match: '%c'\n", g.input[g.input_idx]);
    assert(0);
    return 0;
}

/*************************************************************
************************* PARSER ***************************** 
*************************************************************/

Token * consume(TokenType expected_type) {
    Token * token = g.tokens;
    if(token->type != expected_type) {
        printf("Expected: %s Actual: %s\n",
            token_type_arr[expected_type], token_type_arr[token[0].type] );
        assert(0);
    }
    //printf("consumed: %s\n", token_type_arr[expected_type]);
    // hmm...
    g.tokens = g.tokens + 1;
    //printf("token %d\n", token_count_debug);
    return token;
}

int peek_ahead(TokenType type, int offset) {
    if (g.tokens[offset].type == type) return 1;
    return 0;
}

int peek(TokenType type) {
    return peek_ahead(type, 0);
}

// :add
#define NODES(f)        \
    f(NODE_UNDEFINED)   \
    f(NODE_TEXT)        \
    f(NODE_BOLD)        \
    f(NODE_ITALIC)      \
    f(NODE_LINK)        \
    f(NODE_IMAGE)       \
    f(NODE_NL)          \
    f(NODE_CODE)        \
    f(NODE_HEADER)      \
    f(NODE_QUOTE)       \
    f(NODE_BULLET)      \
    f(NODE_HR)          \

typedef enum {
    NODES(CREATE_ENUM)
    NodeTypeEnd
} NodeType;

char node_type_arr[NodeTypeEnd][TOKEN_STR_MAX] = {
    NODES(CREATE_STRINGS)
};

#define NODE_TEXT_MAX 100

struct Node{
    NodeType type;
    struct Node * next;
    union {
        struct { // NODE_TEXT
            char *value;
            struct Node * inside;
        } text;
        struct { // NODE_BOLD
            struct Node * inside;
        } bold;
        struct { // NODE_LINK
            struct Node * text;
            char * src;
        } link;
        struct { // NODE_IMAGE
            struct Node * text;
            char * src;
        } image;
        struct { // NODE_CODE
            char * value;
        } code;
        struct { // NODE_ITALIC
            struct Node * inside;
        } italic;
        struct { // NODE_H1
            int level;
            char * value;
        } header;
        struct { // NODE_QUOTE
            struct Node * inside;
        } quote;
        struct { // NODE_BULLET
            struct Node * inside;
        } bullet;
    };
};

void parse_any(struct Node *node);

void parse_italic(struct Node * node) {
    assert(node);
    node->type = NODE_ITALIC;
    consume(oitalic);
    node->italic.inside = allocate(sizeof(struct Node));
    parse_any(node->italic.inside);
    consume(citalic);
}

void parse_link(struct Node * node) {
    assert(node);
    node->type = NODE_LINK;
    consume(link);
    node->link.text = allocate(sizeof(struct Node));
    parse_any(node->link.text);
    Token * src_token = consume(src);
    node->link.src = allocate(strlen(src_token->value)+1);
    strcpy(node->link.src, src_token->value);
}
void parse_image(struct Node * node) {
    assert(node);
    node->type = NODE_IMAGE;
    consume(image);
    node->image.text = allocate(sizeof(struct Node));
    // @TODO: If I didn't put any text here.. this can create a problem I guess!
    // I guess there are two kinds of images essentially - images with alt tags, and images without.
    parse_any(node->image.text);
    Token * src_token = consume(src);
    node->image.src = allocate(strlen(src_token->value)+1);
    strcpy(node->image.src, src_token->value);
}

void parse_bold(struct Node * node) {
    assert(node);
    node->type = NODE_BOLD;
    consume(obold);
    node->bold.inside = allocate(sizeof(struct Node));
    parse_any(node->bold.inside);
    consume(cbold);
}

void parse_header(struct Node * node) {
    assert(node);
    Token * header_token = consume(header);
    node->type = NODE_HEADER;
    node->header.level = (int)strlen(header_token->value);
    Token *text_token = consume(text);
    node->header.value = allocate(strlen(text_token->value)+1);
    strcpy(node->header.value, text_token->value);
    if (node->header.level == 1) {
        // @danger - I'm being lazy! This could cause segfault
        // but I guess my gcc flag will point that out..
        //printf("header value %s\n", node->header.value);
        sprintf(g.header_links[g.header_links_idx].name, "%s", node->header.value);
        sprintf(g.header_links[g.header_links_idx].src, "#%s", node->header.value);
        ++g.header_links_idx;
    }
    // conume a newline, to prevent artifical padding
    if (peek(nl)) {
        consume(nl);
    }
}

void parse_text(struct Node * node) {
    assert(node);
    node->type = NODE_TEXT;
    Token *text_token = consume(text);
    node->text.value = allocate(strlen(text_token->value)+2); //nl + \0
    strcpy(node->text.value, text_token->value);
    // seth - I think this is what is causing problems?
    if (peek(nl)) {
        strcat(node->text.value, "\n");
        consume(nl);
    }
}
void parse_nl(struct Node * node) {
    assert(node);
    node->type = NODE_NL;
    consume(nl);
}
void parse_code(struct Node * node) {
    assert(node);
    node->type = NODE_CODE;
    Token * code_token = consume(code);
    node->code.value = allocate(strlen(code_token->value)+1);
    strcpy(node->code.value, code_token->value);
}

int is_deadend(TokenType type) {
    switch(type) {
        case eof:       return 1;
        case src:       return 1;
        case citalic:   return 1;
        case cbold:     return 1;
        default:        return 0;
    }
}

//@TODO; implement config or just delete this
void parse_config(struct Node * node) {
    assert(node);
    node = NULL;
    consume(config);
}
// seth
// I think all of these have problems..
// look at parse_code for a possible fix..
void parse_quote(struct Node * node) {
    assert(node);
    consume(quote);
    node->type = NODE_QUOTE;
    node->quote.inside = allocate(sizeof(struct Node));
    // this creates an infinite problem.
    parse_any(node->quote.inside);
}
void parse_bullet(struct Node * node) {
    assert(node);
    consume(bullet);
    node->type = NODE_BULLET;
    node->bullet.inside = allocate(sizeof(struct Node));
    parse_any(node->bullet.inside);
}
void parse_hr(struct Node * node) {
    assert(node);
    consume(hr);
    node->type = NODE_HR;
}

void parse_rules(struct Node * node) {
    // if you look at the same token twice, throw an error
    assert_s(g.tokens != g.prev_token, token_type_arr[g.tokens->type]);
    g.prev_token = g.tokens;

    // :add
    switch(g.tokens->type) {
        case link:      return parse_link(node);   
        case image:     return parse_image(node);  
        case header:    return parse_header(node); 
        case code:      return parse_code(node);   
        case config:    return parse_config(node); 
        case src:       return;
        case cbold:     return;
        case citalic:   return;
        case quote:     return parse_quote(node);   
        case bullet:    return parse_bullet(node);   
        case hr:        return parse_hr(node);   
        case obold:     return parse_bold(node);   
        case oitalic:   return parse_italic(node); 
        case text:      return parse_text(node);   
        case nl:        return parse_nl(node);     
        case eof:   assert(0 && "parse any - END OF FILE");
        default:
            printf("parse any: token type not found: %s\n", token_type_arr[g.tokens->type]);
            assert(0);
    }
}

void parse_any(struct Node *node) {
    parse_rules(node);
    if (!is_deadend(g.tokens->type)) {
        node->next = allocate(sizeof (struct Node));
        parse_any(node->next);
    }
    else {
        node->next = 0;
    }
}

void print_node(char * t, struct Node *node, int indent) {
    assert(node);
    int inside_indent = indent + 4;
    t_sprintf(t, "\n");
    for (int i = 0; i < indent;++i) {
        t_sprintf(t, "x");
    }
    //assert(node->type);
    // :add
    switch(node->type){
        case NODE_HEADER: {
            t_sprintf(t, "HEADER ");
            //t_sprintf(t, node->header.level);
            t_sprintf(t, " \"");
            t_sprintf(t, node->header.value);
            t_sprintf(t, "\"");
            break;
        }
        case NODE_QUOTE: {
            t_sprintf(t, "QUOTE ");
            print_node(t, node->quote.inside, inside_indent);
            break;
        }
        case NODE_BULLET: {
            t_sprintf(t, "BULLET ");
            print_node(t, node->bullet.inside, inside_indent);
            break;
        }
        case NODE_HR: {
            t_sprintf(t, "HR ");
            break;
        }
        case NODE_BOLD: {
            t_sprintf(t, "BOLD");
            print_node(t, node->bold.inside, inside_indent);
            break;
        }
        case NODE_ITALIC: {
            t_sprintf(t, "ITALIC");
            print_node(t, node->italic.inside, inside_indent);
            break;
        }
        case NODE_TEXT: {
            t_sprintf(t, "TEXT \"");
            t_sprintf(t, node->text.value);
            t_sprintf(t, "\"");
            break;
        }
        case NODE_LINK: {
            t_sprintf(t, "LINK src=\"");
            t_sprintf(t, node->link.src);
            t_sprintf(t, "\"");
            print_node(t, node->link.text, inside_indent);
            break;
        }
        case NODE_IMAGE: {
            t_sprintf(t, "IMAGE src=\"");
            t_sprintf(t, node->link.src);
            t_sprintf(t, "\"");
            print_node(t, node->link.text, inside_indent);
            break;
        }
        case NODE_CODE: {
            t_sprintf(t, "CODE");
            break;
        }
        case NODE_NL: {
            t_sprintf(t, "NL"); 
            break;
        }
        case NODE_UNDEFINED: {
            t_sprintf(t, "UNDEFINED"); 
            break;
        }
        default: 
            printf("print_node - node type not found %s\n", 
                node_type_arr[node->type]);
            assert(0);
    }
    struct Node * next = node->next;
    if (next) {
        print_node(t, next, indent);
    }
}

/*************************************************************
********************* GENERATE HTML ************************** 
*************************************************************/

// :add
void generate_html(char * t, struct Node * node) {
    assert(node);
    switch(node->type){
        case NodeTypeEnd:
            assert(0 && "generate_html - NODE_TYPE_END");
            break;
        case NODE_UNDEFINED:
            printf("NODE_UNDEFINED");
            assert(0);
            break;
        case NODE_NL:
            t_sprintf(t, "<div class=\"newline\"></div>\n");
            break;
        case NODE_TEXT:
            t_sprintf(t, node->text.value);
            break;
        case NODE_HEADER:
            sprintf(t, "%s<h%d id='%s'><a href='#%s'>", t, node->header.level, node->header.value, node->header.value);
            t_sprintf(t, node->header.value);
            sprintf(t, "%s</a></h%d>", t, node->header.level);
            break;
        case NODE_BOLD:
            t_sprintf(t, "<b>");
            generate_html(t, node->bold.inside);
            t_sprintf(t, "</b>");
            break;
        case NODE_ITALIC:
            t_sprintf(t, "<i>");
            generate_html(t, node->bold.inside);
            t_sprintf(t, "</i>");
            break;
        case NODE_LINK:
            t_sprintf_3s(t, "<a href=\"", node->link.src, "\">");
            generate_html(t, node->link.text);
            t_sprintf(t, "</a>");
            break;
        case NODE_IMAGE:
            t_sprintf_3s(t, "<img src=\"/img/series/", node->image.src, "\" alt=\"");
            generate_html(t, node->image.text);
            t_sprintf(t, "\">");
            break;
        case NODE_CODE:{
            t_sprintf(t, "<pre id='pre'>");
            int value_i = 0;
            for (;node->code.value[value_i] != '\0';) {
                char value_char = node->code.value[value_i];
                if (value_char == '<') {
                    strcat(t, "&lt;");
                    value_i += 1;
                }
                else {
                    sprintf(&t[strlen(t)], "%c", value_char);
                    ++value_i;
                }
            }
            t_sprintf(t, "</pre>");
            break;
        }
        case NODE_QUOTE:
            t_sprintf(t, "<div class='quote'>");
            generate_html(t, node->quote.inside);
            t_sprintf(t, "</div>");
            break;
        case NODE_BULLET:
            t_sprintf(t, "<div>&bull; ");
            generate_html(t, node->quote.inside);
            t_sprintf(t, "</div>");
            break;
        case NODE_HR:
            t_sprintf(t, "<hr>");
            break;
        default:
            printf("node not found: '%s'\n", node_type_arr[node->type]);
            assert(0);
    }
    if (node->next != NULL) {
        generate_html(t, node->next);
    }
}

void markdown_compiler(void * memory, unsigned long memory_allocated, const char * arg_groupname, const char * arg_filename, const char * arg_articlename, const char * header, const char * footer, const char * title) {
    g.input_idx         = 0;
    g.token_idx         = 0;
    g.input_len         = 0;
    g.memory_allocated  = memory_allocated;
    g.memory_idx        = 0;
    g.memory            = memory;
    g.tokens            = allocate(TOKEN_ARR_MAX * sizeof(Token));
    g.prev_token        = NULL;
    g.header_links_idx  = 0;

    {
        char temp[TEMP_MAX] = {0};
        temp[0] = '\0';
        t_sprintf_2s(temp, "../html_generator/series/", arg_filename);
        FILE * f = fopen(temp, "r");
        if (!f) {
            printf("Error: file not found: %s\narg_filename: %s\n", temp, arg_filename);
        }
        assert(f);
        temp[0] = '\0';
        fseek(f, 0, SEEK_END);
        g.input_len = (int)ftell(f);
        g.input = allocate((unsigned long)g.input_len+1);
        fseek(f, 0, SEEK_SET);
        fread(g.input, (unsigned long)g.input_len, 1, f );
        fclose(f);
        g.input[g.input_len] = '\0';
    }
    
    // TOKENIZER ***************************************************
    //printf("tokenizer %s\n", arg_articlename);
    
    for (int match_found = 0;match_found != -1;) { // raw input loop
        match_found = 0; 
        for (int rule_idx = 0; rule_idx < TokenTypeEnd; ++rule_idx) {
            // this does the most work
            // @TODO: danger! Infinite loop if no space at end of file..
            match_found = match_token_rule((TokenType)rule_idx);
            if (match_found != 0) break;
        }
        if (g.input_idx >= g.input_len)  {
            // @hack - needed in case there is no newline at the end of a file
            create_blank_token(eof);
            break;
        }
    }
    // assert tags are all closed
    for(int i = 0; i < FlagsEnd; ++i) {
        assert_s(!flags[i], flag_arr[i]);
    }

    // TOKENS *******************************************************
    //printf("tokens\n");
    
    {
        char temp[TEMP_MAX] = {0};
        temp[0] = '\0';
        for(Token *token = g.tokens; token->type != eof; token = &token[1]) {
            if (token->type == nl) {
                t_sprintf_2s(temp, token_type_arr[token->type], "\n");
            }
            else if (token->value == NULL) {
                t_sprintf_2s(temp, token_type_arr[token->type], "() ");
            }
            else {
                t_sprintf_4s(temp, 
                        token_type_arr[token->type],
                        "(", token->value, ") ");
            }
        }
        save(temp, "debug/", arg_groupname, arg_articlename, "_tokens.txt");
    }

    // PARSER *******************************************************
    
    struct Node node = {};
    parse_any(&node);
    consume(eof);
    
    // NODES *******************************************************
    //printf("nodes\n");
    {
        char temp[TEMP_MAX];
        temp[0] = '\0';
        print_node(temp, &node, 0);
        save(temp, "debug/", arg_groupname, arg_articlename, "_nodes.txt");
    }

    // GENERATE HTML *********************************************
    //printf("generate html\n");
    
    char temp[TEMP_MAX];
    temp[0] = '\0';
    
    {
        char header_links_html[10000] = {0};
        for (int i = 0; i < g.header_links_idx; ++i) { 
            Link * link = &g.header_links[i];
            sprintf(header_links_html, "%s<a href='%s'>%s</a>",
                    header_links_html, link->src, link->name);
        }
        sprintf(temp, header, title, "style here", header_links_html);
    }

    generate_html(temp, &node);
    char script[] = "";
    // @dangerous: this seems subject to glitches    
    assert((strlen(temp) + strlen(footer) + strlen(script)) < TEMP_MAX);
    sprintf(temp, footer, temp, script);

    // HTML *******************************************************
    
    //printf("html\n");
    save(temp, "../public/series/", arg_groupname, arg_articlename, ".html");
}
