nmap <F8> :!./build.sh && ./main "markdown.txt" "test"<CR>
nmap <F9> :call system('./build.sh && /seth/system/bin/codeClap main main.c:' . line("."))<CR>
