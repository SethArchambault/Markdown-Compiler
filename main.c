#include"markdown_compiler.c"

int main() {
    int memory_allocated = 13400000;
    void * memory = malloc(memory_allocated);
    markdown_compiler(memory, memory_allocated, "markdown.txt", "test");
    markdown_compiler(memory, memory_allocated, "markdown.txt", "test2");
    return 0;
}
