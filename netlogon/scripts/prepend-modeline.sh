#!/bin/bash
file="$1"
tmpfile=$file.tmp

if [ $# -ne 1 ]; then
    echo "$0 <filename>" >&2
    echo "Adds a modeline for vim and emacs to the top of the file." >&2
    echo "" >&2
    exit 1
fi

if grep 'Editor Settings:' "$file" >/dev/null; then
    echo "$file already has a mode line in it" >&2
    exit 1
fi

echo '/* Editor Settings: expandtabs and use 4 spaces for indentation' >> "$tmpfile"
echo ' * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *' >> "$tmpfile"
echo ' * -*- mode: c, c-basic-offset: 4 -*- */' >> "$tmpfile"
echo ' ' >> "$tmpfile"

cat "$file" >> "$tmpfile"
mv "$tmpfile" "$file"
