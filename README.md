Code from the DUNE beam simulation group repurposed to be used for the 2x2 neutrino flux simulation to allow histogram portability. Its energy range and z-position altered. The position of the detector relative to the beam is found here: https://github.com/DUNE/2x2_sim/blob/main/run-edep-sim/flux/GNuMIFlux.xml. The max energy was changed to 40 GeV as that is where the NuMI ME beam converges to 0. The exact official code from the DUNE Beam Sim group is here: https://cdcvs.fnal.gov/redmine/projects/lbne-beamsim/repository/revisions/2dfb90abe429f0f5a8622113ffe0dae7a4bd5145

Instructions for Fermilab machines:

`source setup_dk2nuplotter.sh
`

Grab the dk2nu files from NERSC (https://portal.nersc.gov/project/dune/data/misc/NuMI_dk2nu/) put them somewhere safe
Run the compiler using the instructions at the top of beamHist2x2.cc

`./beamHist -i "/path/to/files/*.dk2nu" -o testFHC.root -r 67 -t 4
`

Clean up the plots into nice tidy names:

`root -l -b 'copyHists.C("FHC")'
`
