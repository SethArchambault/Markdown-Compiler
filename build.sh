set -e
clear
#gcc -g -Wall -Wextra markdown_compiler.c main.c -o main
gcc -g -fno-pie -fsanitize=address -Wall -Wextra markdown_compiler.c main.c -o main
#gcc -g -fno-pie -no-pie -fsanitize=address -Wall -Wextra markdown_compiler.c main.c -o main
./main
