#!/bin/bash
echo "running build.sh"
module=eressea
rootdir=/home/cvs/checkout/${module}
# www=$rootdir/www-data -- no web archives of eressea source
subdir="source"

date >> ${rootdir}/buildlog

echo ==---------------------==
echo `date` : Rebuilding ${module}
echo ==---------------------==

for sub in $subdir
do
  # update the source tree
  cd $rootdir/$sub
  cvs -q update -drP
  # create the source archive
  # cd $rootdir
  # tar czf ${www}/downloads/${module}-${sub}.tar.gz ${sub}
done
cat >| parameters
${rootdir}/senddiff.pl `cat parameters` 2>&1 >> /tmp/senddiff.log
