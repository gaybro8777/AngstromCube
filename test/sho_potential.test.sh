#!/usr/bin/env bash

exe=../src/a43

out=sho_potential.out
rm -f $out; echo -n "# " > $out; date >> $out

# for a in   1000    100 010 001     200 110 020 101 011 002      300 210 120 030 201 111 021 102 012 003   400 310 220 130 040 301 211 121 031 202 112 022 103 013 004; do
for a in   1000    100 010 001     200 110 020 101 011 002  ; do
# for a in  200 020 002 ; do  ## the lowest non-linear ones
# for a in  200 ; do  ## the lowest non-linear one

(cd ../src/ && make -j) && \
  $exe +verbosity=11 \
    -test sho_potential. \
    +sho_potential.test.numax=2 \
    +sho_potential.test.sigma=2.0 \
    +sho_potential.test.method=2 \
    +sho_potential.test.artificial.potential=$a \
    +sho_potential.test.clear.coeff=99 \
   >> sho_potential.out

    #grep '# V ai#1 aj#0 ' sho_potential.out
done
grep '# V_coeff ai#0 aj#1 ' $out
