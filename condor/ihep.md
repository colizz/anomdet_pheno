# General
// rsync between farm and ihep
rsync /data/pku/home/licq/utils/Delphes-3.5.0/{cards,readers,Makefile} ihep:~/scratchfs/utils/Delphes-3.5.0/
rsync /home/pku/licq/pheno/anomdet/gen/{genpacks,condor,delphes_ana} ihep:~/publicfs/pheno/anomdet/gen/ --exclude={log,output,*.so,*.d,*.pcm,*.root}

# submit jobs on ihep
hep_sub run.sh -argu tagger/train_higgs2p 100000 100 "%{ProcId}" 0 ihep -n 160
hep_sub run.sh -argu tagger/train_higgs4p 100000 100 "%{ProcId}" 0 ihep -n 1200 # failed, but 
hep_sub run.sh -argu tagger/train_higgs4p 100000 100 "%{ProcId}" 1000 ihep -n 200
hep_sub run.sh -argu tagger/train_higgspm2p 100000 100 "%{ProcId}" 0 ihep -n 120

hep_sub run.sh -argu tagger/train_qcd 100000 100 "%{ProcId}" 0 ihep -n 200

hep_sub run.sh -argu tagger/train_hbs 100000 100 "%{ProcId}" 0 ihep -n 20

hep_sub run.sh -argu tagger/train_higgspm2p 100000 100 "%{ProcId}" 0 ihep -n 37
hep_sub run0.sh -argu tagger/train_higgs4p 100000 100 "%{ProcId}" 1199 ihep -n 1

// add more training for bb and bs
hep_sub run.sh -argu tagger/train_hbs 100000 100 "%{ProcId}" 20 ihep -n 180
hep_sub run.sh -argu tagger/train_hbb 100000 100 "%{ProcId}" 0 ihep -n 200

hep_sub run.sh -argu tagger/train_hbb 100000 100 "%{ProcId}" 200 ihep -n 13
hep_sub run.sh -argu tagger/train_hbs 100000 100 "%{ProcId}" 200 ihep -n 11

# submit ntuplizer (note: REDO_DELPHES has been removed!)

hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_all_filelist.txt 1 ihep -n 600
hep_sub ntuplize.sh -argu "%{ProcId}" 600 samples/train_all_filelist.txt 1 ihep -n 1043

./ntuplize.sh 0 0 samples/train_hbs_filelist.txt 0 ihep ## test
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_hbs_filelist.txt 0 ihep -n 20

## re-submit ntuplizer!
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_all_filelist.txt 1 ihep -n 600
hep_sub ntuplize.sh -argu "%{ProcId}" 600 samples/train_all_filelist.txt 1 ihep -n 1043
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_hbs_filelist.txt 1 ihep -n 20
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_recover.txt 0 ihep -n 38
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_hbs_ext_filelist.txt ihep -n 373 #extra hbb and hbs to nfile=200

## merging file (first achieved 24.02.01)
source merge_submit.sh /publicfs/cms/user/licq/condor_output/train_hbs_ntuple 10
source merge_submit.sh /publicfs/cms/user/licq/condor_output/train_qcd 10

# submit SM events

## testing QCD

for i in 250 350 450 550; do rsync QCD/ QCD_$i; sed -i "s|PhaseSpace:pTHatMin = 100|PhaseSpace:pTHatMin = $i|g" QCD_$i/py8_templ.dat; done

hep_sub run.sh -argu QCD_200 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_250 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_300 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_350 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_400 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_450 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_500 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_550 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu QCD_600 10000 10000 "%{ProcId}" 0 ihep -n 1

## testing all MGs ( use run_test.sh!! )
./run.sh sm/WW 10000 10000 0 0 ihep

samlist=(WJetsToQQ WJetsToLNu GJets ZJetsToQQ ZJetsToLL ZJetsToNuNu TTbar SingleTop WW TW GG WG ZW ZG ZZ TZ TG SingleHiggs WH ZH TTbarH TTbarW TTbarZ TTbarG)
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm/${i} 10000 10000 "%{ProcId}" 0 ihep -n 1; done

// no TZ TG and TTbarX
samlist=(WJetsToQQ WJetsToLNu GJets ZJetsToQQ ZJetsToLL ZJetsToNuNu TTbar SingleTop WW TW GG WG ZW ZG ZZ SingleHiggs WH ZH)
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm_custom/${i}_0j 10000 10000 "%{ProcId}" 0 ihep -n 1; done


for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 550 600; do rsync sm/$i/ sm_custom/${i}_ht$ht; sed -i "s|set htjmin 0|set htjmin $ht|g" sm_custom/${i}_ht${ht}/mg5_step2_templ.dat; done; done

for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 550 600; do hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 0 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err; done; done

// following test using new PhaseII Delphes card (index 1)

i=WJetsToQQ; ht=600; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 1 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err
i=ZJetsToQQ; ht=600; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 1 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err
i=ZJetsToLL; ht=400; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 1 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err
i=TTbar; ht=400; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 1 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err
i=SingleHiggs; ht=350; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 1 ihep -o ${i}_ht$ht.out -e ${i}_ht$ht.err


## 24.01.30 new check.. specify decays in MG process..? (index 2)

TTbar SingleTop WW TW WG ZW ZG ZZ TZ TG SingleHiggs WH ZH TTbarH TTbarW TTbarZ TTbarG

samlist=(TTbar SingleTop WW TW WG ZW ZG ZZ TZ TG SingleHiggs WH ZH TTbarH TTbarW TTbarZ TTbarG)
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm/${i} 10000 10000 0 2 ihep delphes_card_CMS_JetClass_addak15.tcl -o logs_240130_splitdecay/${i}.out -e logs_240130_splitdecay/${i}.err; done

// 0j
samlist=(TTbar SingleTop WW TW WG ZW ZG ZZ SingleHiggs WH ZH)
for i in ${samlist[@]}; do echo $i; rsync sm/$i/mg* sm_custom/${i}_0j/ ; done

samlist=(TTbar SingleTop WW TW WG ZW ZG ZZ SingleHiggs WH ZH)
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm_custom/${i}_0j 10000 10000 0 2 ihep delphes_card_CMS_JetClass_addak15.tcl -o logs_240130_splitdecay/${i}_0j.out -e logs_240130_splitdecay/${i}_0j.err; done

## tuned PUPPI! back to original card (index 3)

i=WJetsToQQ; ht=600; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 3 ihep delphes_card_CMS_JetClassII_onlyFatJet.tcl -o logs_240130_tunepuppi/${i}_ht$ht.out -e logs_240130_tunepuppi/${i}_ht$ht.err
i=ZJetsToQQ; ht=600; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 3 ihep delphes_card_CMS_JetClassII_onlyFatJet.tcl -o logs_240130_tunepuppi/${i}_ht$ht.out -e logs_240130_tunepuppi/${i}_ht$ht.err
i=ZJetsToLL; ht=400; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 3 ihep delphes_card_CMS_JetClassII_onlyFatJet.tcl -o logs_240130_tunepuppi/${i}_ht$ht.out -e logs_240130_tunepuppi/${i}_ht$ht.err
i=TTbar; ht=400; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 3 ihep delphes_card_CMS_JetClassII_onlyFatJet.tcl -o logs_240130_tunepuppi/${i}_ht$ht.out -e logs_240130_tunepuppi/${i}_ht$ht.err
i=SingleHiggs; ht=350; hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 3 ihep delphes_card_CMS_JetClassII_onlyFatJet.tcl -o logs_240130_tunepuppi/${i}_ht$ht.out -e logs_240130_tunepuppi/${i}_ht$ht.err

## reconfigure WG and ZG
samlist=(WG ZG)
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm_custom/${i}_0j 10000 10000 0 2 ihep delphes_card_CMS_JetClass_addak15.tcl -o logs_240130_splitdecay/${i}_0j_new.out -e logs_240130_splitdecay/${i}_0j_new.err; done
for i in ${samlist[@]}; do echo $i; hep_sub run_test.sh -argu sm/${i} 10000 10000 0 2 ihep delphes_card_CMS_JetClass_addak15.tcl -o logs_240130_splitdecay/${i}_new.out -e logs_240130_splitdecay/${i}_new.err; done

## testing ttbar (index 10 )
samlist=(TTbar)
samlist=(WJetsToQQ WJetsToLNu ZJetsToQQ ZJetsToLL ZJetsToNuNu SingleTop WW TW TZ ZW ZZ SingleHiggs WH ZH)

for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 600 700 800; do rsync sm/$i/ sm_custom/${i}_ht$ht; sed -i "s|set htjmin 0|set htjmin $ht|g" sm_custom/${i}_ht${ht}/mg5_step2_templ.dat; done; done
for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 600 700 800; do hep_sub run_test.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 10 ihep -o logs_240131_htstudy/${i}_ht$ht.out -e logs_240131_htstudy/${i}_ht$ht.err; done; done

## (add 24.02.27) testing SingleHiggsToBB and DiHiggsTo4B

samlist=(SingleHiggsToBB DiHiggsTo4B)
for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 600 700 800; do rsync sm_ext/$i/ sm_custom/${i}_ht$ht; sed -i "s|set htjmin 0|set htjmin $ht|g" sm_custom/${i}_ht${ht}/mg5_step2_templ.dat; done; done
for i in ${samlist[@]}; do for ht in 250 300 350 400 450 500 600 700 800; do hep_sub run.sh -argu sm_custom/${i}_ht$ht 10000 10000 0 10 ihep -o logs_240131_htstudy/${i}_ht$ht.out -e logs_240131_htstudy/${i}_ht$ht.err; done; done

## testing QCD
for i in 200 250 300 350 400 450 500 550 600; do rsync sm/QCD/ sm_custom/QCD_pthat$i; sed -i "s|PhaseSpace:pTHatMin = 100|PhaseSpace:pTHatMin = $i|g" sm_custom/QCD_pthat$i/py8_templ.dat; done

samlist=(QCD)
for i in ${samlist[@]}; do for ht in 200 250 300 350 400 450 500 550 600; do hep_sub run_test.sh -argu sm_custom/${i}_pthat$ht 10000 10000 0 10 ihep -o logs_240131_htstudy/${i}_pthat$ht.out -e logs_240131_htstudy/${i}_pthat$ht.err; done; done

# 24.02.01 submit all SM events!

i=WJetsToQQ; N=54; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=WJetsToLNu; N=20; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZJetsToQQ; N=45; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZJetsToLL; N=6; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZJetsToNuNu; N=4; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TTbar; N=30; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=SingleTop; N=4; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=WW; N=4; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TW; N=4; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZW; N=3; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZZ; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TZ; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=SingleHiggs; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=WH; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=ZH; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TTbarH; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TTbarW; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=TTbarZ; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err

// for QCD: need to specify DelphesHepMC2WithFilter2!

i=QCD; N=1; start=0; hep_sub run_sm.sh -argu sm/$i 300000 10000 "%{ProcId}" $start ihep DelphesHepMC2WithFilter2 -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err

hep_sub run_sm.sh -argu sm/QCD 200000 10000 "%{ProcId}" 2000 ihep DelphesHepMC2WithFilter2 -n 2600 -o logs_240201_run_sm/QCD.%{ProcId}.out -e logs_240201_run_sm/QCD.%{ProcId}.err

// recover jobs
hep_sub run_sm_recover.sh -argu sm/QCD 200000 10000 "%{ProcId}" 2000 ihep DelphesHepMC2WithFilter2 -n 2600 -o logs_240201_run_sm/QCD.recover.%{ProcId}.out -e logs_240201_run_sm/QCD.recover.%{ProcId}.err
hep_sub run_sm_recover_licq.sh -argu sm/QCD 200000 10000 "%{ProcId}" 2000 ihep DelphesHepMC2WithFilter2 -n 17 -o logs_240201_run_sm/QCD.recover2.%{ProcId}.out -e logs_240201_run_sm/QCD.recover2.%{ProcId}.err

./run_sm_recover.sh sm/QCD 2000 100 0 8888 ihep DelphesHepMC2WithFilter2

## Xbb and Xbs
i=Xbb; N=1; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=Xbs; N=1; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=Xbb; N=4; start=1; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=Xbs; N=4; start=1; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=Xbb; N=25; start=5; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=Xbs; N=25; start=5; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err

// add SingleHiggsToBB and DiHiggsTo4B
i=SingleHiggsToBB; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=DiHiggsTo4B; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err

// add SingleHiggsToBBVarMass and DiHiggsTo4BVarMass (run only 20000 evts per file)
i=SingleHiggsToBBVarMass; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 20000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=DiHiggsTo4BVarMass; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 20000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=SingleHiggsToBBVarMass; N=200; start=50; hep_sub run_sm.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err
i=DiHiggsTo4BVarMass; N=150; start=50; hep_sub run_sm.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err
i=SingleHiggsToBBVarMass; N=200; start=50; hep_sub run_sm.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err
i=DiHiggsTo4BVarMass; N=150; start=50; hep_sub run_sm.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err
// recovery
i=SingleHiggsToBBVarMass; N=200; start=50; hep_sub run_sm_recover.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err
i=DiHiggsTo4BVarMass; N=150; start=50; hep_sub run_sm_recover.sh -argu sm_ext/$i 100000 200 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.ext.err

## WkkTo3W
i=WkkTo3WTo6Q; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
./run_sm.sh sm_ext/WkkTo3WTo6Q 1000 1000 0 8888 ihep

## 24.04.17 resubmit for Leyun's training
// store to sm_evtcls dir -> need to make a soft link for accessing genpacks
// For TTbar, nevt x2

i=WJetsToQQ; N=54; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
i=WJetsToLNu; N=20; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
i=ZJetsToQQ; N=45; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
i=ZJetsToLL; N=6; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
i=ZJetsToNuNu; N=4; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
i=TTbar; N=60; start=0; hep_sub run_sm.sh -argu sm_evtcls/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240417_run_sm/$i.%{ProcId}.out -e logs_240417_run_sm/$i.%{ProcId}.err
hep_sub run_sm.sh -argu sm_evtcls/QCD 200000 10000 "%{ProcId}" 2000 ihep DelphesHepMC2WithFilter2 -n 60 -o logs_240417_run_sm/QCD.%{ProcId}.out -e logs_240417_run_sm/QCD.%{ProcId}.err

# 24.02.05 ntuplize and mix SM events!

./ntuplize_sm.sh 6750 0 samples/sm_all_filelist.txt ihep
hep_sub ntuplize_sm.sh -argu "%{ProcId}" 0 samples/sm_all_filelist.txt ihep -n 6751 -o logs_240205_ntuplize_sm/%{ProcId}.out -e logs_240205_ntuplize_sm/%{ProcId}.err
hep_sub ntuplize_sm.sh -argu "%{ProcId}" 6751 samples/sm_all_filelist.txt ihep -n 2 -o logs_240205_ntuplize_sm/ext.%{ProcId}.out -e logs_240205_ntuplize_sm/ext.%{ProcId}.err
hep_sub ntuplize_sm.sh -argu "%{ProcId}" 6753 samples/sm_all_filelist.txt ihep -n 8 -o logs_240205_ntuplize_sm/ext.%{ProcId}.out -e logs_240205_ntuplize_sm/ext.%{ProcId}.err
hep_sub ntuplize_sm.sh -argu "%{ProcId}" 6761 samples/sm_all_filelist.txt ihep -n 50 -o logs_240205_ntuplize_sm/ext.%{ProcId}.out -e logs_240205_ntuplize_sm/ext.%{ProcId}.err

## mix
./mix_sm.sh samples/sm_all_ntuple_filelist.json sm/mixed_ntuple ihep
hep_sub mix_sm.sh -argu samples/sm_all_ntuple_filelist.json sm/mixed_ntuple ihep all -o logs_240205_ntuplize_sm/mix2.out -e logs_240205_ntuplize_sm/mix2.err
hep_sub mix_sm.sh -argu samples/sm_all_ntuple_filelist.json sm/mixed_msdgt130_ntuple ihep msdgt130 -o logs_240205_ntuplize_sm/mix.msdgt130.out -e logs_240205_ntuplize_sm/mix.msdgt130.err
hep_sub mix_sm.sh -argu samples/sm_all_ntuple_filelist.json sm/mixed_qcdlt0p1_ntuple ihep qcdlt0p1 -o logs_240205_ntuplize_sm/mix.qcdlt0p1.out -e logs_240205_ntuplize_sm/mix.qcdlt0p1.err

// use 10k Xbb and Xbs
hep_sub mix_sm.sh -argu samples/sm_all_sig10k_ntuple_filelist.json sm/mixed_qcdlt0p1_sig10k_ntuple ihep qcdlt0p1 -o logs_240205_ntuplize_sm/mix.qcdlt0p1_sig10k.out -e logs_240205_ntuplize_sm/mix.qcdlt0p1_sig10k.err

// pure 90k Xbb and Xbs
hep_sub mix_sm.sh -argu samples/sm_xbbbs_ntuple_filelist.json sm/mixed_xbbbs_ntuple ihep all -o logs_240205_ntuplize_sm/mix.xbbbs.out -e logs_240205_ntuplize_sm/mix.xbbbs.err
hep_sub mix_sm.sh -argu samples/sm_xbbbs_ntuple_filelist.json sm/mixed_xbbbs_qcdlt0p1_ntuple ihep qcdlt0p1 -o logs_240205_ntuplize_sm/mix.xbbbs_qcdlt0p1.out -e logs_240205_ntuplize_sm/mix.xbbbs_qcdlt0p1.err


## 24.02.23 ntuplize and mix again (with new model)
for i in SingleHiggs WH ZH SingleTop WJetsToQQ ZZ ZJetsToQQ TZ ZJetsToLL ZW TTbarH TTbarZ WW TW WJetsToLNu TTbar ZJetsToNuNu TTbarW QCD QCD_LG; do ln -s ../sm/$i .; done

hep_sub ntuplize_sm.sh -argu "%{ProcId}" 0 samples/sm_2_all_filelist.txt ihep -n 6753 -o logs_240223_ntuplize_sm/%{ProcId}.out -e logs_240223_ntuplize_sm/%{ProcId}.err
hep_sub mix_sm.sh -argu samples/sm_2_all_ntuple_filelist.json sm_2/mixed_qcdlt0p1_ntuple ihep qcdlt0p1 -o logs_240223_ntuplize_sm/mix.qcdlt0p1.out -e logs_240223_ntuplize_sm/mix.qcdlt0p1.err

## 24.04.21 sm_dijet
for i in SingleHiggs WH ZH SingleTop WJetsToQQ ZZ ZJetsToQQ TZ ZJetsToLL ZW TTbarH TTbarZ WW TW WJetsToLNu TTbar ZJetsToNuNu TTbarW QCD QCD_LG; do ln -s ../sm/$i .; done

hep_sub ntuplize_sm.sh -argu "%{ProcId}" 0 samples/sm_dijet_all_filelist.txt ihep makeDiJetRepNtuples.C -n 6801 -o logs_240421_ntuplize_sm/%{ProcId}.out -e logs_240421_ntuplize_sm/%{ProcId}.err
hep_sub mix_sm.sh -argu samples/sm_dijet_all_ntuple_filelist.json sm_dijet/mixed_passsel_ntuple ihep pass_selection -o logs_240421_ntuplize_sm/mix.passsel.out -e logs_240421_ntuplize_sm/mix.passsel.err
