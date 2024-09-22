#!/bin/bash -x

# a wrapper to generate hepmc file
# a customized version to run LHE events from tarballs

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
MDIR=proc

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

# step1: setup the dir for the first time
if [ ! -d $MDIR ]; then
    echo "Setup gridpack dir"
    mkdir -p $MDIR
    
    # untar tarball
    if [ `ls {*.tar.xz,*.tgz,*.tar.gz} | wc -l` -ne 1 ]; then
        echo "Error: more than one tarball in the dir"
        exit 1
    fi
    TARBALL=`ls {*.tar.xz,*.tgz,*.tar.gz}`
    tar -xaf $TARBALL -C $MDIR
fi

# step2: generate event
cd $MDIR

## launch
RAND=$(shuf -i 1-1000000 -n 1)
./run.sh $NEVENT $RAND

cd -
mv $MDIR/events.lhe.gz unweighted_events.lhe.gz

# run pythia
rm -f events.hepmc
LD_LIBRARY_PATH=$MG5_PATH/HEPTools/lib:$LD_LIBRARY_PATH $MG5_PATH/HEPTools/MG5aMC_PY8_interface/MG5aMC_PY8_interface py8.dat
