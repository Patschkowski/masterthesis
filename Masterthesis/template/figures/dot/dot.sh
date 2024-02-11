#! /bin/sh

s=$(pwd)
r=$(find $s -iname '*.dot' -type f)

for i in $r; do 
	t=$(echo $i | sed -e 's/\.dot//g' -e 's/dot\///g')
	cat $i | dot -Tpdf > $t.pdf;
	echo $t.pdf
done
