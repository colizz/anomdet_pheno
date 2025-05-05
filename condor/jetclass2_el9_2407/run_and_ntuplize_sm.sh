#!/bin/bash

## changes w.r.t. run.sh code
##  1. change delphes card to: delphes_card_CMS_JetClassII_lite.tcl
##    > onlyFatJet->lite: adding PUPPI AK4 jets and ele/muon/taus
##    > lite->full version: adding all CHS jets
##  2. use DelphesHepMC2WithFilter(2) instead of DelphesHepMC2 (add a $7 argu to specify which executable to use)
##  3. run subsequent ntuplizer script (add a $8 argu to specify which script to use)
##  4. allow custom selection level (add a $9 argu)
##  5. only show key output in the log file

# sleep $(shuf -i 0-600 -n 1) # random sleep to avoid too many jobs starting at the same time

PROC=$1
NEVENT=$2
NSUBDIVEVENT=$3
JOBNUM=$4
JOBBEGIN=$5
MACHINE=$6
DELPHES_CMD=$7
SCRIPT_NAME=$8
SELECTION_LEVEL=$9
if [[ -z $DELPHES_CMD ]]; then
    DELPHES_CMD=DelphesHepMC2
fi
if [[ -z $SCRIPT_NAME ]]; then
    SCRIPT_NAME=makeNtuplesHH4bResolvedOptionalSel.C
fi
if [[ -z $SELECTION_LEVEL ]]; then
    SELECTION_LEVEL=4j3bor2b
fi

JOBNUM=$((JOBNUM+JOBBEGIN))

ORIGDIR=$(pwd)

# setup workdir in /tmp
RANDSTR=$(tr -dc A-Za-z0-9 </dev/urandom | head -c 10; echo)
WORKDIR=/tmp/$USER/workdir_$(date +%y%m%d-%H%M%S)_${RANDSTR}_$(echo "$PROC" | sed 's/\//_/g')_$JOBNUM
mkdir -p $WORKDIR

cd $WORKDIR

# basic configuration
if [[ $MACHINE == "ihepel9" ]]; then
    OUTPUT_PATH=/publicfs/cms/user/$USER/condor_output
    GENPACKS_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/genpacks
    DELPHES_PATH=/publicfs/cms/user/licq/utils/Delphes-3.5.0
    LOAD_ENV_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/condor/load_custom_el9_env.sh
    NTUPLIZER_FILE_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/$SCRIPT_NAME # from JC2 offical macros
    MODEL_AK4_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_full_nonscale_ak4_alljetghostmatching.ddp2-bs512-lr1e-3/model.onnx
    MODEL_AK8_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_ak8puppi_full_scale/model_embed.onnx
elif [[ $MACHINE == "lxplusel9" ]]; then
    OUTPUT_PATH=root://eoscms.cern.ch//store/cmst3/group/vhcc/sfTuples/condor_output
    GENPACKS_PATH=/afs/cern.ch/user/c/coli/work/gen/genpacks
    DELPHES_PATH=/afs/cern.ch/user/c/coli/work/utils/pheno_utils/Delphes-3.5.0
    LOAD_ENV_PATH=/afs/cern.ch/user/c/coli/work/gen/load_lcg_el9_env.sh
    NTUPLIZER_FILE_PATH=/afs/cern.ch/user/c/coli/work/gen/delphes_ana_el9/$SCRIPT_NAME # from JC2 offical macros
    MODEL_AK4_PATH=/afs/cern.ch/user/c/coli/work/gen/delphes_ana_el9/model/JetClassII_full_nonscale_ak4_alljetghostmatching.ddp2-bs512-lr1e-3/model.onnx
    MODEL_AK8_PATH=/afs/cern.ch/user/c/coli/work/gen/delphes_ana_el9/model/JetClassII_ak8puppi_full_scale/model_embed.onnx
elif [[ $MACHINE == "remote" ]]; then
    OUTPUT_PATH=root://eoscms.cern.ch//store/cmst3/group/vhcc/sfTuples/condor_output
    GENPACKS_PATH=$WORKDIR/genpacks
    DELPHES_PATH=$WORKDIR/Delphes-3.5.0
    LOAD_ENV_PATH=$WORKDIR/load_remote_lcg_el9_env.sh
    NTUPLIZER_FILE_PATH=$WORKDIR/delphes_ana_el9/$SCRIPT_NAME # from JC2 offical macros
    MODEL_AK4_PATH=$WORKDIR/delphes_ana_el9/model/JetClassII_full_nonscale_ak4_alljetghostmatching.ddp2-bs512-lr1e-3/model.onnx
    MODEL_AK8_PATH=$WORKDIR/delphes_ana_el9/model/JetClassII_ak8puppi_full_scale/model_embed.onnx
fi
DELPHES_CARD=delphes_card_CMS_JetClassII_lite.tcl

## configurations for remote mode
if [[ $MACHINE == "remote" ]]; then
    xrdcp --silent -p -f root://eoscms.cern.ch//store/cmst3/group/vhcc/sfTuples/condor_output/input.tar.gz .
    xrdcp --silent -p -f root://eoscms.cern.ch//store/cmst3/group/vhcc/sfTuples/condor_output/gensw.tar.gz .
    tar xaf gensw.tar.gz
    tar xaf input.tar.gz
fi

## load environment
if [ ! -z "${CONDA_PREFIX}" ]; then
    conda deactivate
fi
echo "Load env"
source $LOAD_ENV_PATH > /dev/null 2>&1

# for remote mode, test delphes macro step first in case of failure
if [[ $MACHINE == "remote" ]]; then
    cd $(dirname $NTUPLIZER_FILE_PATH)
    root -b -l -q $SCRIPT_NAME'+("'$WORKDIR'/events_delphes_testexample.root", "out_testexample.root", "'$MODEL_AK4_PATH'", "'$MODEL_AK8_PATH'", "JetPUPPI", "JetPUPPIAK8", "'$SELECTION_LEVEL'")'
    [ ! -f out_testexample.root ] && exit 1
    cd -
fi

generate_delphes(){

    # Now should be inside the genpack
    # you should leave all GEN production logic inside the genpack's run.sh!
    # generate GEN events
    rm -f events.hepmc
    ./run.sh $NSUBDIVEVENT $MACHINE > tmp.log 2>&1
    echo "Generator-level output: " $(cat tmp.log | grep 'Cross-section')
    echo "HepMC event number: " $(grep -c '^E ' events.hepmc)

    # run delphes
    ln -sf $DELPHES_PATH/MinBias_100k.pileup .
    rm -f events_delphes.root
    $DELPHES_PATH/$DELPHES_CMD $DELPHES_PATH/cards/$DELPHES_CARD events_delphes.root events.hepmc > /dev/null 2>&1
    return $?
}

# generate delphes, in a batch of NSUBDIVEVENT
nbatch=$((NEVENT / NSUBDIVEVENT))

for ((i=0; i<nbatch; i++)); do

    echo "Batch: $i"

    # copy genpack if not exist
    cd $WORKDIR
    if [ ! -d "proc_base" ]; then
        mkdir proc_base
        cp -r $GENPACKS_PATH/$PROC/* proc_base/
        # if genpack does not have a run.sh, use the default
        if [ ! -f "proc_base/run.sh" ]; then
            cp $GENPACKS_PATH/run_genpack_default.sh proc_base/run.sh
        fi
    fi
    cd $WORKDIR/proc_base

    generate_delphes

    # if return code is 0
    if [ $? -eq 0 ]; then
        # successful
        mv events_delphes.root $WORKDIR/events_delphes_$i.root
        if [ -f events_lhe.lhe ]; then
            mv events_lhe.lhe $WORKDIR/events_lhe_$i.lhe
        fi
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
# mkdir -p $OUTPUT_PATH/$PROC

# if events_lhe.lhe exists
if [ -f $WORKDIR/events_lhe_0.lhe ]; then
    $THISDIR/scripts/mergeLHE.py -i $WORKDIR'/events_lhe_*.lhe' -o events_lhe.lhe
    mv events_lhe.lhe $OUTPUT_PATH/$PROC/events_lhe_$JOBNUM.lhe
fi

# ntuplize
cd $(dirname $NTUPLIZER_FILE_PATH)
root -b -l -q $SCRIPT_NAME'+("'$WORKDIR'/events_delphes.root", "'$WORKDIR'/out.root", "'$MODEL_AK4_PATH'", "'$MODEL_AK8_PATH'", "JetPUPPI", "JetPUPPIAK8", "'$SELECTION_LEVEL'")'
cd -

# check if job runs successfully
if [ ! -f out.root ]; then
    echo "The job failed, no out.root file"
    exit 1
fi

# transfer the file
mkdir -p $OUTPUT_PATH/${PROC}_ntuple
if [[ $OUTPUT_PATH == root://* ]]; then
    xrdcp --silent -p -f out.root $OUTPUT_PATH/${PROC}_ntuple/ntuples_$JOBNUM.root
else
    cp -f out.root $OUTPUT_PATH/${PROC}_ntuple/ntuples_$JOBNUM.root
fi

# remove workspace
rm -rf $WORKDIR

cd $ORIGDIR
touch dummy.cc
