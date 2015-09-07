GAME=$1
TURN=$2

if [ ! -d $ERESSEA/game-$GAME ] ; then
  echo "No such game: $GAME"
  exit 1
fi

cd $ERESSEA/game-$GAME
if [ -z $TURN ]; then
 TURN=$(cat turn)
fi

echo "running turn $TURN, game $GAME"
if [ -d orders.dir.$TURN ]; then
  echo "orders.dir.$TURN already exists"
else
  mv orders.dir orders.dir.$TURN
  mkdir -p orders.dir
fi
ls -1rt orders.dir.$TURN/turn-* | xargs cat > orders.$TURN
$ERESSEA/bin/eressea -t $TURN run-turn.lua
