#!/usr/bin/env bash
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#

# Modified by DDU

myshuf() {
  perl -MList::Util=shuffle -e 'print shuffle(<>);' "$@";
}

normalize_text() {
  tr '[:upper:]' '[:lower:]' | sed -e 's/^/__label__/g' | \
    sed -e "s/'/ ' /g" -e 's/"//g' -e 's/\./ \. /g' -e 's/<br \/>/ /g' \
        -e 's/,/ , /g' -e 's/(/ ( /g' -e 's/)/ ) /g' -e 's/\!/ \! /g' \
        -e 's/\?/ \? /g' -e 's/\;/ /g' -e 's/\:/ /g' | tr -s " " | myshuf
}

DATADIR=data

mkdir -p "${DATADIR}"

if [ ! -f "./newsgroups.train" ]
then
    wget -c "http://qwone.com/~jason/20Newsgroups/20news-18828.tar.gz" -O "${DATADIR}/newsgroups.tar.gz"
    tar -xzf "${DATADIR}/newsgroups.tar.gz" -C "${DATADIR}"
    echo "Getting training data ready ..."
    (( i = 0 ))
    for label in `ls ${DATADIR}/20news-18828/`; do
        for file in `ls ${DATADIR}/20news-18828/${label}`; do
            echo $i , ${label} , "`cat ${DATADIR}/20news-18828/${label}/${file} | tr '\n' ' ' | tr '\t' ' ' | tr -d '\200-\377' `" >> ./newsgroups.csv
        done
        (( i = $i + 1 ))
    done
    cat "./newsgroups.csv" | normalize_text > "./newsgroups.train"
    echo "done. Compiling fasttext ... "
fi

make

./fasttext.x pwv -input "./newsgroups.train" -output "./newsgroups" -dim 120 -lr 0.01 -ws 5 -epoch 1 -minCount 5 -neg 30 -bucket 1000000 -minn 3 -maxn 6 -t 1e-4 -lrUpdateRate 100

