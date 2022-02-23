#!/bin/bash

SOURCE=thesis

FILE_ENDINGS=(acn acr alg aux bbl blg glg glo gls fdb_latexmk fls idx ilg ind ist loa lof log lot out synctex.gz toc)

for i in "${FILE_ENDINGS[@]}"
do
   : 
   rm -f ${SOURCE}.$i
done