
# *ted.*

the **t**iny **ed**itor.

(or the **t**erminal **ed**itor i guess, use whatever)

## *build.*

you can use the Makefile.

    make

or you can straight up just compile it with the compiler of your choice.

    gcc ted.c -o ted -lncurses

## *use.*

just

    ted -file <name>

save with Ctrl+O.

exit with Ctrl+X.

## *warning.*

 - *ted* does not read existing files as of now. if you provide an existing file, *it will be replaced.*
 - no way to select text as of now.


