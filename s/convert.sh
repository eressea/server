#!/bin/bash
set -euo pipefail

while [ -n "${1+x}" ] ; do
    tmpfile=$(mktemp eressea.XXX)
    iconv -f latin1 -t utf-8 < "$1" | \
        perl -pe 's/ß/ss/; s/ä/ae/; s/ü/ue/; s/ö/oe/;' \
        > "$tmpfile" && \mv "$tmpfile" "$1"
    file "$1"
    shift 1
done
