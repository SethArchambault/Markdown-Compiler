set -e
clear
#gcc -g -Wall -Wextra markdown_compiler.c main.c -o main
# thanks to Martins for the flags
gcc -g markdown_compiler.c main.c -o main -fsanitize=undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -Wshadow -Wconversion -Wno-deprecated-declarations -Wno-unused-parameter -ferror-limit=400 
#gcc -g -fno-pie -no-pie -fsanitize=address -Wall -Wextra markdown_compiler.c main.c -o main
./main
