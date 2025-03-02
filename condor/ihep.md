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

// more, when finalizing
hep_sub run.sh -argu tagger/train_higgs4p 100000 100 "%{ProcId}" 1200 ihep -n 240
hep_sub run.sh -argu tagger/train_higgs2p 100000 100 "%{ProcId}" 160 ihep -n 32
hep_sub run.sh -argu tagger/train_higgspm2p 100000 100 "%{ProcId}" 120 ihep -n 24
hep_sub run.sh -argu tagger/train_qcd 100000 100 "%{ProcId}" 200 ihep -n 40

hep_sub run_recover.sh -argu tagger/train_higgs4p 100000 100 "%{ProcId}" 1200 ihep -n 240
hep_sub run_recover.sh -argu tagger/train_higgs2p 100000 100 "%{ProcId}" 160 ihep -n 32
hep_sub run_recover.sh -argu tagger/train_higgspm2p 100000 100 "%{ProcId}" 120 ihep -n 24

./run.sh tagger/train_higgs2p 10 10 0 0 farm

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

// 24.07 add SingleHiggsToCC
i=SingleHiggsToCC; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err


## WkkTo3W
i=WkkTo3WTo6Q; N=50; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
./run_sm.sh sm_ext/WkkTo3WTo6Q 1000 1000 0 8888 ihep

## hh datasets
./run_sm.sh hh/qqHH_CV_1_C2V_2_kl_1 10000 10000 0 8888 farm


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
hep_sub mix_sm.sh -argu samples/sm_dijet_wkk_ntuple_filelist.json sm_dijet/mixed_wkk_passsel_ntuple ihep pass_selection -o logs_240421_ntuplize_sm/mix.wkk_passsel.out -e logs_240421_ntuplize_sm/mix.wkk_passsel.err

## 24.06.13 process JetClassII
root -b -q 'mixNtuples_JetClassII.C++("/publicfs/cms/user/licq/pheno/anomdet/gen/condor/glopart_training_2312/samples/reorg_JetClassII_2p.json", "/publicfs/cms/user/licq/condor_output/tagger", "/publicfs/cms/user/licq/condor_output/JetClassII/Res2P", "Res2P", "all")'
hep_sub reorg_JetClass.sh -argu ihep

// farm
root -b -q 'mixNtuples_JetClassII.C++("/home/pku/licq/pheno/anomdet/gen/condor/glopart_training_2312/samples/reorg_JetClassII_2p.json", "/data/bond/licq/datasets/JetClassII", "/data/bond/licq/datasets/JetClassII/Res2P", "Res2P", "all")'

## 24.07 for Yuzhe to test H->bb/cc vs QCD

root -b -q 'makeJetRepNtuplesForHqq.C++("/data/bond/licq/delphes/glopart_training/sm_ext/SingleHiggsToBB/events_delphes_0.root", "out.root", "/home/pku/licq/pheno/anomdet/gen/delphes_ana/model/JetClassII_ak8puppi_full_scale/model_embed.onnx")'

hep_sub ntuplize_sm.sh -argu "%{ProcId}" 0 samples/sm_hqq_yuzhe_filelist.txt ihep makeJetRepNtuplesForHqq.C -n 50
hep_sub ntuplize_sm.sh -argu "%{ProcId}" 50 samples/sm_hqq_yuzhe_filelist.txt ihep makeJetRepNtuplesForHqq.C -n 50

// added 25.02: new 2HDM bb and cc samples, as well as bc


## 25.01 process HH di-jet Rep samples



======================================================
# testing HH
samlist=(qqHH_CV_1_C2V_1_kl_1)
for i in ${samlist[@]}; do for mHH in 0 300 400 500 600 700 800 900 1000; do rsync hh/$i/ hh_custom/${i}_mHH$mHH; sed -i "s|set mxx_min_pdg {25:0}|set mxx_min_pdg {25:$mHH}|g" hh_custom/${i}_mHH$mHH/mg5_step2_templ.dat; done; done

for i in ${samlist[@]}; do for mHH in 0 300 400 500 600 700 800 900 1000; do hep_sub run_sm.sh -argu hh_custom/${i}_mHH$mHH 10000 10000 0 8888 ihep -o logs_240717_hh/${i}_mHH$mHH.out -e logs_240717_hh/${i}_mHH$mHH.err; done; done

hep_sub run_sm.sh -argu hh_old/ggHH_kl_0_kt_1 2000 2000 0 8888 ihep DelphesHepMC2
hep_sub run_sm.sh -argu hh_old/ggHH_kl_0_kt_1 10000 10000 0 8889 ihep

./run_sm.sh hh_old/ggHH_kl_0_kt_1 10000 10000 0 8889 ihep
./run_sm.sh hh_old/qqHH_CV_0p5_C2V_1_kl_1 1000 1000 0 8888 farm
./run_sm.sh hh/ggHH_kl_1_kt_1_el9 100 100 0 8888 ihepel9

samlist=(qqHH_CV_1_C2V_1_kl_1)

for i in ${samlist[@]}; do for pt in 0 200 250 300 350 400 450 500 550 600; do rsync hh/$i/ hh_custom/${i}_pt$pt; sed -i "s|set pt_min_pdg {25:0}|set pt_min_pdg {25:$pt}|g" hh_custom/${i}_pt$pt/mg5_step2_templ.dat; done; done

for i in ${samlist[@]}; do for pt in 0 200 250 300 350 400 450 500 550 600; do hep_sub run_sm.sh -argu hh_custom/${i}_pt$pt 10000 10000 0 8888 ihep -o logs_240717_hh/${i}_pt$pt.out -e logs_240717_hh/${i}_pt$pt.err; done; done

## submit
i=qqHH_CV_1_C2V_1_kl_1; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err
i=qqHH_CV_1_C2V_2_kl_1; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err
i=qqHH_CV_1_C2V_1_kl_2; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err
i=qqHH_CV_1_C2V_1_kl_0; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err
i=qqHH_CV_0p5_C2V_1_kl_1; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err
i=qqHH_CV_1p5_C2V_1_kl_1; N=30; start=0; hep_sub run_sm.sh -argu hh/$i 100000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh/$i.%{ProcId}.out -e logs_240717_hh/$i.%{ProcId}.err

i=ggHH_kl_1_kt_1; N=1000; start=0; hep_sub run_sm.sh -argu hh/$i 10000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh_tmp/$i.%{ProcId}.out -e logs_240717_hh_tmp/$i.%{ProcId}.err
i=ggHH_kl_1_kt_1; N=1000; start=0; hep_sub run_sm_recover.sh -argu hh/$i 10000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh_tmp/$i.recover.%{ProcId}.out -e logs_240717_hh_tmp/$i.recover.%{ProcId}.err

i=ggHH_kl_5_kt_1; N=500; start=0; hep_sub run_sm.sh -argu hh/$i 10000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh_tmp/$i.%{ProcId}.out -e logs_240717_hh_tmp/$i.%{ProcId}.err
i=ggHH_kl_1_kt_1_tmp; N=11; start=1000; hep_sub run_sm.sh -argu hh/$i 10000 10000 "%{ProcId}" $start ihep -n $N -o logs_240717_hh_tmp/$i.%{ProcId}.out -e logs_240717_hh_tmp/$i.%{ProcId}.err

# convert lhe (not used)

341 345 349 339

./convert.sh 0 0 ggHH_kl_0_kt_1 ihep

hep_sub convert.sh -argu 0 "%{ProcId}" ggHH_kl_0_kt_1 ihep -n 341
hep_sub convert.sh -argu 0 "%{ProcId}" ggHH_kl_1_kt_1 ihep -n 345
hep_sub convert.sh -argu 0 "%{ProcId}" ggHH_kl_2p45_kt_1 ihep -n 349
hep_sub convert.sh -argu 0 "%{ProcId}" ggHH_kl_5_kt_1 ihep -n 339

hep_sub convert.sh -argu 1 "%{ProcId}" ggHH_kl_0_kt_1 ihep -n 1

# 2307 finalize JetClassII
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_all_2407.txt ihep -n 2
hep_sub ntuplize.sh -argu "%{ProcId}" 2 samples/train_all_2407.txt ihep -n 998
hep_sub ntuplize.sh -argu "%{ProcId}" 1000 samples/train_all_2407.txt ihep -n 1015

/// test command
root -b -q 'makeNtuples.C++("/home/pku/licq/pheno/anomdet/events_delphes_qcd_0.root", "test_qcd.root", "JetPUPPIAK8", "GenJetAK8", true, true)'
root -b -q 'mixNtuples.C++("/home/pku/licq/pheno/anomdet/gen/condor/jetclass2_2407/samples/mix_test.json", "/home/pku/licq/pheno/anomdet/jetclass2_generation/delphes_ana", "/home/pku/licq/pheno/anomdet/jetclass2_generation/delphes_ana", "mix", 0, {0, 0.8})'

// example mix for official notebook
root -b -q '/home/pku/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("/home/pku/licq/pheno/anomdet/gen/condor/jetclass2_2407/samples/mix_example.json", "/home/pku/licq/pheno/anomdet/jetclass2_generation/delphes_ana/data", "/home/pku/licq/pheno/anomdet/jetclass2_generation/delphes_ana/data", "Mix", 0, {0, 1})'

## formal runs
root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_res2p_trainval.json", "/publicfs/cms/user/licq/condor_output", "/publicfs/cms/user/licq/condor_output/tagger_jetclass2/final", "Res2P", 0, {0, 0.8})'

hep_sub mix_simple.sh -argu res34p_train
hep_sub mix_simple.sh -argu qcd_train

hep_sub mix_simple.sh -argu res2p_val
hep_sub mix_simple.sh -argu res34p_val
hep_sub mix_simple.sh -argu qcd_val

hep_sub mix_simple.sh -argu res2p_test
hep_sub mix_simple.sh -argu res34p_test
hep_sub mix_simple.sh -argu qcd_test

# origanize
sam=Res34P
for i in `seq 0 859`; do ln -s ../final/${sam}_$(printf "%04d" $i).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 860 1074`; do ln -s ../final/${sam}_$(printf "%04d" $((i+1000))).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 1075 1289`; do ln -s ../final/${sam}_$(printf "%04d" $((i+2000))).root ${sam}_$(printf "%04d" $i).root; done
sam=Res2P
for i in `seq 0 199`; do ln -s ../final/${sam}_$(printf "%04d" $i).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 200 249`; do ln -s ../final/${sam}_$(printf "%04d" $((i+1000))).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 250 299`; do ln -s ../final/${sam}_$(printf "%04d" $((i+2000))).root ${sam}_$(printf "%04d" $i).root; done
sam=QCD
for i in `seq 0 279`; do ln -s ../final/${sam}_$(printf "%04d" $i).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 280 349`; do ln -s ../final/${sam}_$(printf "%04d" $((i+1000))).root ${sam}_$(printf "%04d" $i).root; done
for i in `seq 350 419`; do ln -s ../final/${sam}_$(printf "%04d" $((i+2000))).root ${sam}_$(printf "%04d" $i).root; done

touch *

sam=QCD; mode=train; i=0; echo tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i+0)))_$(printf "%04d" $((i+9))).tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i+j))).root "; done)

sam=Res34P; mode=train; for i in `seq 0 85`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res34P; mode=val; for i in `seq 86 106`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res34P; mode=val; for i in `seq 107 107`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+4)))_500k.tar $(for j in {0..4}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res34P; mode=test; for i in `seq 107 127`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+5)))_$(printf "%04d" $((i*10+14)))_1M.tar $(for j in {5..14}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res34P; mode=test; for i in `seq 128 128`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+5)))_$(printf "%04d" $((i*10+9)))_500k.tar $(for j in {5..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done

sam=Res2P; mode=train; for i in `seq 0 19`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res2P; mode=val; for i in `seq 20 24`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=Res2P; mode=test; for i in `seq 25 29`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done

sam=QCD; mode=train; for i in `seq 0 27`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=QCD; mode=val; for i in `seq 28 34`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done
sam=QCD; mode=test; for i in `seq 35 41`; do tar -chvf ../tarball/JetClassII_Pythia_${mode}_${sam}_$(printf "%04d" $((i*10+0)))_$(printf "%04d" $((i*10+9)))_1M.tar $(for j in {0..9}; do echo -n "${sam}_$(printf "%04d" $((i*10+j))).root "; done); done

========================================================================================
# test photon

./run_sm.sh sm_ext/photon 100 100 0 8888 farm


========================================================================================
# mix CMS GloParT QCD samples


root -b -q '/publicfs/cms/user/licq/pheno/anomdet/jetclass2_generation/delphes_ana/mixNtuples.C+("samples/mix_qcd_cms.json", "/publicfs/cms/user/licq/samples/deepjetak8/20230504_ak8_UL17_v8", "/publicfs/cms/user/licq/samples/deepjetak8/20230504_ak8_UL17_v8/QCD_Pt_170toInf_ptBinned_TuneCP5_13TeV_pythia8_remix", "dnnTuples_remix", 0, {0, 1})'

========================================================================================
# Sophon AK4
## generation
// changed to ihep el9 env => in run_ak4.sh env setup
// additional:
//  - change WORKDIR to /tmp
//  - as we use new pythia8 version (8311), use Main:numberOfEvents = 0

./run_ak4.sh taggerak4/train_higgs2p 100 100 0 8888 ihepel9
hep_sub run_ak4.sh -argu taggerak4/train_higgs2p 100 100 "%{ProcId}" 0 ihepel9 -n 1
hep_sub run_ak4.sh -argu taggerak4/train_higgs2p 100000 100 "%{ProcId}" 0 ihepel9 -n 200

## ntuplizer
./ntuplize_ak4.sh 0 0 samples_ak4/train_all_filelist.txt ihepel9

hep_sub ntuplize_ak4.sh -argu "%{ProcId}" 0 samples_ak4/train_all_filelist.txt ihepel9 -n 200 -o logs_2501_ntuplize_ak4/ext.%{ProcId}.out -e logs_2501_ntuplize_ak4/ext.%{ProcId}.err

## generate ttbarFH test samples
// shouldn't add a Delphes filter. keep all events
./run_ak4.sh taggerak4/TTbarSL 100 100 0 8888 ihepel9

i=TTbarSL; hep_sub run_ak4.sh -argu taggerak4/$i 100000 10000 "%{ProcId}" 0 ihepel9 -n 20 -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

## ntuplizer for SophonAK4 inference
root -b -q 'makeNtuplesEvalSophonAK4.C+("/publicfs/cms/user/licq/condor_output/taggerak4/train_higgs2p/events_delphes_0.root", "test_ak4.root", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_full_nonscale_ak4.ddp2-bs512-lr1e-3/model.onnx", "JetPUPPI", false)'

./ntuplize_evalsophonak4.sh 0 0 samples_ak4/train_ttbar_filelist.txt ihepel9 makeNtuplesEvalSophonAK4.C

hep_sub ntuplize_evalsophonak4.sh -argu "%{ProcId}" 0 samples_ak4/train_ttbar_filelist.txt ihepel9 makeNtuplesEvalSophonAK4.C -n 20 -o logs_2501_ntuplize_ak4/ext.%{ProcId}.out -e logs_2501_ntuplize_ak4/ext.%{ProcId}.err

========================================================================================
# Sophon AK8 new inference

// 25.02: new 2HDM bb and cc samples, as well as bc

./run_sm.sh sm_ext/SingleHpmToBC_2HDM 1000 1000 0 8888 ihepel9 DelphesHepMC2WithFilter

i=SingleH0ToBB_2HDM_PT400; N=100; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithFilter -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=SingleH0ToCC_2HDM_PT400; N=100; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithFilter -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err
i=SingleHpmToBC_2HDM_PT400; N=100; start=0; hep_sub run_sm.sh -argu sm_ext/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithFilter -n $N -o logs_240201_run_sm/$i.%{ProcId}.out -e logs_240201_run_sm/$i.%{ProcId}.err

// ntuplize
root -b -q 'makeNtuplesEvalSophonFatJet.C++("/publicfs/cms/user/licq/condor_output/sm_ext/SingleH0ToBB_2HDM_PT400/events_delphes_0.root", "test_ak4.root", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_ak8puppi_full_scale/model_embed.onnx", "JetPUPPIAK8", false)'

hep_sub ntuplize_evalsophon.sh -argu "%{ProcId}" 0 samples/eval_higgs_filelist.txt ihepel9 makeNtuplesEvalSophonFatJet.C -n 150 -o logs_2501_ntuplize/ext.%{ProcId}.out -e logs_2501_ntuplize/ext.%{ProcId}.err
./ntuplize_evalsophon.sh 0 0 samples/eval_higgs_filelist.txt ihepel9 makeNtuplesEvalSophonFatJet.C

========================================================================================
# Sophon AK15

// ntuplize_ak15.sh from ntuplize.sh

./ntuplize_ak15.sh 0 0 samples/train_ak15_all_filelist.txt ihepel9

hep_sub ntuplize_ak15.sh -argu "%{ProcId}" 0 samples/train_ak15_all_filelist.txt ihepel9 -n 2015 -o logs_2501_sophon_ak15/$i.%{ProcId}.out -e logs_2501_sophon_ak15/$i.%{ProcId}.err

========================================================================================

# SM 1l trigger

## ./run_sm.sh sample production

./run_sm.sh sm_1l/WJetsToLNuPlus12J 100 100 0 8888 ihepel9
./run_sm.sh sm_1l/WJetsToLNuPlus12J 10000 10000 0 8888 farm DelphesHepMC2

./run_sm.sh sm_1l/WJetsToLNuPlus12J 10000 10000 0 8888 farm DelphesHepMC2

// test on ihep
./run_sm.sh sm_1l_fj/WJetsToLNu_ht50 100 100 0 8888 ihepel9 DelphesHepMC2

i=WJetsToLNu_ht50; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht100; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht150; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht200; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht250; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht300; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

i=WJetsToLNu_ht100; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht110; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht120; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht130; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht140; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

// test more (for AK15 study)
for i in WJetsToLNu; do for ht in 30 40 60 70 80 90; do rsync sm_1l_custom/${i}_ht50/ sm_1l_custom/${i}_ht$ht; sed -i "s|set htjmin 50|set htjmin $ht|g" sm_1l_custom/${i}_ht${ht}/mg5_step2_templ.dat; done; done

i=WJetsToLNu_ht30; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht40; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht60; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht70; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht80; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_ht90; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err


for i in TTbarSL; do for ht in 90 100 110 120 130 140 150; do rsync sm_1l_fj/${i}_ht80/ sm_1l_fj/${i}_ht$ht; sed -i "s|set htjmin 80|set htjmin $ht|g" sm_1l_fj/${i}_ht${ht}/mg5_step2_templ.dat; done; done

i=TTbarSL_ht80; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht90; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht100; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht110; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht120; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht130; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht140; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_ht150; N=1; start=0; hep_sub run_sm.sh -argu sm_1l_custom/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2 -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

// production
//// for fatjet: use DelphesHepMC2WithSingleLeptonAndFatJetFilter!!
i=WJetsToLNu_HT150; N=750; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_HT150; N=250; start=750; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

// for lep trigger w/o fatjet req (to compute incl xsec)
i=WJetsToLNu; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TW; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TWSL; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WW; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WWSL; N=100; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
// added TTbarSL
i=TTbarSL; N=400; start=100; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL; N=500; start=500; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

i=TTbarSL_FJ; N=100; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_FJ; N=400; start=100; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

// dedicated W->cb samples
i=TTbarSL_WToCB_FJ; N=100; start=0; hep_sub run_sm.sh -argu sm_1l_fj/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TWSL_WToCB; N=25; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WWSL_WToCB; N=25; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=TTbarSL_WToCB; N=25; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

./run_sm.sh sm_1l/TTbarSL_WToCB_FJ 100 100 0 8888 ihepel9 DelphesHepMC2WithSingleLeptonFilter

// w/ 4jets selection
i=WJetsToLNuPlus2J_Delphes4J; N=200; start=0; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAnd4JetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNuPlus2J_Delphes4J; N=800; start=200; hep_sub run_sm.sh -argu sm_1l/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAnd4JetFilter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

./run_sm.sh sm_1l/WJetsToLNuPlus2J_Delphes4J 100 100 0 8888 ihepel9 DelphesHepMC2WithSingleLeptonAnd4JetFilter

// samples for AK15 trigger

./run_sm.sh sm_1l_fjak15/TTbarSL_FJAK15 10000 10000 0 8888 ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetAK15Filter

i=TTbarSL_FJAK15; N=500; start=0; hep_sub run_sm.sh -argu sm_1l_fjak15/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetAK15Filter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_HT50; N=500; start=0; hep_sub run_sm.sh -argu sm_1l_fjak15/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetAK15Filter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err
i=WJetsToLNu_HT50; N=1500; start=500; hep_sub run_sm.sh -argu sm_1l_fjak15/$i 100000 10000 "%{ProcId}" $start ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetAK15Filter -n $N -o logs_241124_sm_1l/$i.%{ProcId}.out -e logs_241124_sm_1l/$i.%{ProcId}.err

// for pythia-vincia-herwig study

./run_sm.sh sm_1l/TTbar0JSL_Vincia_FJ 1000 1000 0 8888 ihepel9 DelphesHepMC2WithSingleLeptonAndFatJetFilter

## ntuplizer (WcbAna)

root -b -q 'makeNtuplesWcbAna.C++("/publicfs/cms/user/licq/condor_output/sm_1l_fj/WJetsToLNu_HT150/events_delphes_0.root", "test_ak4.root", "JetPUPPIAK8", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_full_nonscale_ak4.ddp2-bs512-lr1e-3/model.onnx", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_ak8puppi_full_scale/model_embed.onnx", false)'

./ntuplize_wcbana.sh 0 0 samples_wcb/sm_1l_fj_wjets.txt ihepel9 makeNtuplesWcbAna.C
./ntuplize_wcbana.sh 0 0 samples_wcb/sm_1l_fj_ttbarsl.txt ihepel9 makeNtuplesWcbAna.C

proc=wjets; N=1000;    hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fj_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=ttbarsl_YZ; N=1950; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fj_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=ttbarsl; N=500; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fj_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wwsl; N=100; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=twsl; N=100; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wjets_nocut; N=100;   hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wcbsignals; N=100;   hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err

// resolved Wcb analyzer
proc=ttbarsl; N=100; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wjets; N=200; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wwsl; N=100; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=twsl; N=100; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wcbsignals; N=75;   hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err

// fatjet: move to AK15
//!! remember to add JetPUPPIAK15 to script argument!!
root -b -q 'makeNtuplesWcbAna.C++("/publicfs/cms/user/licq/condor_output/sm_1l_fjak15/TTbarSL_FJAK15/events_delphes_0.root", "out.root", "JetPUPPIAK15", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_full_nonscale_ak4.ddp2-bs512-lr1e-3/model.onnx", "/publicfs/cms/user/licq/pheno/anomdet/gen/delphes_ana_el9/model/JetClassII_full_ak15_manual.ddp4-bs512-lr2e-3/model.onnx")'

./ntuplize_wcbana.sh 0 0 samples_wcb/sm_1l_fj_wjets.txt ihepel9 makeNtuplesWcbAna.C JetPUPPIAK15

proc=wjets; N=1998;  hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fjak15_${proc}.txt ihepel9 makeNtuplesWcbAna.C JetPUPPIAK15 -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=ttbarsl; N=500; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fjak15_${proc}.txt ihepel9 makeNtuplesWcbAna.C JetPUPPIAK15 -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=twww; N=200; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fjak15_${proc}.txt ihepel9 makeNtuplesWcbAna.C JetPUPPIAK15 -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wcbsignals; N=75; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fjak15_${proc}.txt ihepel9 makeNtuplesWcbAna.C JetPUPPIAK15 -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err

// final ntuplizer: 
/// 1) new fj ntuplizer for ttbar, from wofj events (use ntuplize_wcbana_require_fj!)

./ntuplize_wcbana_require_fj.sh 0 0 samples_wcb/sm_1l_fj_ttbarsl_wofj.txt ihepel9 makeNtuplesWcbAna.C

proc=ttbarsl_wofj; N=1000; hep_sub ntuplize_wcbana_require_fj.sh -argu "%{ProcId}" 0 samples_wcb/sm_1l_fj_${proc}.txt ihepel9 makeNtuplesWcbAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err

/// 2) ext samples for rsv
proc=ttbarsl; N=900; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 100 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err
proc=wjets; N=800; hep_sub ntuplize_wcbana.sh -argu "%{ProcId}" 200 samples_wcb/sm_1l_rsv_${proc}.txt ihepel9 makeNtuplesWcbResolvedAna.C -n $N -o logs_2501_ntuplize/$proc.%{ProcId}.out -e logs_2501_ntuplize/$proc.%{ProcId}.err


## merge
hadd sm_1l_fj/merged_ntuple/WJetsToLNu_HT150_ntuple_id0-999.root sm_1l_fj/WJetsToLNu_HT150_ntuple/*.root
hadd sm_1l_fj/merged_ntuple/TTbarSL_YZ_ntuple.root sm_1l_fj/TTbarSL_YZ_*_ntuple/*.root
hadd sm_1l_fj/merged_ntuple/TTbarSL_FJ_ntuple_id0-499.root sm_1l_fj/TTbarSL_FJ_ntuple/*.root
hadd sm_1l/merged_ntuple/WWSL_ntuple_id0-99.root sm_1l/WWSL_ntuple/*.root
hadd sm_1l/merged_ntuple/TWSL_ntuple_id0-99.root sm_1l/TWSL_ntuple/*.root
hadd sm_1l/merged_ntuple/WJetsToLNu_ntuple_id0-99.root sm_1l/WJetsToLNu_ntuple/*.root

hadd sm_1l_fj/merged_ntuple/TTbarSL_woFJ_ntuple_id0-999.root sm_1l_fj/TTbarSL_woFJ_ntuple/*.root

// signals
hadd sm_1l/merged_ntuple/WWSL_WToCB_ntuple_id0-24.root sm_1l/WWSL_WToCB_ntuple/*.root
hadd sm_1l/merged_ntuple/TWSL_WToCB_ntuple_id0-24.root sm_1l/TWSL_WToCB_ntuple/*.root
hadd sm_1l/merged_ntuple/TTbarSL_WToCB_ntuple_id0-24.root sm_1l/TTbarSL_WToCB_ntuple/*.root
hadd sm_1l_fj/merged_ntuple/TTbarSL_WToCB_FJ_ntuple_id0-24.root sm_1l_fj/TTbarSL_WToCB_FJ_ntuple/*.root

// resolved samples
hadd sm_1l_rsv/merged_ntuple/WJetsToLNuPlus2J_Delphes4J_ntuple_id0-199.root sm_1l_rsv/WJetsToLNuPlus2J_Delphes4J_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/WWSL_ntuple_id0-99.root sm_1l_rsv/WWSL_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/TWSL_ntuple_id0-99.root sm_1l_rsv/TWSL_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/TTbarSL_ntuple_id0-99.root sm_1l_rsv/TTbarSL_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/WWSL_WToCB_ntuple_id0-24.root sm_1l_rsv/WWSL_WToCB_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/TWSL_WToCB_ntuple_id0-24.root sm_1l_rsv/TWSL_WToCB_ntuple/*.root
hadd sm_1l_rsv/merged_ntuple/TTbarSL_WToCB_ntuple_id0-24.root sm_1l_rsv/TTbarSL_WToCB_ntuple/*.root
/// new
hadd sm_1l_rsv/merged_ntuple/WJetsToLNuPlus2J_Delphes4J_ntuple_id200-999.root `echo sm_1l_rsv/WJetsToLNuPlus2J_Delphes4J_ntuple/ntuples_{200..999}.root`
hadd sm_1l_rsv/merged_ntuple/TTbarSL_ntuple_id100-999.root `echo sm_1l_rsv/TTbarSL_ntuple/ntuples_{100..999}.root`

// AK15 jets
hadd sm_1l_fjak15/merged_ntuple/WJetsToLNu_HT50_ntuple_id0-1997.root sm_1l_fjak15/WJetsToLNu_HT50_ntuple/*.root
hadd sm_1l_fjak15/merged_ntuple/TTbarSL_FJAK15_ntuple_id0-499.root sm_1l_fjak15/TTbarSL_FJAK15_ntuple/*.root

hadd sm_1l_fjak15/merged_ntuple/WWSL_ntuple_id0-99.root sm_1l_fjak15/WWSL_ntuple/*.root
hadd sm_1l_fjak15/merged_ntuple/TWSL_ntuple_id0-99.root sm_1l_fjak15/TWSL_ntuple/*.root
hadd sm_1l_fjak15/merged_ntuple/WWSL_WToCB_ntuple_id0-24.root sm_1l_fjak15/WWSL_WToCB_ntuple/*.root
hadd sm_1l_fjak15/merged_ntuple/TWSL_WToCB_ntuple_id0-24.root sm_1l_fjak15/TWSL_WToCB_ntuple/*.root
hadd sm_1l_fjak15/merged_ntuple/TTbarSL_WToCB_ntuple_id0-24.root sm_1l_fjak15/TTbarSL_WToCB_ntuple/*.root

## merge QCD/Xbb/Xcc for evaluation
hadd sm/merged_ntuple/QCD_ntuple_id2000-2499.root `echo sm/QCD_ntuple/ntuples_{2000..2499}.root`
hadd sm_ext/merged_ntuple/SingleH0ToBB_2HDM_PT400_ntuple_id0-49.root `echo sm_ext/SingleH0ToBB_2HDM_PT400_ntuple/ntuples_{0..49}.root`
hadd sm_ext/merged_ntuple/SingleH0ToCC_2HDM_PT400_ntuple_id0-49.root `echo sm_ext/SingleH0ToCC_2HDM_PT400_ntuple/ntuples_{0..49}.root`
hadd sm_ext/merged_ntuple/SingleHpmToBC_2HDM_PT400_ntuple_id0-49.root `echo sm_ext/SingleHpmToBC_2HDM_PT400_ntuple/ntuples_{0..49}.root`