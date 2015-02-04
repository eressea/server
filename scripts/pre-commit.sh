#!/bin/sh
#
# An example hook script to verify what is about to be committed.
# Called by "git commit" with no arguments.  The hook should
# exit with non-zero status after issuing an appropriate message if
# it wants to stop the commit.

if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

# If you want to allow non-ASCII filenames set this variable to true.
allownonascii=$(git config --bool hooks.allownonascii)

# Redirect output to stderr.
exec 1>&2

# Cross platform projects tend to avoid non-ASCII filenames; prevent
# them from being added to the repository. We exploit the fact that the
# printable range starts at the space character and ends with tilde.
if [ "$allownonascii" != "true" ] &&
	# Note that the use of brackets around a tr range is ok here, (it's
	# even required, for portability to Solaris 10's /usr/bin/tr), since
	# the square bracket bytes happen to fall in the designated range.
	test $(git diff --cached --name-only --diff-filter=A -z $against |
	  LC_ALL=C tr -d '[ -~]\0' | wc -c) != 0
then
	cat <<\EOF
Error: Attempt to add a non-ASCII file name.

This can cause problems if you want to work with people on other platforms.

To be portable it is advisable to rename the file.

If you know what you are doing you can disable this check using:

  git config hooks.allownonascii true
EOF
	exit 1
fi

# If there are whitespace errors, print the offending file names and fail.
git diff-index --check --cached $against --

if [ $? -ne 0 ]; then
    echo "Whitespace errors, aborting."
    exit 1
fi

ASTYLE_FILE=.astylerc

if [ ! -f $ASTYLE_FILE ]; then
    echo "Could not find astyle options file $ASTYLE_FILE, aborting."
    echo $(pwd)
    exit 1
fi

ASTYLE_OPT=--options=$ASTYLE_FILE
PREFIX=pre-commit~~

# copy every file to tmp file and format it with astyle
# exit 1 if astyle would format it
git diff-index --cached --name-only HEAD | while read filename; do
    if [ "${filename##*.}" = "c" ] || [ "${filename##*.}" = "h" ]; then
	tmpname=${PREFIX}.${filename##*.}
	git show :"${filename}" -- > "$tmpname"
	result=$(astyle -Q $ASTYLE_OPT "$tmpname" | egrep "^Formatted")
	rm "$tmpname" "$tmpname".orig
	if [ ! -z "$result" ]; then
	    echo "$filename does not meet coding standards, aborting."
	    cat <<\EOF
Please run
    astyle --option=.astylerc $file
or
    astyle --options=.astylerc -r "src/*.c" "src/*.h"
EOF
	    exit 1
	fi
    fi
done

if [ $? -eq 0 ]; then
    echo "Formatting okay, but did you remember to add untracked files?"
else
    exit 1
fi
