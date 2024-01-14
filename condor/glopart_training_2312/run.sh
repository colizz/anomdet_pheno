#!/bin/bash -x

PROC=$1
NEVENT=$2
NSUBDIVEVENT=$3
JOBNUM=$4
JOBBEGIN=$5
MACHINE=$6

JOBNUM=$((JOBNUM+JOBBEGIN))

# basic configuration
if [[ $MACHINE == "farm" ]]; then
    OUTPUT_PATH=/data/bond/licq/delphes/glopart_training
    GENPACKS_PATH=/home/pku/licq/pheno/anomdet/gen/genpacks
    DELPHES_PATH=/data/pku/home/licq/utils/Delphes-3.5.0
    LOAD_ENV_PATH=/home/pku/licq/utils/load_standalonemg_env.sh
elif [[ $MACHINE == "ihep" ]]; then
    OUTPUT_PATH=/publicfs/cms/user/licq/condor_output
    GENPACKS_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/genpacks
    DELPHES_PATH=/scratchfs/cms/licq/utils/Delphes-3.5.0
    LOAD_ENV_PATH=/scratchfs/cms/licq/utils/load_standalonemg_env.sh
fi
DELPHES_CARD=delphes_card_CMS_JetClass_addak15.tcl

## load environment
if [ ! -z "${CONDA_PREFIX}" ]; then
    conda deactivate
fi
echo "Load env"
source $LOAD_ENV_PATH

WORKDIR=$OUTPUT_PATH/workdir_$(date +%y%m%d-%H%M%S)_$JOBNUM
mkdir -p $WORKDIR

cd $WORKDIR

generate_delphes(){

    # Now should be inside the genpack
    # you should leave all GEN production logic inside the genpack's run.sh!
    # generate GEN events
    rm -f events.hepmc
    ./run.sh $NSUBDIVEVENT $MACHINE

    # run delphes
    $DELPHES_PATH/DelphesHepMC2 $DELPHES_PATH/cards/$DELPHES_CARD events_delphes.root events.hepmc
    return $?
}

# generate delphes, in a batch of NSUBDIVEVENT
nbatch=$((NEVENT / NSUBDIVEVENT))

for ((i=0; i<nbatch; i++)); do

    echo "Batch: $i"

    # copy genpack if not exist
    cd $WORKDIR
    if [ ! -d $PROC ]; then
        cp -r $GENPACKS_PATH/$PROC .
        # if genpack does not have a run.sh, use the default
        if [ ! -f $PROC/run.sh ]; then
            cp $GENPACKS_PATH/run_genpack_default.sh $PROC/run.sh
        fi
    fi
    cd $WORKDIR/$PROC

    generate_delphes

    # if return code is 0
    if [ $? -eq 0 ]; then
        # successful
        mv events_delphes.root $WORKDIR/events_delphes_$i.root
    fi
    cd $WORKDIR

    # intermediate file merging for every 100 batches
    if [ $(((i+1) % 100)) -eq 0 ]; then
        hadd -f $WORKDIR/merged_events_delphes_$i.root $WORKDIR/events_delphes_*.root
        if [ $? -eq 0 ]; then
            rm -f $WORKDIR/events_delphes_*.root
        fi
    fi

done

# combine all root
if [ $nbatch -eq 1 ]; then
    mv $WORKDIR/events_delphes_0.root events_delphes.root
else
    hadd -f events_delphes.root $WORKDIR/*.root
fi
mkdir -p $OUTPUT_PATH/$PROC

# transfer the file
mv events_delphes.root $OUTPUT_PATH/$PROC/events_delphes_$JOBNUM.root
