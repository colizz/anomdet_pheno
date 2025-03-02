#!/bin/bash -x

MACHINE=$1

# basic configuration
if [[ $MACHINE == "ihep" ]]; then
    OUTPUT_PATH=/publicfs/cms/user/$USER/condor_output
    GENPACKS_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/genpacks
    DELPHES_PATH=/scratchfs/cms/licq/utils/Delphes-3.5.0
    LOAD_ENV_PATH=/scratchfs/cms/licq/utils/load_standalonemg_env.sh
    NTUPLIZER_FILE_PATH=/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana/mixNtuples_JetClassII.C
fi

source $LOAD_ENV_PATH

DIR=$( cd "$( dirname "$0" )" && pwd )
for sam in Res2P QCD; do
    mkdir -p $OUTPUT_PATH/JetClassII/$sam   
    root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 0, "train", "all")'
done
# for sam in Res34P; do
#     root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'_part1.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 0, "all")'
#     root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'_part2.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 200, "all")'
#     root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'_part3.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 400, "all")'
#     root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'_part4.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 600, "all")'
#     root -b -q $NTUPLIZER_FILE_PATH'++("'$DIR'/samples/reorg_JetClassII_'$sam'_part5.json", "'$OUTPUT_PATH'/tagger", "'$OUTPUT_PATH'/JetClassII/'$sam'", "'$sam'", 800, "all")'
# done
