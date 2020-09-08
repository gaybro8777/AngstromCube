#!/usr/bin/env bash

exe=../src/a43

### Al-P dimer
geometry_file=atoms.xyz
printf " 2 \n#cell 4.233418 4.233418 8.466836 p p p \n" > $geometry_file
# printf " 2 \n#cell 10.5835 10.5835 12.7003 p p p \n" > $geometry_file
# printf " 2 \n#cell 21.16708996 21.16708996 25.400507952 p p p \n" > $geometry_file
echo "Al   0 0 -1.058354498" >> $geometry_file
echo "P    0 0  1.058354498" >> $geometry_file
out_file=potential_generator.AlP.pw.out

### Al fcc bulk
geometry_file=atoms.xyz
printf " 4 \n#cell 4.0495 4.0495 4.0495 p p p \n" > $geometry_file
echo "Al   -1.012375 -1.012375 -1.012375" >> $geometry_file
echo "Al    1.012375  1.012375 -1.012375" >> $geometry_file
echo "Al    1.012375 -1.012375  1.012375" >> $geometry_file
echo "Al   -1.012375  1.012375  1.012375" >> $geometry_file
out_file=potential_generator.Al-fcc.pw.out

### P sc bulk
geometry_file=atoms.xyz
printf " 1 \n#cell 3.0 3.0 3.0 p p p \n" > $geometry_file
echo "P 0 0 0" >> $geometry_file
out_file=potential_generator.P-sc.pw.out

### Al sc bulk
geometry_file=atoms.xyz
printf " 1 \n#cell 3.0 3.0 3.0 p p p \n" > $geometry_file
echo "Al 0 0 0" >> $geometry_file
out_file=potential_generator.Al-sc.pw.out


# geometry_file=C_chain.xyz
# printf " 1 \n#cell 1.420282 10 10 p p p \n" > $geometry_file
# echo "C   0 0 0" >> $geometry_file
# out_file=potential_generator.$geometry_file.sho.out

# geometry_file=graphene.xyz
# out_file=potential_generator.graphene.sho.out

(cd ../src/ && make -j) && \
$exe +verbosity=7 \
    -test potential_generator. \
        +geometry.file=$geometry_file \
        +potential_generator.grid.spacing=0.251 \
        +electrostatic.solver=mg \
        +electrostatic.potential.to.file=v_es.mg.dat \
        +occupied.bands=4 \
        +element_C="2s 2 2p 2 0 | 1.2 sigma .8" \
        +element_Al="3s* 2 3p* 1 0 3d | 1.8 sigma 1.1" \
         +element_P="3s* 2 3p* 3 0 3d | 1.8 sigma 1.1" \
        +single_atom.local.potential.method=parabola \
        +single_atom.init.echo=0 \
        +single_atom.init.scf.maxit=1 \
        +single_atom.echo=1 \
        +logder.start=2 +logder.stop=1 \
        +bands.per.atom=4 \
        +potential_generator.max.scf=1 \
        +basis=pw \
        +pw_hamiltonian.cutoff.energy=3.5 \
        +pw_hamiltonian.test.kpoints=17 \
        +pw_hamiltonian.floating.point.bits=64 \
        +pw_hamiltonian.scale.nonlocal.s=1 \
        +dense_solver.test.overlap.eigvals=1 \
        > $out_file

        
exit
#         +electrostatic.solver=load +electrostatic.potential.from.file=v_es.mg.dat \
#         +element_Al="3s* 2 3p* 1 0 3d | 1.8 sigma 1.1" \
#          +element_P="3s* 2 3p* 3 0 3d | 1.8 sigma 1.1" \
#         +electrostatic.solver=mg \
#         +electrostatic.potential.to.file=v_es.mg.dat \
#         +repeat.eigensolver=0 \
#         +eigensolver=cg +conjugate_gradients.max.iter=1 \
#         +start.waves.scale.sigma=5 \
#         +atomic.valence.decay=0 \

#         +basis=sho \
#         +sho_hamiltonian.test.numax=3 \
#         +sho_hamiltonian.test.sigma=1.1 \
#         +sho_hamiltonian.test.overlap.eigvals=1 \
#         +sho_hamiltonian.test.sigma.asymmetry=1 \
#         +sho_hamiltonian.test.kpoints=17 \
#         +sho_hamiltonian.floating.point.bits=64 \
#         +sho_hamiltonian.test.green.function=200 \
#         +sho_hamiltonian.test.green.lehmann=200 \

### simple cubic
printf " 1 \n#cell 12 12 12 i i i\n" > atoms.xyz
echo "Xe   0 0 0" >> atoms.xyz

(cd ../src/ && make -j) && \
$exe +verbosity=11 \
    -test potential_generator. \
        +repeat.eigensolver=5 \
        +eigensolver=cg +conjugate_gradients.max.iter=12 \
        +potential_generator.grid.spacing=0.251 \
        +electrostatic.solver=Bessel0 \
        +element_He="1s* 2 2p | 1.5 sigma .75" \
        +element_Ne="2s 2 2p 6 | 1.8 sigma .7" \
        +element_Ar="3s* 2 3p* 6 3d | 1.6 sigma .8" \
        +element_Ar="3s 2 3p 6 | 1.6 sigma .8" \
        +element_Xe="5s* 2 5p* 6 5d | 2.24 sigma .62" \
        +element_Xe="5s 2 5p 6 | 2.24 sigma .62" \
        +occupied.bands=4 \
        +element_B="2s* 2 2p 1 0 3d | 1.2 sigma .7" \
        +element_Al="3s* 2 3p* 1.1 0 3d | 1.8 sigma 1.1 Z= 14" \
        +potential_generator.use.bessel.projection=5 \
        +single_atom.local.potential.method=parabola \
        +single_atom.init.echo=7 \
        +single_atom.init.scf.maxit=1 \
        +single_atom.echo=7 \
        +logder.start=2 +logder.stop=1 \
        +bands.per.atom=4 \
        +potential_generator.max.scf=5 \
        +start.waves.scale.sigma=9 \
        +atomic.valence.decay=0 \
        > potential_generator.out

#         +electrostatic.solver=mg +electrostatic.potential.to.file=v_es.mg.dat \

#         +element_He="1s* 2 2p | 1.5 sigma .75" \
#         +electrostatic.solver=load +electrostatic.potential.from.file=v_es.fft.dat \
#         +electrostatic.solver=mg \
#         +element_Ne="2s* 2 2p* 6 3d | 1.8 sigma .7" \


### Results:
### Al-P dimer: Spectrum  -0.636   -0.504   -0.458   -0.219 | -0.216   -0.216   -0.182   -0.134 Ha with #cell 10.5835 10.5835 12.7003 p p p

### He simple cubic at Gamma point: -0.664    0.298    1.092    1.094    1.100    1.173    1.181    2.524    2.528    2.533
### B="2s* 2 2p* 1 0 3d | 1.2 sigma .7" +occupied.bands=1.5    -0.266    0.535    0.538    0.538    0.954    1.276    1.276    1.722    1.756    1.756
### B="2s* 2 2p 1 0 3d | 1.2 sigma .7"  +occupied.bands=1.5    -0.265    0.829    0.846    0.846    0.954    1.285    1.285    1.944    2.009    2.009
### Ar="3s 2 3p 6 | 1.6 sigma .8"                              -0.642   -0.140   -0.140   -0.140 (seems too high by .25 Ha)
### Xe="5s 2 5p 6 | 2.24 sigma .62"                            -0.555   -0.234   -0.234   -0.234

### dipole in Si chain experiment:
#
# printf " 4 \n#cell 4.2334 4.2334 8.4668 p p p\n" > atoms.xyz
# echo "Al   0 0 -1.05835" >> atoms.xyz
# echo "P    0 0  1.05835" >> atoms.xyz
# echo "Si   0 0  3.17505" >> atoms.xyz
# echo "Si   0 0 -3.17505" >> atoms.xyz
# 
# (cd ../src/ && make -j) && \
# $exe +verbosity=9 \
#     -test potential_generator. \
#         +eigensolver=none \
#         +potential_generator.grid.spacing=0.251 \
#         +electrostatic.solver=mg +electrostatic.potential.to.file=v_es.mg.dat \
#         +element_He="1s* 2 2p | 1.5 sigma .75" \
#         +element_Ne="2s* 2 2p* 6 3d | 1.8 sigma .7" \
#         +element_Al="3s* 2 3p* 1.1 0 3d | 1.8 sigma 1.1 Z= 14" \
#         +element_Si="3s* 2 3p* 2   0 3d | 1.8 sigma 1.1" \
#          +element_P="3s* 2 3p* 2.9 0 3d | 1.8 sigma 1.1 Z= 14" \
#         +potential_generator.use.bessel.projection=0 \
#         +single_atom.local.potential.method=sinc \
#         +single_atom.init.echo=7 \
#         +single_atom.init.scf.maxit=9 \
#         +single_atom.echo=7 \
#         +logder.start=2 +logder.stop=1 \
#         +bands.per.atom=4 \
#         +potential_generator.max.scf=13 \
#         +atomic.valence.decay=0 \
#         > potential_generator.out
