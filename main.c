#include <stdlib.h>
#include <stdio.h>

// could include from a common file
#define assert(cond) {if (!(cond)){ printf("\n%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}

#define GROUPS_MAX 			20
#define GROUP_NAME_MAX  	20
#define GROUP_ARTICLES_MAX  20
#define ARTICLE_NAME_MAX  	20

#define LINK_NAME_MAX       20
#define LINK_SRC_MAX        20

#define TEMP_MAX            1000
extern void markdown_compiler();

struct Link {
    char name[LINK_NAME_MAX];
    char src[LINK_SRC_MAX];
};

#define CONFIG_TEMPLATE_MAX 20
#define CONFIG_TAGS_MAX     20
#define TAG_MAX             20
#define CONFIG_LINKS_MAX    20

struct Config {
    char template[CONFIG_TEMPLATE_MAX];
    char tags[CONFIG_TAGS_MAX][TAG_MAX];
    struct Link links[CONFIG_LINKS_MAX];
};

void read_file(char * filename, char * buffer, int max) {
    FILE *f = fopen(filename, "r");
    assert(f);
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    assert(len < max);
    fseek(f, 0, SEEK_SET);
    fread(buffer, len, 1, f);
}

#define HEADER_MAX 10000
#define FOOTER_MAX 1000
int main() {
	char groups[GROUPS_MAX][GROUP_NAME_MAX] = {
		"programming",
		"health",
		"whiteness",
	};
	int group_idx[GROUPS_MAX] = {0};
	char articles[GROUPS_MAX][GROUP_ARTICLES_MAX][ARTICLE_NAME_MAX] = {0};
    int memory_allocated = 13400000;
    void * memory = malloc(memory_allocated);
    char header[HEADER_MAX];
    char footer[FOOTER_MAX];
    read_file("templates/single_header.chtml", header, HEADER_MAX);
    read_file("templates/single_footer.chtml", footer, FOOTER_MAX);
    char title[1000] = "title goes here";
    markdown_compiler(memory, memory_allocated, "_single", "markdown.txt", "test", header, footer, title);
    markdown_compiler(memory, memory_allocated, "_single", "markdown2.txt", "test2", header, footer, title);

    // read file
    printf("%d\n", TEMP_MAX);
    
    return 0;
}
