#!/bin/bash -x

JOBNUM=$1
JOBBEGIN=$2
FILELIST=$3 #samples/train_all_filelist.txt
MACHINE=$4
SCRIPT_NAME=$5
if [[ -z $SCRIPT_NAME ]]; then
    SCRIPT_NAME=makeNtuples.C
fi

JOBNUM=$((JOBNUM+JOBBEGIN))


# using the jobnum to read the line
DIR="$( cd "$( dirname "$0" )" && pwd )"
FILEIN=$(sed -n "$((JOBNUM+1))p" "$DIR/$FILELIST")
if [[ "$FILEIN" == "" ]]; then
    echo "No more files to process"
    exit 1
fi

# basic configuration
if [[ $MACHINE == "ihep" ]]; then
    OUTPUT_PATH=/publicfs/cms/user/$USER/condor_output
    DELPHES_PATH=/scratchfs/cms/licq/utils/Delphes-3.5.0
    LOAD_ENV_PATH=/scratchfs/cms/licq/utils/load_standalonemg_env.sh
    NTUPLIZER_FILE_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana/$SCRIPT_NAME
fi
DELPHES_CARD=delphes_card_CMS_JetClassII_onlyFatJet.tcl


## load environment
if [ ! -z "${CONDA_PREFIX}" ]; then
    conda deactivate
fi
echo "Load env"
source $LOAD_ENV_PATH

RANDSTR=$(tr -dc A-Za-z0-9 </dev/urandom | head -c 10; echo)
WORKDIR=$OUTPUT_PATH/workdir_$(date +%y%m%d-%H%M%S)_${RANDSTR}_$(echo "$PROC" | sed 's/\//_/g')_$JOBNUM
mkdir -p $WORKDIR

cd $WORKDIR

# process the paths
filein_path=$OUTPUT_PATH/$FILEIN

## extract the dir name
dir_path=$(dirname "$filein_path")
file_name=$(basename "$filein_path")

## the new delphes path and the ntuple path to be stored
## note: if the dir path ends with "_0", if means we should rerun delphes step
if [[ $dir_path == *"_0" ]]; then
    REDO_DELPHES=1
else
    REDO_DELPHES=0
fi

dir_path_new=${dir_path%_0}
dir_path_ntuple=${dir_path%_0}_ntuple

mkdir -p $dir_path_new
mkdir -p $dir_path_ntuple

# redo delphes
if [[ "$REDO_DELPHES" == "1" ]]; then
    ln -s $DELPHES_PATH/MinBias_100k.pileup .
    rm -f $dir_path_new/$file_name
    $DELPHES_PATH/DelphesROOT $DELPHES_PATH/cards/$DELPHES_CARD $dir_path_new/$file_name $filein_path || exit $?
fi

# run ntuplizer
if [[ "$FILEIN" == *"train_qcd"* ]]; then
    assign_qcd=1
else
    assign_qcd=0
fi
root -b -q $NTUPLIZER_FILE_PATH'+("'$dir_path_new/$file_name'", "'$dir_path_ntuple/${file_name/events_delphes/ntuples}'", "JetPUPPIAK8", '$assign_qcd')'

# remove workspace
rm -rf $WORKDIR
