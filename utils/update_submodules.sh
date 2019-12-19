#!/bin/bash

if [[ $# -eq 0 ]] ; then
	target_head="origin/master"
else
	target_head=$1
fi

echo "Updating all libs to point to $target_head"

find . -maxdepth 2 -type d -name "lib" | while read lib_d; do
	echo "Updating '$lib_d'..."
	cd "$lib_d"
	git fetch
	git checkout $target_head
	cd -
done

