# submit jobs on ihep
hep_sub run.sh -argu train_higgs2p 100000 100 "%{ProcId}" 0 ihep -n 160
hep_sub run.sh -argu train_higgs4p 100000 100 "%{ProcId}" 0 ihep -n 1200 # failed, but 
hep_sub run.sh -argu train_higgs4p 100000 100 "%{ProcId}" 1000 ihep -n 200
hep_sub run.sh -argu train_higgspm2p 100000 100 "%{ProcId}" 0 ihep -n 120

hep_sub run.sh -argu train_qcd 100000 100 "%{ProcId}" 0 ihep -n 200

hep_sub run.sh -argu train_hbs 100000 100 "%{ProcId}" 0 ihep -n 20

# submit ntuplizer

hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_all_filelist.txt 1 ihep -n 600
hep_sub ntuplize.sh -argu "%{ProcId}" 600 samples/train_all_filelist.txt 1 ihep -n 1043

./ntuplize.sh 0 0 samples/train_hbs_filelist.txt 0 ihep ## test
hep_sub ntuplize.sh -argu "%{ProcId}" 0 samples/train_hbs_filelist.txt 0 ihep -n 20

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