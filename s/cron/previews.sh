#!/bin/sh

function abort() {
echo $1
[ -z $2 ] && exit -1
exit $2 # otherwise
}

[ -z ${ERESSEA} ] && ERESSEA=~/eressea
branch="master"
if [ -e ${ERESSEA}/build/.preview ]; then
    branch=`cat ${ERESSEA}/build/.preview`
fi
SRC=${ERESSEA}/git
${SRC}/s/preview build ${branch} || abort "failed to build ${branch}"
${SRC}/s/preview version
for game in 2 3 4 ; do
	${SRC}/s/preview -g ${game} run && \
	${SRC}/s/preview -g ${game} send
done
