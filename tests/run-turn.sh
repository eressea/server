NEWFILES="data/185.dat datum parteien parteien.full passwd score turn"
cleanup () {
rm -rf reports $NEWFILES
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

set -e
cd $ROOT/tests
setup
cleanup
VALGRIND=`which valgrind`
SERVER=../Debug/eressea/eressea
if [ -n "$VALGRIND" ]; then
SUPP=../share/ubuntu-12_04.supp
SERVER="$VALGRIND --suppressions=$SUPP --error-exitcode=1 --leak-check=no $SERVER"
fi
echo "running $SERVER"
$SERVER -t 184 ../scripts/run-turn.lua
[ -d reports ] || quit 4 "no reports directory created"
CRFILE=185-zvto.cr
for file in $NEWFILES reports/$CRFILE ; do
  [ -e $file ] || quit 5 "did not create $file"
done
grep -q PARTEI reports/$CRFILE || quit 1 "CR did not contain any factions"
grep -q REGION reports/$CRFILE || quit 2 "CR did not contain any regions"
grep -q SCHIFF reports/$CRFILE || quit 3 "CR did not contain any ships"
grep -q BURG reports/$CRFILE || quit 4 "CR did not contain any buildings"
grep -q EINHEIT reports/$CRFILE || quit 5 "CR did not contain any units"
grep -q GEGENSTAENDE reports/$CRFILE || quit 6 "CR did not contain any items"
echo "integration tests: PASS"
cleanup
