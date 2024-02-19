#!/bin/bash -x

JSONFILE=$1 # samples/sm_all_ntuple_filelist.json
OUTPUT_PREFIX=$2 # sm/mixed_ntuple
MACHINE=$3
SELECTION_MODE=$4
if [[ -z $SELECTION_MODE ]]; then
    SELECTION_MODE="all"
fi

JOBNUM=0
DIR=$( cd "$( dirname "$0" )" && pwd )

# basic configuration
if [[ $MACHINE == "ihep" ]]; then
    OUTPUT_PATH=/publicfs/cms/user/$USER/condor_output
    LOAD_ENV_PATH=/scratchfs/cms/licq/utils/load_standalonemg_env.sh
    NTUPLIZER_FILE_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana/mixNtuples.C
fi


## load environment
if [ ! -z "${CONDA_PREFIX}" ]; then
    conda deactivate
fi
echo "Load env"
source $LOAD_ENV_PATH

RANDSTR=$(tr -dc A-Za-z0-9 </dev/urandom | head -c 10; echo)
WORKDIR=$OUTPUT_PATH/workdir_$(date +%y%m%d-%H%M%S)_${RANDSTR}_$JOBNUM
mkdir -p $WORKDIR

cd $WORKDIR


# run mixer
json_file_path=$DIR/$JSONFILE
input_file_prefix=$OUTPUT_PATH
output_file_dir=$OUTPUT_PATH/$OUTPUT_PREFIX
mkdir -p $output_file_dir

root -b -q $NTUPLIZER_FILE_PATH'+("'$json_file_path'", "'$input_file_prefix'", "'$output_file_dir'", "'$SELECTION_MODE'")'

# remove workspace
rm -rf $WORKDIR
