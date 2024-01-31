#!/bin/bash -x

# a wrapper to generate hepmc file

# this is a customized version for qcd event generator, using pythia8 only

NEVENT=$1
MACHINE=$2
if [ -z $MACHINE ]; then
    MACHINE=farm
fi

# basic configuration
if [[ $MACHINE == "farm" ]]; then
    MG5_PATH=/data/pku/home/licq/utils/MG5_aMC_v2_9_18
    LOAD_ENV_PATH=/home/pku/licq/utils/load_standalonemg_env.sh
elif [[ $MACHINE == "ihep" ]]; then
    MG5_PATH=/scratchfs/cms/licq/utils/MG5_aMC_v2_9_18
    LOAD_ENV_PATH=/scratchfs/cms/licq/utils/load_standalonemg_env.sh
fi

# the MG process dir
# MDIR=proc

## load environment
if [ -z "$PYTHIA8DATA" ]; then
    if [ ! -z "${CONDA_PREFIX}" ]; then
        conda deactivate
    fi
    echo "Load env"
    source $LOAD_ENV_PATH
fi

# cd into current genpack's dir
cd "$( dirname "${BASH_SOURCE[0]}" )"

# read the custom py8 parameters
# sample one line from the paramter file if py8_params.dat exists
if [ -f py8_params.dat ]; then
    RANDOM_PARAMS=$(shuf -n 1 py8_params.dat)
    IFS=' ' read -ra PARAMS <<< $RANDOM_PARAMS
fi

# step1: compile py8 program
if [ ! -f py8_main ]; then
    echo "Comple py8 program"
    g++ py8_main.cc -o py8_main -I$MG5_PATH/HEPTools/pythia8//include -ldl -fPIC -lstdc++ -std=c++11 -O2 -DHEPMC2HACK -DGZIP -I$MG5_PATH/HEPTools/zlib/include -L$MG5_PATH/HEPTools/zlib/lib -Wl,-rpath,$MG5_PATH/HEPTools/zlib/lib -lz -L$MG5_PATH/HEPTools/pythia8//lib -Wl,-rpath,$MG5_PATH/HEPTools/pythia8//lib -lpythia8 -ldl -I$MG5_PATH/HEPTools/hepmc/include -L$MG5_PATH/HEPTools/hepmc/lib -Wl,-rpath,$MG5_PATH/HEPTools/hepmc/lib -lHepMC -DHEPMC2
fi

# step2: generate event

## write py8.dat
cp -f py8_templ.dat py8.dat
## if py8_params.dat exists, replace $1, $2, ... by the customized params
if [ -f py8_params.dat ]; then
    for i in `seq 1 ${#PARAMS[@]}`; do
        sed -i "s/\$${i}/${PARAMS[$((i-1))]}/g" py8.dat
    done
fi

sed -i "s/\$NEVENT/$NEVENT/g" py8.dat # initialize nevent
sed -i "s/\$SEED/$RANDOM/g" py8.dat # initialize seed, this is not working for pythia as we use pythia's own seed

## generate hepmc events (events.hepmc)
./py8_main
