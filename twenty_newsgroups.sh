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
    rm -rf newsgroups.csv
    (( i = 0 ))
    for label in `ls ${DATADIR}/20news-18828/`; do
        for file in `ls ${DATADIR}/20news-18828/${label}`; do
            echo "${i}~${label}~""`cat ${DATADIR}/20news-18828/${label}/${file} | sed 1,"/^$/d" | tr '\n' ' ' | tr '\t' ' ' | tr '~' ' ' | tr -d '\200-\377' `" >> ./newsgroups.csv
        done
        (( i = $i + 1 ))
    done
    cat ./newsgroups.csv | normalize_text > ./newsgroups.all
    head -n 13180 newsgroups.all | awk -F'~' '{print $1" , "$2" , "$3}' > newsgroups_polar.train
    #head -n 13180 newsgroups.all | awk -F'~' '{print $1" , "$3}' > newsgroups_polar.train
    head -n 13180 newsgroups.all | awk -F'~' '{print $1" , "$3}' > newsgroups.train
    tail -n 5648 newsgroups.all | awk -F'~' '{print $1" , "$3}' > newsgroups.test
    echo "done. Compiling ... "
fi

make

echo 
echo "Training without polarization term..."
./fasttext.x supervised -input "./newsgroups.train" -output "./newsgroups" -dim 20 -lr 0.2 -wordNgrams 1 -minCount 1 -bucket 1000000 -epoch 100 -thread 8 # -pretrainedVectors "./newsgroups.vec"
./fasttext.x test "./newsgroups.bin" "./newsgroups.test"

echo
echo "Do polarized word embedding..."
./fasttext.x pwv -input "./newsgroups_polar.train" -output "./newsgroups_polar" -dim 20 -lr 0.05 -wordNgrams 1 -ws 6 -epoch 3 -minCount 1 -neg 20 -bucket 1000000 -minn 3 -maxn 6 -t 1e-4 -lrUpdateRate 100

echo 
echo "Training with pretrained polarized embedding"

./fasttext.x supervised -input "./newsgroups.train" -output "./newsgroups" -dim 20 -lr 0.2 -wordNgrams 1 -minCount 1 -bucket 1000000 -epoch 100 -thread 8 -pretrainedVectors "./newsgroups_polar.vec"
./fasttext.x test "./newsgroups.bin" "./newsgroups.test"


