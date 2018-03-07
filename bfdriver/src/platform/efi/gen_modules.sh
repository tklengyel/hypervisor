#!/bin/bash

modularize()
{
    cp $1 modules/
    cd modules
    filename=$(basename "$1")
    xxd -i "$filename" > ${filename}.c
    rm -f "$filename"
    name="${filename%.*}"
    echo "HYPERVISOR_MODULE($name)" >> modlist.h
    cd -
}

set -e

if [ -z $1 ] ; then
    echo "$0: please pass a list of files to include in bfloader"
    exit 1
fi

rm -f modules/*

for file in $@ ; do
    modularize $file
done

if [ ! -z $(cat modules/modlist.h | sort | uniq -d) ] ; then
    echo "$0: duplicate module detected"
    exit 1 
fi

