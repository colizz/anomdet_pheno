# submit jobs on ihep
hep_sub run.sh -argu train_higgs2p 100000 100 "%{ProcId}" 0 ihep -n 160
hep_sub run.sh -argu train_higgs4p 100000 100 "%{ProcId}" 0 ihep -n 1200 # failed, but 
hep_sub run.sh -argu train_higgs4p 100000 100 "%{ProcId}" 1000 ihep -n 200
hep_sub run.sh -argu train_higgspm2p 100000 100 "%{ProcId}" 0 ihep -n 120

hep_sub run.sh -argu train_qcd 100000 100 "%{ProcId}" 0 ihep -n 200

# submit ntuplizer

hep_sub ntuplize.sh -argu "%{ProcId}" 0 ihep -n 600


hep_sub run.sh -argu GJets 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu ZJetsToQQ 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu ZJetsToLL 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu ZJetsToNuNu 10000 10000 "%{ProcId}" 0 ihep -n 1
hep_sub run.sh -argu  10000 10000 "%{ProcId}" 0 ihep -n 1
