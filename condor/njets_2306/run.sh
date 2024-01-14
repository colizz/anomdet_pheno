#!/bin/bash -x

MDIR=$1
NEVENT=$2
JOBNUM=$3

## load environment
source ~/.bashrc
conda deactivate
source ~/utils/load_standalonemg_env.sh

WORKDIR=/home/pku/licq/pheno/anomdet/gen/$MDIR/condor_output/$JOBNUM
mkdir -p $WORKDIR
cd $WORKDIR

# generate MG events
tar xaf /home/pku/licq/pheno/anomdet/gen/$MDIR/gridpack_gridpack.tar.gz
./run.sh $NEVENT $JOBNUM
gzip -d events.lhe.gz

# run pythia and delphes
cd /home/pku/licq/pheno/anomdet/gen/
python3 mg5helper/launch.py -f -i $WORKDIR/events.lhe \
 --pcard cards/${MDIR}_PY8Card.dat \
 --dcard /data/pku/home/licq/utils/Delphes-3.5.0/cards/delphes_card_CMS_JetClass_mod.tcl \
 -n 1 -t ${JOBNUM}

# clean up
mv $WORKDIR/delphes_$JOBNUM.root $WORKDIR/..
rm -rf $WORKDIR
