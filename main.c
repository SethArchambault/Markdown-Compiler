#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Article { 
    char group_name[30]; char file[60]; char article_name[200]; 
    char *header; char *footer; char *title;
};

#define HEADER_MAX 10000
#define FOOTER_MAX 10000
char header[HEADER_MAX];
char footer[FOOTER_MAX];
#include "series/articles.h"

// could include from a common file
#define assert(cond) {if (!(cond)){ printf("\n%s:%d:5: error: Assert failed: %s\n", __FILE__, __LINE__, #cond); *(volatile int * )0 = 0;}}

#define GROUPS_MAX 			20
#define GROUP_NAME_MAX  	20
#define GROUP_ARTICLES_MAX  20
#define ARTICLE_NAME_MAX  	20

#define LINK_NAME_MAX       200
#define LINK_SRC_MAX        200

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

// not used
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
    buffer[len] = '\0';
}

void createHtmlFromLinks(char * html, struct Link *link) {
    for (int i = 0; strlen(link[i].src) != 0; ++i) { 
        sprintf(html, "%s<a href='%s'>%s</a><br>",
                html, link[i].src, link[i].name);
    }
}

int main() {
    int memory_allocated = 13400000;
    char * memory = malloc(memory_allocated);

    /* some memory test
    for (int i = 0; i < memory_allocated;++i) {
        memory[i] = 'x';
    }
    */

    // templates
    read_file("templates/single_header.chtml", header, HEADER_MAX);
    read_file("templates/single_footer.chtml", footer, FOOTER_MAX);

    // check out articles.h if you want to add new routes..
    for (int i = 0; articles[i].file[0] != '\0'; ++i) {
        struct Article * a = &articles[i];
        markdown_compiler(memory, memory_allocated, a->group_name, a->file, a->article_name, a->header, a->footer, a->title);
        //printf("done %d\n", i);
    }
    // read file
    printf("DONE\n");
    return 0;
}
