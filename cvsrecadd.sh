#! /bin/sh
# Recursive cvs add and commit
# (c) 2002 Hugo Haas <http://larve.net/people/hugo/>
# License: GPL
#
# http://larve.net/people/hugo/2002/06/rcvsac
#
# $Id: cvsrecadd.sh,v 1.1 2007/07/23 01:18:05 jwthomp Exp $

cvswrapper() {
    echo Running cvs with arguments: "$@"
    cvs "$@"
}

add() {
    if [ $1 -eq 1 ]; then
	CVSOPTIONS="-kb"
    else
	CVSOPTIONS=""
    fi
    shift
    if [ "x$*" != "x" ]; then
	cvswrapper add $CVSOPTIONS $*
    fi
}

if [ $# -eq 0 -o $# -gt 2 ]; then
    echo "Usage: $0 <directory to add> [ comment ]"
    exit 0
fi

DIRS=`find $1 -type d | egrep -v '/CVS$'`

for d in $DIRS; do
  if [ ! -d $d/CVS ]; then
      cvswrapper add $d
  else
      echo "$d already in CVS"
  fi
done

for d in $DIRS; do
  cd $d
  TOADD=""
  TOADDKB=""
  for f in `find . -maxdepth 1 -type f | sed -e 's|^./||' | egrep -v '(^\.?#|~$)'`; do
    egrep "^/$f/" CVS/Entries > /dev/null
    if [ $? -ne 0 ]; then
	echo $f | egrep -i '\.(jpg|png|gif|mov|avi|gz|zip|jar|bz2|ps|pdf|doc|ppt)$' > /dev/null
	if [ $? -eq 0 ]; then
	    TOADDKB="$TOADDKB $f"
	else
	    TOADD="$TOADD $f"
	fi
	else
	echo "$d/$f already in CVS"
    fi
  done
  add 0 $TOADD
  add 1 $TOADDKB
  cd -
done

if [ "x$2" != "x" ]; then
    cvswrapper commit -m "$2" $1
else
    cvswrapper commit $1
fi

