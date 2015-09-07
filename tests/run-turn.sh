NEWFILES="data/185.dat datum parteien parteien.full passwd score turn"
cleanup () {
rm -rf reports $NEWFILES
}

setup() {
ln -sf ../scripts/config.lua
}

quit() {
test -n "$2" && echo $2
echo "integration tests: FAILED"
exit $1
}

assert_grep_count() {
file=$1
expr=$2
expect=$3
count=`grep -cE $expr $file`
[ $count -eq $expect ] || quit 1 "expected $expect counts of $expr in $file, got $count"
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
assert_grep_count reports/$CRFILE '^REGION' 7
assert_grep_count reports/$CRFILE '^PARTEI' 2
assert_grep_count reports/$CRFILE '^SCHIFF' 1
assert_grep_count reports/$CRFILE '^BURG' 1
assert_grep_count reports/$CRFILE '^EINHEIT' 2
assert_grep_count reports/$CRFILE '^GEGENSTAENDE' 2
echo "integration tests: PASS"
cleanup
