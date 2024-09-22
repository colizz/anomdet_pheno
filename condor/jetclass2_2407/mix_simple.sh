#!/bin/bash -x

cd /publicfs/cms/user/licq/pheno/anomdet/gen/condor/jetclass2_2407

MODE=$1
if [[ $MODE == "res2p_train" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res2p_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res2P", 0, {0, 0.8})'
elif [[ $MODE == "res34p_train" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res34p_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res34P", 0, {0, 0.8})'
elif [[ $MODE == "qcd_train" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_qcd_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "QCD", 0, {0, 0.8})'

elif [[ $MODE == "res2p_val" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res2p_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res2P", 1200, {0.8, 1})'
elif [[ $MODE == "res34p_val" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res34p_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res34P", 1860, {0.8, 1})'
elif [[ $MODE == "qcd_val" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_qcd_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "QCD", 1280, {0.8, 1})'

elif [[ $MODE == "res2p_test" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res2p_test.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res2P", 2250, {0, 1})'
elif [[ $MODE == "res34p_test" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res34p_test.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res34P", 3075, {0, 1})'
elif [[ $MODE == "qcd_test" ]]; then
    root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_qcd_test.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "QCD", 2350, {0, 1})'

fi
