#!/bin/bash

LOCKFILE=/tmp/eressea.lock

function usage() {
  cat <<HEREDOC
usage: $0 [-t <turn>] [-g <game>] [ -h ] command [args]
commands:
  usage or -h option -- print this info
  update -- pull, rebuild, and install the code
  reports -- write reports
  send [id ...] -- send reports to one or more factions, or to all factions
  fetch -- fetch orders from mail server
  create-orders -- process all orders in queue to be processed by server
  run -- run a turn
  all -- do a complete cycle: create-orders, run, send
HEREDOC
  exit 1
}

function abort() {
  echo $1
  free_lock $LOCKFILE
  [ -z $2 ] && exit -1
  exit $2 # otherwise
}

function assert_dir() {
  [ -z $1 ] && abort "missing directory name"
  [ -d $1 ] || abort "missing directory ($1)"
}

function assert_file() {
  [ -z $1 ] && abort "missing file name"
  [ -f $1 ] || abort "missing file ($1)"
}

function acquire_lock() {
  lockfile -10 -r 30 $1 || abort "could not acquire lock"
}

function free_lock() {
  rm -f $1
}

function get_turn() {
  if [ $turn -eq -1 ]; then
    assert_file $GAMEDIR/turn
    let turn=$(cat $GAMEDIR/turn)
  fi
}


function update() {
  assert_dir $SOURCE
  cd $SOURCE
  git fetch || abort "failed to update source. do you have local changes?"
  [ -z $1 ] || git checkout $1
  git pull -q
  git submodule update
  s/build || abort "build failed."
  s/install || abort "install failed."
}

function parse_config() {
  [ -e $GAMEDIR/eressea.ini ] || abort "file $GAMEDIR/eressea.ini not found"
  $BINDIR/inifile $GAMEDIR/eressea.ini get "$1:$2"
}

function send_report() {
  zip="$turn-$1.zip"
  zip -q -u $zip $turn-$1.?r
  email=$(grep "faction=$1:" reports.txt | cut -d: -f2 | sed 's/email=//')
  [ -z $email ] && echo "faction $1 not found" && return
  lang=$(grep "faction=$1:" reports.txt | cut -d: -f3 | sed 's/lang=//')
  info=$ETCDIR/report-mail.$lang.txt
  [ -e $info ] || info=$ETCDIR/report-mail.txt
  [ -e $info ] || info=/dev/null
  muttrc=$ETCDIR/muttrc
  [ -e $muttrc ] ||  abort "missing muttrc file $muttrc"
  echo "sending reports to $1 / $email ($lang)"
  cat $info | mutt -F $muttrc -s "$gamename ($game) Report -- $1" -a $zip -- $email
}

function send() {
  sent=0
  assert_dir $GAMEDIR/reports  
  cd $GAMEDIR/reports
  [ -f reports.txt ] || abort "missing reports.txt for game $game"
  for faction in $* ; do
    send_report $faction
    sent=1
  done
  if [ $sent -eq 0 ]; then
    for faction in $(cat reports.txt | cut -f1 -d: | cut -f2 -d=); do
      send_report $faction
    done
  fi
}

function fetchmail() {
  fetchmailrc=$ETCDIR/fetchmailrc
  procmailrc=$ETCDIR/procmailrc
  assert_file $fetchmailrc
  assert_file $procmailrc
  fetchmail -f $fetchmailrc
}

function create_orders() {
  cd $GAMEDIR
  if [ -d orders.dir.$turn ]; then
    echo "orders.dir.$turn already exists"
  else
    mv orders.dir orders.dir.$turn
    mkdir -p orders.dir
  fi
  ls -1rt orders.dir.$turn/turn-* | xargs cat > orders.$turn

  # check if mail processing has finished
  if [ -e orders.queue ] ; then
    lockfile -r3 -l120 orders.queue.lock
    # TODO check exit status and die or press on?
    mv orders.queue orders.dir.$turn/orders.queue
    rm -f orders.queue.lock
  fi
}

function write_reports() {
  assert_file $GAMEDIR/data/$turn.dat
  cd $GAMEDIR
  REPORTS=$GAMEDIR/reports
  [ -d $REPORTS ] && rm -rf $REPORTS
  [ -e $REPORTS ] && echo "could not delete $REPORTS"
  mkdir $REPORTS
  ./eressea -t$turn reports.lua
  compress_reports
}

function compress_reports() {
  assert_dir $GAMEDIR
  cd $GAMEDIR/reports
  $BINDIR/compress.py $turn $gamename
}

function run_turn() {
  echo "running turn $turn of game $game"
  assert_dir $GAMEDIR
  cd $GAMEDIR
  assert_dir data
  assert_file $GAMEDIR/data/$turn.dat
  assert_file $GAMEDIR/orders.$turn
  rm -rf reports
  mkdir -p reports
  
  ./eressea -v$verbose -t$turn $LUADIR/run-turn.lua
  mkdir -p log
  ln -f eressea.log log/eressea.log.$TURN

  let turn=$turn+1
  [ -e data/$turn.dat ] || abort "no data file created"

  compress_reports
}

function run_all() {
  create_orders
  run_turn
  send
}

game=0
turn=-1
verbose=1
while getopts :g:t:v: o; do
  case "${o}" in
    v)
      verbose=${OPTARG}
      ;;
    g)
      game=${OPTARG}
      ;;
    t)
      turn=${OPTARG}
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done
shift $((OPTIND-1))

if [ -z $1 ] || [ "$1" = "usage" ]; then
  usage
  exit 1
fi


[ -z $ERESSEA ] && abort "The ERESSEA environment variable is not set."
assert_dir $ERESSEA
[ -z $SOURCE ] && SOURCE=$ERESSEA/git
GAMEDIR=$ERESSEA/game-$game
assert_dir $GAMEDIR
BINDIR=$ERESSEA/server/bin
assert_dir $BINDIR
LUADIR=$ERESSEA/server/scripts
assert_dir $LUADIR
ETCDIR=$ERESSEA/server/etc
assert_dir $ETCDIR
get_turn

gamename="$(parse_config game name)"

while [ ! -z $1 ]; do
  case "$1" in
    "update")
      acquire_lock $LOCKFILE
      shift
      update $*
      ;;
    "send")
      acquire_lock $LOCKFILE
      shift
      send $*  
      ;;
    "fetch")
      acquire_lock $LOCKFILE
      fetchmail
      ;;
    "create-orders")
      acquire_lock $LOCKFILE
      create_orders
      ;;
    "reports")
      acquire_lock $LOCKFILE
      write_reports
      ;;
    "compress")
      acquire_lock $LOCKFILE
      compress_reports 
      ;;
    "run")
      acquire_lock $LOCKFILE
      run_turn
      ;;
    "all")
      acquire_lock $LOCKFILE
      run_all
      ;;
    "lock")
      acquire_lock $LOCKFILE
      sleep 30
      ;;
    *)
      usage
      ;; 
  esac
  shift
done

free_lock $LOCKFILE

## DONE install etc/procmail,fetchmail...
## DONE install server/Mail
## NOT install no runturn.lua
## TODO procmail in requirements
## TODO execute.lock unnecessary because of LOCKFILE?
## DONE eressea.ini checker options , checker template
## TODO eressea.ini email, sender, name
## turn 0 in tutorial?
## TODO orders-*: lock_file die or unlink?
## TODO create-oders: check exit status and die or press on?


# orders-accept: called by procmail; parse email, write orders.queue and orders.dir; works with orders.queue.lock
## eorders.py: used by orders-process/accept; helper
## epasswd.py: used by orders-accept, checkpasswd; checks passwords against file

# sendreport.sh: called by procmail; sends report to other address, depends on 1234.sh in reports
## functions.sh: used by sendreports, sendreport; helper
## checkpasswd: called by sendreport.sh, calls EPasswd


# orders.cron: called by cron; calls orders-process
## orders-process: called by orders.cron; check passwd, echeck, send confirmation; create lockfile for orders.queue; backs up and removes orders.queue after timeout, removes orders.queue
### checker.sh: called by orders-process; configured in eresse.ini: game.checker
### eorders.py

# **run-eressea.cron: called by cron (or hand); waits for queue.lock, calls create-oders, backup, run-turn, sends reports
## create-orders: called by run-eressea.cron; creates orders.$turn from orders.dir, moves orders.dir to orders.dir.$turn; works with orders.queue.lock
## backup-eressea: called by run-eressea.cron
## run-turn: called by run-eressea.cron; runs server once
## compress.sh: called by run-eressea.cron; calls compress.py
### compress.py: called by compress.sh; compresses reports of factions in reports.txt, creates 1234.sh scripts for reportnachforderung
## sendreports.sh: called by run-eressea.cron; send all reports, uses 1234.sh in reports
### functions.sh
#### send-bz2-report: called by 1234.sh in reports; email one faction's report
#### send-zip-report: more modern version

#run-turn.sh: to be called by cron or hand; calls update, send (one or all, via mutt), fetchmail (and thus procmail), create_orders, write_reports, run_turn, or run_all;

# preview.cron: runs preview reports

# tests/run-turn.sh: called by integration build


# edit fetchmailrc, procmailrc, muttrc
# 
