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

#define assert(cond) {if (!(cond)){ print_debug_log(); printf("\n%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}
#define assert_d(cond, arg) {if (!(cond)){ print_debug_log(); printf("Assert Failed: %s, %d\n", #cond, arg); *(volatile int * )0 = 0;}}
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
#define TOKEN_ARR_MAX 2000
#define TOKEN_RULES_MAX 6

#define REGEX_ARR_COUNT 20

#define REGEX_STR_MAX 50
#define TOKEN_STR_MAX 100
#define TOKEN_RULE_STR_MAX 20

#define CAPTURE_STR_MAX 20000

#define TEMP_MAX 10000

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
    t(code)            \
    t(config)           \
    t(text)             \
    t(obold)            \
    t(cbold)            \
    t(oitalic)          \
    t(citalic)          \
    t(otagimage)        \
    t(otagtext)         \
    t(href)             \
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
    "[:code:]",                 
    "[:config:]",
    "[:text:]",
    "[:obold:]",                 
    "[:cbold:]",                 
    "[:oitalic:]",                 
    "[:citalic:]",                 
    "[:otagimage:]",                 
    "[:otagtext:]",                 
    "[:href:]",                 
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


// @TODO - just make this return the number of matched characters
int is_match(const char * c1, const char * c2) {
    for(int i = 0;c2[i] != '\0';++i) {
        if(c1[i] == '\0') return 0;
        if(c1[i] != c2[i]) return 0;
    }
    return 1;
}


struct TokenizerState {
    int italic_opened;
    int bold_opened;
    int tag_text_opened;
    int tag_href_opened;
    int code_opened;
};

struct TokenizerState tokenizer_state = {0};

int is_bold(const char * c) {
    if(! is_match(c, "**")) return 0;
    if (tokenizer_state.bold_opened) return 1;
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
    if (tokenizer_state.italic_opened) return 1;
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
        tokenizer_state.tag_text_opened) {
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
        else if (is_match(&regex[regex_idx], "[:code:]")){
            if(!is_match(&code[code_idx], "```")) return 0;
            int i = 3;
            for(;;++i) {
                if (code[code_idx + i] == '\0') return 0;
                if (is_match(&code[code_idx + i], "```")) break;
            }
            i += 3;
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:text:]")) {
            debug("text match\n");
            // capture text until you find a link or the end of the line
            int i = 0;
            for(;;++i) {
                if (is_inline_tag(&code[code_idx + i])) break;
                if (is_bold(&code[code_idx + i])) break;
                if (is_italic_char(&code[code_idx + i])) break;
                if (is_eol(code[code_idx + i])) break;
            }
            if (i == 0) return 0; // must match at least one char
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:obold:]")){
            if( is_match(&code[code_idx], "**") &&
                !tokenizer_state.bold_opened) 
            {
                tokenizer_state.bold_opened = 1;
                inc(code_idx, 2, CODE_FILE_MAX);
                break;
            }
            return 0;
        }
        else if (is_match(&regex[regex_idx], "[:cbold:]")){
            if(!is_match(&code[code_idx], "**")) return 0;
            if(!tokenizer_state.bold_opened) return 0;
            tokenizer_state.bold_opened = 0;
            inc(code_idx, 2, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:oitalic:]")){
            if(code[code_idx] != '_') return 0;
            if(tokenizer_state.italic_opened) return 0;
            tokenizer_state.italic_opened = 1;
            inc(code_idx, 1, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:citalic:]")){
            if(code[code_idx] != '_') return 0;
            if(!tokenizer_state.italic_opened) return 0;
            tokenizer_state.italic_opened = 0;
            inc(code_idx, 1, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:otagimage:]")){
            if(!is_match(&code[code_idx], "![")) return 0;
            if(tokenizer_state.tag_text_opened) return 0;
            tokenizer_state.tag_text_opened = 1;
            inc(code_idx, 2, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:otagtext:]")){
            if(code[code_idx] != '[') return 0;
            if(tokenizer_state.tag_text_opened) return 0;
            tokenizer_state.tag_text_opened = 1;
            inc(code_idx, 1, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:href:]")){
            if(!is_match(&code[code_idx], "](")) return 0;
            if(tokenizer_state.tag_text_opened == 0) return 0;
            tokenizer_state.tag_text_opened = 0;
            int i = 2;
            for (;;++i) {
                if (is_eol(code[code_idx+i])) return 0;
                if (code[code_idx+i] == ')') break;
            }
            ++i;
            inc(code_idx, i, CODE_FILE_MAX);
            break;
        }
        else if (is_match(&regex[regex_idx], "[:ctaghref:]")){
            if(code[code_idx] != ')') return 0;
            if(!tokenizer_state.tag_href_opened) return 0;
            tokenizer_state.tag_text_opened = 0;
            inc(code_idx, 1, CODE_FILE_MAX);
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
int memory_allocated = 13400000;
int memory_idx = 0;

void * allocate(int memory_needed) {
    int memory_free = (memory_allocated-memory_idx);
    assert_d(memory_needed < memory_free, memory_allocated + memory_needed);
    void * block = &memory[memory_idx];
    memory_idx += memory_needed;
    //printf("allocated   %20d    needed %20d\n", memory_free, memory_needed);
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
                // @coupling: I can't increment code_idx more than the 
                // capture string size, which I want to do.
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
    // hmm...
    tokens = tokens + 1;
    //printf("token %d\n", token_count_debug);
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
    f(NODE_IMAGE)       \
    f(NODE_NL)          \
    f(NODE_CODE)        \

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
            char * href;
        } link;
        struct { // NODE_IMAGE
            struct Node * text;
            char * href;
        } image;
        struct { // NODE_CODE
            char * value;
        } code;
        struct { // NODE_ITALIC
            struct Node * inside;
        } italic;
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
    consume(otagtext);
    node->link.text = allocate(sizeof(struct Node));
    parse_any(node->link.text);
    Token * href_token = consume(href);
    node->link.href = allocate(strlen(href_token->value)+1);
    strcpy(node->link.href, href_token->value);
}
void parse_image(struct Node * node) {
    assert(node);
    node->type = NODE_IMAGE;
    consume(otagimage);
    node->image.text = allocate(sizeof(struct Node));
    parse_any(node->image.text);
    Token * href_token = consume(href);
    node->image.href = allocate(strlen(href_token->value)+1);
    strcpy(node->image.href, href_token->value);
}

void parse_bold(struct Node * node) {
    assert(node);
    node->type = NODE_BOLD;
    consume(obold);
    node->bold.inside = allocate(sizeof(struct Node));
    parse_any(node->bold.inside);
    consume(cbold);
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
        case href:       return 1;
        case citalic:    return 1;
        case cbold:      return 1;
        default:    return 0;
    }
}

void parse_any(struct Node *node) {
    switch(tokens->type) {
        case otagtext:
            parse_link(node);
            break;
        case otagimage:
            parse_image(node);
            break;
        case code:
            parse_code(node);
            break;
        case config:
			//@TODO; implement config
            break;
        case href:
            return;
        case obold:
            parse_bold(node);
            break;
        case cbold:
            return;
        case oitalic:
            parse_italic(node);
            break;
        case citalic:
            return;
        case text:
            parse_text(node);
            break;
        case nl:
            parse_nl(node);
            break;
        case eof:
            printf("END OF FILE");
            break;
        default:
            printf("parse any: token type not found: %s\n", token_type_arr[tokens->type]);
            assert(0);
    }
    if (!peek(eof)) {
        // Here, a node has been allocated, but there might not be a next token
        if (!is_deadend(tokens->type)) {
            node->next = allocate(sizeof (struct Node));
            parse_any(node->next);
        }
    }
    else {
        node->next = NULL;
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
            string_cat(t, "LINK href=\"", TEMP_MAX);
            string_cat(t, node->link.href, TEMP_MAX);
            string_cat(t, "\"", TEMP_MAX);
            print_node(t, node->link.text, inside_indent);
            break;
        }
        case NODE_IMAGE: {
            string_cat(t, "IMAGE href=\"", TEMP_MAX);
            string_cat(t, node->link.href, TEMP_MAX);
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
    }
    struct Node * next = node->next;
    if (next) {
        print_node(t, next, indent);
    }
}
#endif

void generate_html(struct Node * node) {
    assert(node);
    switch(node->type){
        case NodeTypeEnd:
            assert(0 && "generate_html - NODE_TYPE_END");
            break;
        case NODE_UNDEFINED:
            printf("NODE_UNDEFINED");
            //assert(0 && "generate_html - NODE_UNDEFINED");
            break;
        case NODE_NL:
            printf("\n");
            break;
        case NODE_TEXT:
            printf("%s", node->text.value);
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
            printf("<a href=\"%s\">", node->link.href);
            generate_html(node->link.text);
            printf("</a>");
            break;
        case NODE_IMAGE:
            printf("<img src=\"%s\" alt=\"", node->image.href);
            generate_html(node->image.text);
            printf("\">");
            break;
        case NODE_CODE:
            printf("<pre>");
            printf("%s", node->code.value);
            printf("</pre>");
            break;
    }
    if (node->next != NULL) {
        generate_html(node->next);
    }
}


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

    printf("\n-------Code-------\n%s", code);
    tokenizer(code);
    printf("\n------Tokens------\n");
    for(Token *token = tokens; token->type != eof; token = &token[1]) {
        if (token->type == nl) {
            printf("%s()\n", token_type_arr[token->type]);
        }
        else {
            printf("%s(%s) ", token_type_arr[token->type], token->value);
        }
    }
    struct Node node = {};
    parser(&node);
    printf("\n\n------Nodes-------");
    char temp[TEMP_MAX];
    temp[0] = '\0';
    print_node(temp, &node, 0);
    printf("%s\n", temp);
    printf("\n------Html--------\n");
    generate_html(&node);
    printf("\nmemory used %d", memory_idx);
    return 0;
}
