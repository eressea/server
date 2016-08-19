# run list-spells-e3.lua, e.g. build-x86_64-gcc-Debug/eressea/eressea -v0 list-spells-e3.lua
# cp ../reports/*nr e3
cat e3/*nr | perl extractzauber.perl > spells.csv
cat spells.csv | perl csv2mediawikitable.perl > spelltable.mwiki
cat spells.csv | perl csv2mediawikicmplt.perl > spelldescriptions.mwiki
