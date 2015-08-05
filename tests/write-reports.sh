cleanup () {
rm -rf reports score
}

setup() {
ln -sf ../scripts/config.lua
}

quit() {
test -n "$2" && echo $2
exit $1
}

ROOT=`pwd`
while [ ! -d $ROOT/.git ]; do
  ROOT=`dirname $ROOT`
done

cd $ROOT/tests
setup
cleanup
valgrind ../Debug/eressea/eressea -t 184 ../scripts/reports.lua
[ -d reports ] || quit 4 "no reports directory created"
CRFILE=184-zvto.cr
grep -q PARTEI reports/$CRFILE || quit 1 "CR did not contain any factions"
grep -q REGION reports/$CRFILE || quit 2 "CR did not contain any regions"
grep -q SCHIFF reports/$CRFILE || quit 3 "CR did not contain any ships"
grep -q BURG reports/$CRFILE || quit 4 "CR did not contain any buildings"
grep -q EINHEIT reports/$CRFILE || quit 5 "CR did not contain any units"
grep -q GEGENSTAENDE reports/$CRFILE || quit 6 "CR did not contain any items"
echo "integration tests: PASS"
cleanup
