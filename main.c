#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int debug_log_start = 0;
int debug_log_end   = 0;
#define debug_log_max 200
char debug_log[debug_log_max];
/*
void debug(const char * arg){
    for(int i = 0;arg[i] != '\0';++i) {
        debug_log[debug_log_end] = arg[i];
        debug_log_end++;
        if (debug_log_end > debug_log_max) {
            debug_log_end = 0;
        }
        if (debug_log_start == debug_log_end) {
            debug_log_start++;
        }
        if (debug_log_start > debug_log_max) {
            debug_log_start = 0;
        }
    }
}
*/
#define debug(arr) {}


void print_debug_log(){
    int i = debug_log_start;
    for(;;) {
        if (debug_log[i] == '\0') break;
        if (i == debug_log_end) break;

        printf("%c", debug_log[i]);
        i++;
        if (i == debug_log_max) i = 0;
    }
}

#define assert(cond) {if (!(cond)){ print_debug_log(); printf("%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}
#define assert_d(cond, arg) {if (!(cond)){ print_debug_log(); printf("Assert Failed: %s, %d\n", #cond, arg); *(volatile int * )0 = 0;}}
#define inc(var, inc, max) { assert_d(var < max, var); var += inc; }
#define set(arr, idx, val, max) { assert(idx < max); arr[idx] = val;}

void string_cat(char * t, char * str, int max){
    int needed = strlen(str) + strlen(t);
    assert_d(needed < max, needed);
    strcat(t, str);
}
/*************************************************************
*********************** TOKENIZER **************************** 
*************************************************************/

#define CODE_FILE_MAX 500
#define TOKEN_ARR_MAX 100
#define TOKEN_RULES_MAX 6

#define REGEX_ARR_COUNT 20

#define REGEX_STR_MAX 50
#define TOKEN_STR_MAX 100
#define TOKEN_RULE_STR_MAX 20

#define CAPTURE_STR_MAX 200

#define TEMP_MAX 200

#define CREATE_ENUM(name)   name,

#define TOKENS(t)       \
    t(eof)              \
    t(h6)               \
    t(h5)               \
    t(h4)               \
    t(h3)               \
    t(h2)               \
    t(h1)               \
    t(quote)            \
    t(code)             \
    t(bold)             \
    t(italic)           \
    t(config)           \
    t(text)             \
    t(obracket)         \
    t(cbracket)         \
    t(oparen)           \
    t(cparen)           \
    t(exclamation)      \
    t(nl)               \

typedef enum {
    TOKENS(CREATE_ENUM)
    TokenTypeEnd
} TokenType;

#define REGEX_ARR_MAX TokenTypeEnd 

char regex_arr [REGEX_ARR_MAX][REGEX_STR_MAX] = {
    "[:eof:]",
    "###### ",  
    "##### ",  
    "#### ",  
    "### ",  
    "## ",  
    "# ",  
    "> ",  
    "```",                 
    "**",                 
    "_",                 
    "[:config:]",
    "[:text:]",
    "[",                 
    "]",                 
    "(",                 
    ")",                 
    "!",
    "\n",
};

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

int is_eol(const char c) {
    if(c == '\n') return 1;
    if(c == '\0') return 1;
    return 0;
}

int contains(const char c, const char * matches) {
    for (int i = 0; matches[i] != '\0';++i) {
        if (c == matches[i]) return 1;
    }
    return 0;
}

int is_inline_tag(const char c) {
    if (contains(c, "!()[]"))  return 1;
    return 0;
}

// @TODO - just make this return the number of matched characters
int is_match(const char * c1, const char * c2) {
    for(int i = 0;c2[i] != '\0';++i) {
        if(c1[i] == '\0') return 0;
        if(c1[i] != c2[i]) return 0;
    }
    return 1;
}

int is_bold(const char * c) {
    if(is_match(c, "**")) return 1;
    return 0;
}

int is_italic_char(const char c) {
    if(c == '_') return 1;
    return 0;
}

int capture_match(const char * code, const char * regex, char * capture_string) {
    int regex_idx   = 0;
    int code_idx    = 0;
    capture_string[0] = '\0';
    for (;;) {
        //printf("\n'%c' == '%c' rule: %s     ", 
         //       code[code_idx],regex[regex_idx], regex);
        if (regex[regex_idx] == '\0') {
            // remember, this is the end of the regex string
            // not the end of the file
            break;
        }
        else if (is_match(&regex[regex_idx], "[:config:]")) {
            debug("config match\n");
            // capture text until you find a link or the end of the line
            int i = 0;
            if (code[code_idx + i] != ':') return 0;
            ++i;
            for(;;++i) {
                if (is_eol(code[code_idx + i])) break;
            }
            if (i == 1) return 0; // must match at least one char
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:text:]")) {
            debug("text match\n");
            // capture text until you find a link or the end of the line
            int i = 0;
            for(;;++i) {
                if (is_inline_tag(code[code_idx + i])) break;
                if (is_bold(&code[code_idx + i])) break;
                if (is_italic_char(code[code_idx + i])) break;
                if (is_eol(code[code_idx + i])) break;
            }
            if (i == 0) return 0; // must match at least one char
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:eof:]")) { 
            int i = 0;
            if(code[code_idx + i] != '\n') return 0;
            ++i;
            if(code[code_idx + i] != '\0') return 0;
            ++i;
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (code[code_idx] == regex[regex_idx]) {
            inc(regex_idx, 1, REGEX_STR_MAX);
            inc(code_idx, 1, CODE_FILE_MAX); 
        }
        else {
            // no match
            return 0;
        }
    }
    int i = 0;
    for(; i < code_idx;) {
        capture_string[i] = code[i];
        inc(i,1, CAPTURE_STR_MAX);
    }
    capture_string[i] = '\0';
    return 1;
}

// Global Variables
Token * tokens;
void * memory;
int memory_allocated = 3400;
int memory_idx = 0;

void * allocate(int memory_needed) {
    int memory_free = (memory_allocated-memory_idx);
    assert_d(memory_needed < memory_free, memory_allocated + memory_needed);
    void * block = &memory[memory_idx];
    memory_idx += memory_needed;
    return block;
}
void tokenizer(const char * code) {
    int token_idx = 0;
    int code_idx = 0;
    for (;;) {
        int match_found = 0;
        char capture_string[CAPTURE_STR_MAX] = {0};
        for (int regex_idx= 0; regex_idx < REGEX_ARR_MAX; ++regex_idx) {
            match_found = capture_match(
                    &code[code_idx], regex_arr[regex_idx], capture_string);
            if (match_found) {
                Token * token = &tokens[token_idx];
                token->type  = regex_idx;
                assert(strlen(capture_string) < CAPTURE_STR_MAX);
                token->value = allocate(strlen(capture_string)+1);
                strcpy(token->value, capture_string);
                inc(token_idx, 1, TOKEN_ARR_MAX);
                inc(code_idx, strlen(capture_string), CODE_FILE_MAX);
                if (token->type == eof) return;
                break;
            }
        }// tokens
        if (match_found == 0) {
            // print error
            int line_start = 0;
            printf("\n");
            for(int i = 0; i < (int)strlen(code); ++i) {
                printf("%c", code[i]);
                if (code[i] == '\n'){ 
                   if(i > code_idx) break;
                   line_start = i;
                }
            }
            int col = code_idx - line_start;
            for(int i = 0; i < col ; ++i) {
                printf(" ");
            }
            printf("|<-- no match: '%c'\n", code[code_idx]);
            assert(0);
        } // error
    }// code
}

#if 1

/*************************************************************
************************* PARSER ***************************** 
*************************************************************/

Token * consume(TokenType expected_type) {
    Token * token = tokens;
    if(token->type != expected_type) {
        printf("Expected: %s Actual: %s\n",
            token_type_arr[expected_type], token_type_arr[token[0].type] );
        assert(0);
    }
    //printf("consumed: %s\n", token_type_arr[expected_type]);
    tokens = tokens + 1;
    return token;
}

int peek_ahead(TokenType type, int offset) {
    if (tokens[offset].type == type) return 1;
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
    union {
        struct { // NODE_TEXT
            char *value;
            struct Node * inside;
            struct Node * next;
        } text;
        struct { // NODE_BOLD
            struct Node * inside;
        } bold;
        struct { // NODE_LINK
            struct Node * text;
            char * href;
        } link;
        struct { // NODE_ITALIC
            struct Node * inside;
        } italic;
        struct {
            char value[TOKEN_STR_MAX];
        }id;
        struct {
            char name[TOKEN_STR_MAX];
        }var_ref;
        struct {
            int value;
        }integer;
        struct { // NODE_CALL
            char name[TOKEN_STR_MAX];
            struct Node ** arg_exprs;
        } call;
    };
};

void * print_node(char * t, struct Node *node) {
    assert(node);
    switch(node->type){
        case NODE_BOLD:
            string_cat(t, "<NODE_BOLD inside=", TEMP_MAX);
            print_node(t, node->bold.inside);
            string_cat(t, " >", TEMP_MAX);
            break;
        case NODE_ITALIC:
            string_cat(t, "<NODE_ITALIC inside=", TEMP_MAX);
            print_node(t, node->italic.inside);
            string_cat(t, " >", TEMP_MAX);
            break;
        case NODE_TEXT:
            string_cat(t, "<NODE_TEXT value=\"", TEMP_MAX);
            string_cat(t, node->text.value, TEMP_MAX);
            string_cat(t, "\">", TEMP_MAX);
            break;
        case NODE_LINK:
            string_cat(t, "<NODE_LINK text=", TEMP_MAX);
            print_node(t, node->link.text);
            string_cat(t, " href=\"", TEMP_MAX);
            string_cat(t, node->link.href, TEMP_MAX);
            string_cat(t, "\">", TEMP_MAX);
            break;
        /*
        case NODE_DEF:
            printf("<NODE_DEF name=\"%s\" ", node->def.name);
            printf("args=[");
            for (int i = 0;node->def.args[i] != NULL; ++i) {
                if (i != 0) printf(", ");
                print_node(node->def.args[i]);
            }
            printf("] ");
            printf("body=");
            print_node(node->def.body);
            printf(" >");
            break;
        case NODE_VAR_REF:
            printf("<NODE_VAR_REF value=\"%s\">", node->var_ref.name);
            break;
        case NODE_ID:
            printf("<NODE_ID value=\"%s\">", node->id.value);
            break;
        case NODE_INT:
            printf("<NODE_INT value=%d>", node->integer.value);
            break;
        case NODE_CALL:
            printf("<NODE_CALL name=\"%s\" ", node->call.name);
            printf("args=[");
            for (int i = 0;node->call.arg_exprs[i] != NULL; ++i) {
                if (i != 0) printf(", ");
                print_node(node->call.arg_exprs[i]);
            }
            printf("]>");
            break;
            */
        default: 
        printf("print_node node type not found %s", 
                node_type_arr[node->type]);
    }
    return t;
}


void parse_any();

void parse_italic(struct Node * node) {
    assert(node);
    node->type = NODE_ITALIC;
    consume(italic);
    node->italic.inside = allocate(sizeof(struct Node));
    parse_any(node->italic.inside);
    consume(italic);
}

void parse_link(struct Node * node) {
    assert(node);
    node->type = NODE_LINK;
    consume(obracket);
    node->link.text = allocate(sizeof(struct Node));
    parse_any(node->link.text);
    consume(cbracket);
    consume(oparen);
    Token *href_token = consume(text);
    node->link.href = allocate(strlen(href_token->value)+1);
    strcpy(node->link.href, href_token->value);
    consume(cparen);
}
void parse_bold(struct Node * node) {
    assert(node);
    node->type = NODE_BOLD;
    consume(bold);
    node->bold.inside = allocate(sizeof(struct Node));
    parse_any(node->bold.inside);
    consume(bold);
}

void parse_text(struct Node * node) {
    assert(node);
    node->type = NODE_TEXT;
    Token *text_token = consume(text);
    node->text.value = allocate(strlen(text_token->value)+1);
    strcpy(node->text.value, text_token->value);
    if (peek(nl)) {
        consume(nl);
        node->text.next = allocate(sizeof (struct Node));
        parse_any(node->text.next);
    }
}
void parse_any(struct Node *node) {
    switch(tokens->type) {
        case obracket:
            parse_link(node);
            break;
        case bold:
            parse_bold(node);
            break;
        case italic:
            parse_italic(node);
            break;
        case text:
            parse_text(node);
            break;
        case nl:
            consume(nl);
            break;
        default:
            printf("parse any: token type not found: %s\n", token_type_arr[tokens->type]);
            assert(0);
    }
}


void parser(struct Node *node) {
    assert(node);
    for(;!peek(eof);) {
        parse_any(node);
    }
    consume(eof);
}
#endif
#if 0
int args_til(TokenType type) {
    int arg_count = 0;
    for(int i = 0;!peek_ahead(rparen, i);++i) {
        if (peek_ahead(lparen, i)) {
            continue;
        }
        if (peek_ahead(comma, i)) {
            continue;
        }
        ++arg_count;
    }
    return arg_count;
}

void parse_arg_names(struct Node ** node) {
    assert(node);
    consume(lparen);
    // allocate space for arg_names
    {
        int i = 0;
        for (; i < args_til(rparen); ++i) {
            node[i] = allocate(sizeof(struct Node));
        }
        node[i] = NULL;
    }
    int node_idx = 0;
    if(peek(id)) {
        parse_id(node[node_idx]);
        ++node_idx;
        for (;peek(comma);++node_idx) {
            consume(comma);
            parse_id(node[node_idx]);
        }
    }
    consume(rparen);
}


void parse_call();

void parse_expr(struct Node * node) {
    assert(node);
    // allocate space for arg_names
    if (peek(integer)) {
        parse_integer(node);
    }
    else if (peek(id) && peek_ahead(lparen, 1)) {
        parse_call(node);
    }
    else if (peek(id)) {
        parse_var_ref(node);
    }
}


void parse_call(struct Node * node) {
    assert(node);
    node->type = NODE_CALL;
    strcpy(node->call.name, consume(id)->val);
    node->call.arg_exprs = allocate(args_til(rparen)*(sizeof(void *)+1));
    int i = 0;
    for (; i < args_til(rparen); ++i) {
        node->call.arg_exprs[i] = allocate(sizeof(struct Node));
    }
    node->call.arg_exprs[i] = NULL;


    consume(lparen);
    if (!peek(rparen)){
        parse_expr(node->call.arg_exprs[0]);
        for(int i= 1;peek(comma); ++i) {
            consume(comma);
            parse_expr(node->call.arg_exprs[i]);
        }
    }
    consume(rparen);
}

void parse_def(struct Node * node) {
    assert(node);
    node->type = NODE_DEF;
    consume(def);
    strcpy(node->def.name, consume(id)->val);
    node->def.args = allocate(sizeof(void *) * (args_til(rparen)+1));
    parse_arg_names(node->def.args);
    node->def.body = allocate(sizeof(void *) * 10);
    parse_expr(node->def.body);
    consume(end);
}

void generate_js(struct Node *node) {
    assert(node);
    switch(node->type){
        case NODE_DEF:
            printf("function %s(", node->def.name);
            for (int i = 0;node->def.args[i] != NULL; ++i) {
                if (i != 0) printf(", ");
                generate_js(node->def.args[i]);
            }
            printf(") {\n return ");
            generate_js(node->def.body);
            printf("\n}");
            break;
        case NODE_VAR_REF:
            printf("%s", node->var_ref.name);
            break;
        case NODE_ID:
            printf("%s", node->id.value);
            break;
        case NODE_INT:
            printf("%d", node->integer.value);
            break;
        case NODE_CALL:
            printf("%s(", node->call.name);
            for (int i = 0;node->call.arg_exprs[i] != NULL; ++i) {
                if (i != 0) printf(", ");
                generate_js(node->call.arg_exprs[i]);
            }
            printf(");");
            break;
        default: 
        printf("print_node node type not found %s", 
                node_type_arr[node->type]);
    }
}
#endif

int main() {
    memory = malloc(memory_allocated);
    tokens = allocate(TOKEN_ARR_MAX * sizeof(Token));


    char * code;
    {
        FILE * f = fopen("markdown.txt", "r");
        assert(f);
        fseek(f, 0, SEEK_END);
        long int length = ftell(f);
        code = allocate(length+1);
        fseek(f, 0, SEEK_SET);
        fread(code, length, 1, f );
        fclose(f);
        code[length] = '\0';
    }

#if 1
    printf("\nCode:\n%s\n", code);
#endif
    tokenizer(code);
    printf("\nTokens:\n");
    for(Token *token = tokens; token->type != eof; token = &token[1]) {
        if (token->type == nl) {
            printf("%s()\n", token_type_arr[token->type]);
        }
        else {
            printf("%s(%s) ", token_type_arr[token->type], token->value);
        }
    }
#if 1
    struct Node node = {};
    parser(&node);
    printf("\nNode:\n");
    char temp[TEMP_MAX];
    temp[0] = '\0';
    printf("%s\n", print_node(temp, &node));
    printf("memory used %d", memory_idx);
#endif
#if 0
    printf("\n\njs:\n");
    generate_html(&node);
#endif 
    return 0;
}
