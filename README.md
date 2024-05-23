# Markdown-Compiler
A minimal markdown compiler for my personal website, written in C in a Handmade
style. Not really recommended for public use, but if you're adventurous go
ahead.. Also if you make something better let me know!

# Setup on MacOS

This will: 

- Generate the folder structure
- Create a sample file
- Download the compiler
- Build the website
- Open the webite
- Open the example file

```
mkdir my_website
cd my_website
mkdir -p public/series
mkdir -p html_generator/series
echo "# a heading\nregular text, _italic_, **bold**, [link](https://handmade.network.com)\n> quote\n---\na line\n- bullet" > html_generator/series/example-blog.txt
git clone git@github.com:SethArchambault/Markdown-Compiler.git
cd Markdown-Compiler
./build.sh
cd ..
open Markdown-Compiler/series/articles.h
open public/series/example-blog-url.html
open html_generator/series/example-blog.txt
```

# Notes

- Articles go in: html_generator/series
- And come out:  public/series
- Each new file will need a line in Markdown-Compiler/series/articles.h
- The header is in Markdown-Compiler/single_header.chtml
- The footer is in Markdown-Compiler/single_footer.chtml
- Notice that h1 tags `#` will have navigation that shows up in the sidebar.
- This is assuming your server is pointing at the public folder, and that you've got an index there that links to some of the pages in the series folder. Don't ask me why it's called series, I don't remember.






[Casey Muratori - Compression Oriented Programming](https://caseymuratori.com/blog_0015)

- [ ] Remove post specific stuff from main.c


