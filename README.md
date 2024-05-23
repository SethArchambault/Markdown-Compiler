# Markdown-Compiler
A minimal markdown compiler for my personal website, written in C in a Handmade
style. Not really recommended for public use, but if you're adventurous go
ahead.. Also if you make something better let me know!

# Setup

This will 
- Generate the folder structure
- Create a sample file
- Download the compiler
- Build the website
- Open the webite

```
mkdir my_website
cd my_website
mkdir -p public/series
mkdir -p html_generator/series
echo "# a heading\nregular text, _italic_, **bold**, [link](https://handmade.network.com)\n> quote\n---\na line\n- bullet" > html_generator/series/example-blog.txt
git clone git@github.com:SethArchambault/Markdown-Compiler.git
cd Markdown-Compiler
./build.sh
open public/series/example-blog-url.html
```




[Casey Muratori - Compression Oriented Programming](https://caseymuratori.com/blog_0015)

- [ ] Remove post specific stuff from main.c


