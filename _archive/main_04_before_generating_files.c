#include<stdio.h>
#include<string.h>
#include<stdlib.h>


#define assert(cond) {if (!(cond)){ printf("\n%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}
#define assert_d(cond, arg) {if (!(cond)){ printf("Assert Failed: %s, %d\n", #cond, arg); *(volatile int * )0 = 0;}}
#define assert_s(cond, arg) {if (!(cond)){ printf("Assert Failed: %s, %s\n", #cond, arg); *(volatile int * )0 = 0;}}
#define inc(var, inc, max) { assert_d(var < max, var); var += inc; }
#define set(arr, idx, val, max) { assert(idx < max); arr[idx] = val;}

#define string_cat(t,str, max){\
    int needed = strlen(str) + strlen(t);\
    assert_d(needed < max, needed);\
    strcat(t, str);\
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

#define TEMP_MAX 100000

#define CREATE_ENUM(name)   name,

#define TOKENS(t)       \
    t(eof)              \
    t(header)           \
    t(quote)            \
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

struct Global {
    Token * tokens;
    Token * prev_token;
    int token_idx;
    void * memory;
    int memory_allocated;
    int memory_idx;
    char * input;
    int input_idx;
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
    if(c == '\0') return 1;
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

void * allocate(int memory_needed) {
    int memory_free = (g.memory_allocated-g.memory_idx);
    assert_d(memory_needed < memory_free, g.memory_allocated + memory_needed);
    void * block = &g.memory[g.memory_idx];
    g.memory_idx += memory_needed;
    //printf("allocated   %20d    needed %20d\n", memory_free, memory_needed);
    return block;
}

void create_token(int type, char * cursor, int len) {
    //printf("creating token %d, %s %d\n", g.token_idx, token_type_arr[type], len);
    Token * token   = &g.tokens[g.token_idx];
    token->type     = type;
    assert(len < CAPTURE_STR_MAX);
    token->value    = allocate(len + 1);
    for(int i = 0; i < len; ++i) {
        token->value[i] = cursor[i];
    }
    inc(g.token_idx, 1, TOKEN_ARR_MAX);
}

void create_blank_token(int type) {
    printf("creating token %d, %s\n", g.token_idx, token_type_arr[type]);
    Token * token   = &g.tokens[g.token_idx];
    token->type     = type;
    token->value    = NULL;
    inc(g.token_idx, 1, TOKEN_ARR_MAX);
}

int token_text() {
    int token_type = text;
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

int token_basic(int token_type, const char * str) {
    char * cursor = &g.input[g.input_idx];
    if(!is_match(cursor, str)) return 0;
    create_token(token_type, cursor, strlen(str));
    inc(g.input_idx, strlen(str), INPUT_MAX);
    return 1;
}

int token_header() {
    int token_type = header;
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
    printf("token config match\n");
    ++i;
    for(;;++i) {
        if (is_eol(cursor[i])) break;
    }
    if (i == 1) return 0; // must match at least one char
    inc(g.input_idx, i, INPUT_MAX);
    create_token(config, &cursor[1], i-1);
    return 1;
}

int token_toggle(int type, int flag, const char * match, int matching_state) {
    char * cursor = &g.input[g.input_idx];
    if(!is_match(cursor, match)) return 0;
    if(flags[flag] != matching_state) return 0;
    flags[flag] = !matching_state;
    inc(g.input_idx, strlen(match), INPUT_MAX);
    create_blank_token(type);
    return 1;
}

int token_src() {
    int type = src;
    const char *front   = "](";
    const char *back    = ")";
    int front_len       = strlen(front);
    int back_len        = strlen(back);
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
    const char * back   = "```";
    int front_len       = strlen(front);
    int back_len        = strlen(back);
    char * cursor       = &g.input[g.input_idx];
    if(!is_match(cursor, front)) return 0;
    int i = front_len;
    for(;;++i) {
        if (cursor[i] == '\0') return 0;
        if (is_match(&cursor[i], back)) break;
    }
    create_token(code, &cursor[front_len], i-front_len);
    i += back_len;
    inc(g.input_idx, i, INPUT_MAX);
    return 1;
}

// add new rules here
int match_token_rule(tk) {
    switch(tk) {
        case link:     return token_toggle(tk, tag_text_opened,  "[",  0);
        case image:     return token_toggle(tk, tag_text_opened,  "![", 0);
        case src:       return token_src();
        case oitalic:   return token_toggle(tk, italic_opened,    "_",  0);
        case citalic:   return token_toggle(tk, italic_opened,    "_",  1);
        case obold:     return token_toggle(tk, bold_opened,      "**", 0);
        case cbold:     return token_toggle(tk, bold_opened,      "**", 1);
        case quote:     return token_basic(tk, "> ");
        case nl:        return token_basic(tk, "\n");
        case config:    return token_config();
        case header:    return token_header();
        case code:      return token_code();
        case eof:       return token_eof();
        case text:      return token_text();
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
    };
};

void parse_any();

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
    node->header.level = strlen(header_token->value);
    Token *text_token = consume(text);
    node->header.value = allocate(strlen(text_token->value)+1);
    strcpy(node->header.value, text_token->value);
}

void parse_text(struct Node * node) {
    assert(node);
    node->type = NODE_TEXT;
    Token *text_token = consume(text);
    node->text.value = allocate(strlen(text_token->value)+1);
    strcpy(node->text.value, text_token->value);
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

int is_deadend(int type) {
    switch(type) {
        case eof:       return 1;
        case src:       return 1;
        case citalic:   return 1;
        case cbold:     return 1;
        default:        return 0;
    }
}

//@TODO; implement config
void parse_config(struct Node * node) {
    node = NULL;
    consume(config);
}
void parse_quote(struct Node * node) {
    assert(node);
    consume(quote);
    node->type = NODE_QUOTE;
    node->quote.inside = allocate(sizeof(struct Node));
    parse_any(node->quote.inside);
}

void parse_rules(struct Node * node) {
    // if you look at the same token twice, throw an error
	assert_s(g.tokens != g.prev_token, token_type_arr[g.tokens->type]);
	g.prev_token = g.tokens;

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
}

void parser(struct Node *node) {
    assert(node);
    parse_any(node);
    consume(eof);
}

void print_node(char * t, struct Node *node, int indent) {
    assert(node);
    int inside_indent = indent + 4;
    string_cat(t, "\n", TEMP_MAX);
    for (int i = 0; i < indent;++i) {
        string_cat(t, " ", TEMP_MAX);
    }
    switch(node->type){
        case NODE_HEADER: {
            string_cat(t, "HEADER ", TEMP_MAX);
            //string_cat(t, node->header.level, TEMP_MAX);
            string_cat(t, " \"", TEMP_MAX);
            string_cat(t, node->header.value, TEMP_MAX);
            string_cat(t, "\"", TEMP_MAX);
            break;
        }
        case NODE_QUOTE: {
            string_cat(t, "QUOTE ", TEMP_MAX);
            print_node(t, node->quote.inside, inside_indent);
            break;
        }
        case NODE_BOLD: {
            string_cat(t, "BOLD", TEMP_MAX);
            print_node(t, node->bold.inside, inside_indent);
            break;
        }
        case NODE_ITALIC: {
            string_cat(t, "ITALIC", TEMP_MAX);
            print_node(t, node->italic.inside, inside_indent);
            break;
        }
        case NODE_TEXT: {
            string_cat(t, "TEXT \"", TEMP_MAX);
            string_cat(t, node->text.value, TEMP_MAX);
            string_cat(t, "\"", TEMP_MAX);
            break;
        }
        case NODE_LINK: {
            string_cat(t, "LINK src=\"", TEMP_MAX);
            string_cat(t, node->link.src, TEMP_MAX);
            string_cat(t, "\"", TEMP_MAX);
            print_node(t, node->link.text, inside_indent);
            break;
        }
        case NODE_IMAGE: {
            string_cat(t, "IMAGE src=\"", TEMP_MAX);
            string_cat(t, node->link.src, TEMP_MAX);
            string_cat(t, "\"", TEMP_MAX);
            print_node(t, node->link.text, inside_indent);
            break;
        }
        case NODE_CODE: {
            string_cat(t, "CODE", TEMP_MAX);
            break;
        }
        case NODE_NL: {
            string_cat(t, "NL", TEMP_MAX); 
            break;
        }
        case NODE_UNDEFINED: {
            string_cat(t, "UNDEFINED", TEMP_MAX); 
            break;
        }
        default: 
            printf("print_node node type not found %s\n", 
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

void generate_html(struct Node * node) {
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
            printf("\n");
            break;
        case NODE_TEXT:
            printf("%s", node->text.value);
            break;
        case NODE_HEADER:
            printf("<h%d>", node->header.level);
            printf("%s", node->header.value);
            printf("</h%d>", node->header.level);
            break;
        case NODE_BOLD:
            printf("<b>");
            generate_html(node->bold.inside);
            printf("</b>");
            break;
        case NODE_ITALIC:
            printf("<i>");
            generate_html(node->bold.inside);
            printf("</i>");
            break;
        case NODE_LINK:
            printf("<a src=\"%s\">", node->link.src);
            generate_html(node->link.text);
            printf("</a>");
            break;
        case NODE_IMAGE:
            printf("<img src=\"%s\" alt=\"", node->image.src);
            generate_html(node->image.text);
            printf("\">");
            break;
        case NODE_CODE:
            printf("<pre>");
            printf("%s", node->code.value);
            printf("</pre>");
            break;
        case NODE_QUOTE:
            printf("<div class='quote'>");
            generate_html(node->quote.inside);
            printf("</div>");
            break;
		default:
			printf("node not found: '%s'\n", node_type_arr[node->type]);
            assert(0);
    }
    if (node->next != NULL) {
        generate_html(node->next);
    }
}

int main() {
    g.input_idx         = 0;
    g.token_idx         = 0;
    g.memory_allocated  = 13400000;
    g.memory_idx        = 0;
    g.memory            = malloc(g.memory_allocated);
    g.tokens            = allocate(TOKEN_ARR_MAX * sizeof(Token));
    g.prev_token        = NULL;
    {
        FILE * f = fopen("markdown.txt", "r");
        assert(f);
        fseek(f, 0, SEEK_END);
        long int length = ftell(f);
        g.input = allocate(length+1);
        fseek(f, 0, SEEK_SET);
        fread(g.input, length, 1, f );
        fclose(f);
        g.input[length] = '\0';
    }
    printf("\n-------Code-------\n%s", g.input);
    // ********************** TOKENIZER ***************************
    for (int match_found = 0;match_found != -1;) { // raw input loop
        match_found = 0; 
        for (int rule_idx = 0; rule_idx < TokenTypeEnd; ++rule_idx) {
            // this does the most work
            match_found = match_token_rule(rule_idx);
            if (match_found != 0) break;
        }
    }
    // assert tags are all closed
    for(int i = 0; i < FlagsEnd; ++i) {
        assert_s(!flags[i], flag_arr[i]);
    }
    printf("\n------Tokens------\n");
    for(Token *token = g.tokens; token->type != eof; token = &token[1]) {
        if (token->type == nl) {
            printf("%s\n", token_type_arr[token->type]);
        }
        else if (token->value == NULL) {
            printf("%s ", token_type_arr[token->type]);
        }
        else {
            printf("%s(%s) ", token_type_arr[token->type], token->value);
        }
    }

    // ************************* PARSER ***************************
    struct Node node = {};
    parser(&node);
    printf("\n\n------Nodes-------");
    char temp[TEMP_MAX];
    temp[0] = '\0';
    print_node(temp, &node, 0);
    printf("%s\n", temp);

    // *********************** GENERATE HTML **********************
    printf("\n------Html--------\n");
    generate_html(&node);
    printf("\nmemory used %d", g.memory_idx);
    return 0;
}
