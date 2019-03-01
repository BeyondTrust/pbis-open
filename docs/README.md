# Generating BeyondTrust AD Bridge man pages

Man pages are generated from the source markdown files (in md/) via the Makefile.
This uses `ronn` to generate the roff format man files e.g. `ronn -r file.md`
You can view the markdown files as formatted man pages via `ronn --man file.md`

Note: the source markdown files are first processed by a script to conditionally include/exclude
BeyondTrust AD Bridge Enterprise only sections, and then by a second script to add troff paragraph indentation codes..

The supported format is specified in [ronn-format](https://rtomayko.github.io/ronn/ronn-format.7)
and [ronn](https://rtomayko.github.io/ronn/ronn.1.html) covers the use of ronn itself.
