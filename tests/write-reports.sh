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

#set -e
cd $ROOT/tests
setup
#cleanup
VALGRIND=`which valgrind`
TESTS=../Debug/eressea/test_eressea
SERVER=../Debug/eressea/eressea
if [ -n "$VALGRIND" ]; then
SUPP=../share/ubuntu-12_04.supp
VALGRIND="$VALGRIND --track-origins=yes --gen-suppressions=all --suppressions=$SUPP --error-exitcode=1 --leak-check=no"
fi
echo "running $TESTS"
$VALGRIND $TESTS
echo "running $SERVER"
$VALGRIND $SERVER -t 184 ../scripts/reports.lua
[ -d reports ] || quit 4 "no reports directory created"
CRFILE=184-zvto.cr
grep -q PARTEI reports/$CRFILE || quit 1 "CR did not contain any factions"
grep -q -E '"B.ume";type' reports/$CRFILE || \
	quit 1 "regions did not contain trees"
grep -q -E '"Silber";type' reports/$CRFILE || \
	quit 1 "regions did not contain silver"
grep -q '"Bauern";type' reports/$CRFILE || \
	quit 1 "regions did not contain peasants"
grep -q '"Sch..linge";type' reports/$CRFILE || \
	quit 1 "regions did not contain saplings"
grep -q REGION reports/$CRFILE || quit 2 "CR did not contain any regions"
grep -q SCHIFF reports/$CRFILE || quit 3 "CR did not contain any ships"
grep -q BURG reports/$CRFILE || quit 4 "CR did not contain any buildings"
grep -q EINHEIT reports/$CRFILE || quit 5 "CR did not contain any units"
grep -q GEGENSTAENDE reports/$CRFILE || quit 6 "CR did not contain any items"
echo "integration tests: PASS"
cleanup
