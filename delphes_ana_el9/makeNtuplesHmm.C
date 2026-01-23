#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "EventData.h"
#include "GenPartGenericProcessor.h"
// #include "FatJetMatching.h"
#include "OrtHelperSophonAK4.h"
#include "JetMatching.h"
#include "JetGhostMatching.h"

// Function to process jet-related information
void processJet(const Jet* jet, EventData& data, OrtHelperSophonAK4* sp4helper = nullptr, const Vertex* pv = nullptr, const JetGhostMatching::JetGhostContent* ghostContent = nullptr) {
    data.vfloatVars["jet_pt"]->push_back(jet->PT);
    data.vfloatVars["jet_eta"]->push_back(jet->Eta);
    data.vfloatVars["jet_phi"]->push_back(jet->Phi);
    data.vfloatVars["jet_energy"]->push_back(jet->P4().Energy());
    data.vfloatVars["jet_mass"]->push_back(jet->Mass);
    
    if (ghostContent) {
        data.vintVars["jet_hadronFlavor"]->push_back(ghostContent->hadronFlavor);
        data.vintVars["jet_partonFlavor"]->push_back(ghostContent->partonFlavor);
    } else {
        data.vintVars["jet_hadronFlavor"]->push_back(-1);
        data.vintVars["jet_partonFlavor"]->push_back(-1);
    }
    data.vintVars["jet_flavor"]->push_back(jet->Flavor);

    int nparticles = 0;
    // Loop over all jet's constituents
    for (Int_t idx_ct = 0; idx_ct < jet->Constituents.GetEntriesFast(); ++idx_ct) {
        const TObject *object = jet->Constituents.At(idx_ct);
        if (object && object->IsA() == ParticleFlowCandidate::Class()) {
            nparticles++;
        }
    }
    data.vfloatVars["jet_nparticles"]->push_back(nparticles);
    
    // Apply SophonAK4 tagging if available
    if (sp4helper != nullptr) {
        std::map<std::string, std::vector<float>> particleVars;
        std::map<std::string, float> jetVars;
        
        // Prepare input for SophonAK4
        for (Int_t idx_ct = 0; idx_ct < jet->Constituents.GetEntriesFast(); ++idx_ct) {
            const TObject *object = jet->Constituents.At(idx_ct);
            if (!object) continue;
            
            if (object->IsA() == ParticleFlowCandidate::Class()) {
                const ParticleFlowCandidate *pfcand = (ParticleFlowCandidate *)object;
                
                if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) continue;
                
                TLorentzVector p4 = pfcand->P4();
                
                particleVars["part_px"].push_back(p4.Px());
                particleVars["part_py"].push_back(p4.Py());
                particleVars["part_pz"].push_back(p4.Pz());
                particleVars["part_energy"].push_back(p4.E());
                particleVars["part_pt"].push_back(pfcand->PT);
                particleVars["part_deta"].push_back((jet->Eta > 0 ? 1 : -1) * (pfcand->Eta - jet->Eta));
                particleVars["part_dphi"].push_back(deltaPhi(pfcand->Phi, jet->Phi));
                particleVars["part_charge"].push_back(pfcand->Charge);
                particleVars["part_pid"].push_back(pfcand->PID);
                particleVars["part_d0val"].push_back(pfcand->D0);
                particleVars["part_d0err"].push_back(pfcand->ErrorD0);
                particleVars["part_dzval"].push_back((pv && pfcand->DZ != 0) ? (pfcand->DZ - pv->Z) : pfcand->DZ);
                particleVars["part_dzerr"].push_back(pfcand->ErrorDZ);
            }
        }
        
        jetVars["jet_pt"] = jet->PT;
        jetVars["jet_eta"] = jet->Eta;
        jetVars["jet_phi"] = jet->Phi;
        jetVars["jet_energy"] = jet->P4().Energy();
        
        // Infer SophonAK4 model
        sp4helper->infer_model(particleVars, jetVars);
        const auto &sp4output = sp4helper->get_output();
        
        // Fill SophonAK4 scores
        data.vfloatVars["jet_sophonAK4_probB"]->push_back(
            std::accumulate(sp4output.begin() + 0, sp4output.begin() + 5, 0.0)
        );
        data.vfloatVars["jet_sophonAK4_probC"]->push_back(
            std::accumulate(sp4output.begin() + 5, sp4output.begin() + 10, 0.0)
        );
        data.vfloatVars["jet_sophonAK4_probL"]->push_back(
            std::accumulate(sp4output.begin() + 10, sp4output.begin() + 32, 0.0)
        );

    } else {
        // If no SophonAK4 model, fill with default values
        data.vfloatVars["jet_sophonAK4_probB"]->push_back(-1.0);
        data.vfloatVars["jet_sophonAK4_probC"]->push_back(-1.0);
        data.vfloatVars["jet_sophonAK4_probL"]->push_back(-1.0);
    }
}

// Function to process muon-related information
void processMuon(const Muon* muon, EventData& data) {
    data.vfloatVars["mu_pt"]->push_back(muon->PT);
    data.vfloatVars["mu_eta"]->push_back(muon->Eta);
    data.vfloatVars["mu_phi"]->push_back(muon->Phi);
    data.vfloatVars["mu_energy"]->push_back(muon->P4().E());
    data.vintVars["mu_charge"]->push_back(muon->Charge);
    data.vintVars["mu_pid"]->push_back(-13 * muon->Charge);
    data.vfloatVars["mu_iso"]->push_back(muon->IsolationVar);
}

// Function to process particle information
void processParticle(const ParticleFlowCandidate* pfcand, EventData& data, const Vertex* pv, 
                    const TLorentzVector& pfcand_sum) {
    if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) {
        return;
    }

    TLorentzVector p4 = pfcand->P4();

    // Calculate delta values with pfcand_sum
    float deta = p4.Eta() - pfcand_sum.Eta();
    float dphi = deltaPhi(p4.Phi(), pfcand_sum.Phi());
    float dr = std::sqrt(deta*deta + dphi*dphi);

    data.vfloatVars["part_px"]->push_back(p4.Px());  
    data.vfloatVars["part_py"]->push_back(p4.Py());  
    data.vfloatVars["part_pz"]->push_back(p4.Pz());  
    data.vfloatVars["part_energy"]->push_back(p4.E());
    data.vfloatVars["part_mass"]->push_back(pfcand->Mass);
    data.vfloatVars["part_pt"]->push_back(pfcand->PT);
    data.vfloatVars["part_eta"]->push_back(pfcand->Eta);
    data.vfloatVars["part_phi"]->push_back(pfcand->Phi);
    data.vintVars["part_charge"]->push_back(pfcand->Charge);
    data.vintVars["part_pid"]->push_back(pfcand->PID);
    data.vfloatVars["part_d0val"]->push_back(pfcand->D0);
    data.vfloatVars["part_d0err"]->push_back(pfcand->ErrorD0);
    data.vfloatVars["part_dzval"]->push_back((pv && pfcand->DZ != 0) ? (pfcand->DZ - pv->Z) : pfcand->DZ);
    data.vfloatVars["part_dzerr"]->push_back(pfcand->ErrorDZ);
    
    // Add new delta variables
    data.vfloatVars["part_dr"]->push_back(dr);
    data.vfloatVars["part_deta"]->push_back(deta);
    data.vfloatVars["part_dphi"]->push_back(dphi);
}

// Function to process GenParticle information for H->μμ analysis
void processGenParticleList(EventData& data, TClonesArray* branchParticle) {
    std::vector<const GenParticle*> lhe_muons;
    std::vector<const GenParticle*> lhe_higgs_or_z;
    
    // First pass: collect muons and Higgs/Z bosons with status 22 or 23
    for(int i = 0; i < branchParticle->GetEntriesFast(); ++i) {
        const GenParticle* genparticle = (GenParticle*)branchParticle->At(i);
        
        // Collect muons with status 23 (output particles)
        if (genparticle->Status == 23 && std::abs(genparticle->PID) == 13) {
            lhe_muons.push_back(genparticle);
        }
        
        // Collect Higgs or Z boson with status 22
        if (genparticle->Status == 22) {
            int absPID = std::abs(genparticle->PID);
            if (absPID == 25 || absPID == 35 || absPID == 23) {
                lhe_higgs_or_z.push_back(genparticle);
            }
        }
    }
    
    // Fill gen muon variables
    bool filled_muons = false;
    
    // First, check if there are output muons
    if (lhe_muons.size() >= 2) {
        // Sort by pt
        std::sort(lhe_muons.begin(), lhe_muons.end(), 
                 [](const GenParticle* a, const GenParticle* b) { return a->PT > b->PT; });
        
        const GenParticle* mu1 = lhe_muons[0];
        const GenParticle* mu2 = lhe_muons[1];
        
        TLorentzVector p4_mu1 = mu1->P4();
        TLorentzVector p4_mu2 = mu2->P4();
        
        data.floatVars["gen_mu1_pt"] = mu1->PT;
        data.floatVars["gen_mu1_eta"] = mu1->Eta;
        data.floatVars["gen_mu1_phi"] = mu1->Phi;
        data.floatVars["gen_mu1_mass"] = mu1->Mass;
        
        data.floatVars["gen_mu2_pt"] = mu2->PT;
        data.floatVars["gen_mu2_eta"] = mu2->Eta;
        data.floatVars["gen_mu2_phi"] = mu2->Phi;
        data.floatVars["gen_mu2_mass"] = mu2->Mass;
        
        TLorentzVector dimuon_p4 = p4_mu1 + p4_mu2;
        data.floatVars["gen_dimu_mass"] = dimuon_p4.M();
        
        filled_muons = true;
    } else if (lhe_muons.size() == 1) {
        std::cerr << "Only one muon with status 23 found. Expected at least two." << std::endl;
    }
    
    // If not filled from output muons, check Higgs or Z boson daughters
    if (!filled_muons && !lhe_higgs_or_z.empty()) {
        for (const GenParticle* parent : lhe_higgs_or_z) {
            std::vector<const GenParticle*> muon_daughters;
            
            // Get daughters
            for (int i = parent->D1; i <= parent->D2; ++i) {
                if (i >= 0 && i < branchParticle->GetEntriesFast()) {
                    const GenParticle* daughter = (GenParticle*)branchParticle->At(i);
                    if (std::abs(daughter->PID) == 13) {
                        muon_daughters.push_back(daughter);
                    }
                }
            }
            
            if (muon_daughters.size() == 2) {
                // Sort by pt
                std::sort(muon_daughters.begin(), muon_daughters.end(), 
                         [](const GenParticle* a, const GenParticle* b) { return a->PT > b->PT; });
                
                const GenParticle* mu1 = muon_daughters[0];
                const GenParticle* mu2 = muon_daughters[1];
                
                TLorentzVector p4_mu1 = mu1->P4();
                TLorentzVector p4_mu2 = mu2->P4();
                
                data.floatVars["gen_mu1_pt"] = mu1->PT;
                data.floatVars["gen_mu1_eta"] = mu1->Eta;
                data.floatVars["gen_mu1_phi"] = mu1->Phi;
                data.floatVars["gen_mu1_mass"] = mu1->Mass;
                
                data.floatVars["gen_mu2_pt"] = mu2->PT;
                data.floatVars["gen_mu2_eta"] = mu2->Eta;
                data.floatVars["gen_mu2_phi"] = mu2->Phi;
                data.floatVars["gen_mu2_mass"] = mu2->Mass;
                
                TLorentzVector dimuon_p4 = p4_mu1 + p4_mu2;
                data.floatVars["gen_dimu_mass"] = dimuon_p4.M();
                
                filled_muons = true;
                break;
            }
        }
    }
    
    // If still not filled, leave variables at default
}

void makeNtuplesHmm(TString inputFile, TString outputFile, TString modelPathAK4, TString jetBranch = "JetPUPPI") {
    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // Define all branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        {"pass_singlemu_trigger", "int"},
        {"pass_dimu_selection", "int"},
        {"pfcand_sum_mass", "float"},
        {"pfcand_sum_HT", "float"},
        {"pfcand_sum_pt", "float"},
        {"pfcand_sum_eta", "float"},
        {"pfcand_sum_phi", "float"},
        {"pfcand_sum_energy", "float"},
        
        // Gen muon variables
        {"gen_mu1_pt", "float"},
        {"gen_mu1_eta", "float"},
        {"gen_mu1_phi", "float"},
        {"gen_mu1_mass", "float"},
        {"gen_mu2_pt", "float"},
        {"gen_mu2_eta", "float"},
        {"gen_mu2_phi", "float"},
        {"gen_mu2_mass", "float"},
        {"gen_dimu_mass", "float"},
        
        // Jet variables (as vectors)
        {"jet_pt", "vector<float>"},
        {"jet_eta", "vector<float>"},
        {"jet_phi", "vector<float>"},
        {"jet_energy", "vector<float>"},
        {"jet_mass", "vector<float>"},
        {"jet_hadronFlavor", "vector<int>"},
        {"jet_partonFlavor", "vector<int>"},
        {"jet_flavor", "vector<int>"},
        {"jet_nparticles", "vector<float>"},
        {"jet_sophonAK4_probB", "vector<float>"},
        {"jet_sophonAK4_probC", "vector<float>"},
        {"jet_sophonAK4_probL", "vector<float>"},
        
        // Muon variables (as vectors)
        {"mu_pt", "vector<float>"},
        {"mu_eta", "vector<float>"},
        {"mu_phi", "vector<float>"},
        {"mu_energy", "vector<float>"},
        {"mu_charge", "vector<int>"},
        {"mu_pid", "vector<int>"},
        {"mu_iso", "vector<float>"},

        // MET variables
        {"met_pt", "float"},
        {"met_phi", "float"},

        // Particle variables
        {"part_label", "vector<int>"},
        {"part_px", "vector<float>"},
        {"part_py", "vector<float>"},
        {"part_pz", "vector<float>"},
        {"part_energy", "vector<float>"},
        {"part_mass", "vector<float>"},
        {"part_pt", "vector<float>"},
        {"part_eta", "vector<float>"},
        {"part_phi", "vector<float>"},
        {"part_charge", "vector<int>"},
        {"part_pid", "vector<int>"},
        {"part_d0val", "vector<float>"},
        {"part_d0err", "vector<float>"},
        {"part_dzval", "vector<float>"},
        {"part_dzerr", "vector<float>"},
        {"part_dr", "vector<float>"},
        {"part_deta", "vector<float>"},
        {"part_dphi", "vector<float>"},

        // Gen-level info
        {"genpart_pt", "vector<float>"},
        {"genpart_eta", "vector<float>"},
        {"genpart_phi", "vector<float>"},
        {"genpart_energy", "vector<float>"},
        {"genpart_pid", "vector<int>"},
        {"process_index", "int"},

        {"gen_weight", "vector<float>"},
    };

    // Initialize EventData
    EventData data(branchList);
    data.setOutputBranch(tree);

    // Read input file
    TChain *chain = new TChain("Delphes");
    chain->Add(inputFile);
    ExRootTreeReader *treeReader = new ExRootTreeReader(chain);
    Long64_t allEntries = treeReader->GetEntries();

    std::cerr << "** Input file: " << inputFile << std::endl;
    std::cerr << "** Output file: " << outputFile << std::endl;
    std::cerr << "** AK4 model path: " << modelPathAK4 << std::endl;
    std::cerr << "** Jet branch: " << jetBranch << std::endl;
    std::cerr << "** Total events: " << allEntries << std::endl;

    // set branches
    TClonesArray *branchVertex = treeReader->UseBranch("Vertex");
    TClonesArray *branchParticle = treeReader->UseBranch("Particle");
    TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
    TClonesArray *branchJet = treeReader->UseBranch(jetBranch);
    TClonesArray *branchMuon = treeReader->UseBranch("Muon");
    TClonesArray *branchMET = treeReader->UseBranch("PuppiMissingET");
    TClonesArray *branchWeight = treeReader->UseBranch("Weight");

    double jetR = 0.4;
    std::cerr << "jetR = " << jetR << std::endl;
    
    // Initialize JetGhostMatching
    JetGhostMatching ghostMatch(treeReader, jetR, 1e-18);
    
    // Initialize SophonAK4 helper
    OrtHelperSophonAK4 *sp4helper = nullptr;
    bool use_sophon_ak4 = !modelPathAK4.IsNull() && modelPathAK4 != "";
    
    if (use_sophon_ak4) {
        std::cerr << "Initializing SophonAK4 model..." << std::endl;
        sp4helper = new OrtHelperSophonAK4(modelPathAK4.Data(), false); // debug=false
        std::cerr << "SophonAK4 model initialized." << std::endl;
    } else {
        std::cerr << "No SophonAK4 model provided, will not use jet tagging." << std::endl;
    }

    // Initialize GenPartGenericProcessor
    GenPartGenericProcessor* genhelper = new GenPartGenericProcessor(false); // debug=false

    // event loop
    int num_processed = 0;
    int num_pass_selection = 0;
    
    for (Long64_t entry = 0; entry < allEntries; ++entry) {
        if (entry % 100 == 0) {
            std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
        }

        treeReader->ReadEntry(entry);
        ++num_processed;

        // reset data
        data.reset();
        
        // Check single muon trigger: at least one muon with pT>24, |η|<2.4, IsolationVar<0.15
        bool pass_singlemu_trigger = false;
        for (Int_t i = 0; i < branchMuon->GetEntries(); ++i) {
            const Muon *muon = (Muon *)branchMuon->At(i);
            if (muon->PT > 24 && std::abs(muon->Eta) < 2.4 && muon->IsolationVar < 0.15) {
                pass_singlemu_trigger = true;
                break;
            }
        }
        data.intVars["pass_singlemu_trigger"] = pass_singlemu_trigger ? 1 : 0;
        
        if (!pass_singlemu_trigger) {
            std::cerr << "Event failed single muon trigger criteria. This shouldn't happen if the delphes events have passed this filter" << std::endl;
            continue;
        }

        // Process muons that satisfy pT>20, |η|<2.4, IsolationVar<0.25
        int num_selected_muons = 0;
        for (Int_t i = 0; i < branchMuon->GetEntries(); ++i) {
            const Muon *muon = (Muon *)branchMuon->At(i);
            if (muon->PT > 20 && std::abs(muon->Eta) < 2.4 && muon->IsolationVar < 0.25) {
                processMuon(muon, data);
                ++num_selected_muons;
            }
        }
        
        // Check dimuon selection: require at least 2 muons
        bool pass_dimu_selection = (num_selected_muons >= 2);
        data.intVars["pass_dimu_selection"] = pass_dimu_selection ? 1 : 0;
        
        if (!pass_dimu_selection) {
            continue;
        }

        // Create mapping for particles to jets
        std::map<const TObject*, int> objectToIndexMap;
        
        // Get primary vertex for SophonAK4
        const Vertex *pv = (branchVertex != nullptr) ? ((Vertex *)branchVertex->At(0)) : nullptr;
        
        // Collect jets
        std::vector<Jet*> eventJets;
        for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
            eventJets.push_back((Jet *)branchJet->At(i));
        }

        // Get GenParticle
        std::vector<GenParticle*> genParticles;
        for (Int_t j = 0; j < branchParticle->GetEntriesFast(); ++j) {
            genParticles.push_back((GenParticle *)branchParticle->At(j));
        }

        // Ghost Matching for AK4 jets
        std::vector<JetGhostMatching::JetGhostContent> ghostContents = 
            ghostMatch.getDetailedGhostContent(eventJets, genParticles);
        
        // Process jets that satisfy pT>25, |η|<4.7
        int stored_jet_idx = 0;
        for(Int_t idx_jet = 0; idx_jet < branchJet->GetEntriesFast(); ++idx_jet) {
            const Jet *jet = (Jet*) branchJet->At(idx_jet);
            
            // Apply jet selection
            if (jet->PT > 25 && std::abs(jet->Eta) < 4.7) {
                const auto& ghostContent = ghostContents[idx_jet];
                processJet(jet, data, sp4helper, pv, &ghostContent);
                
                // Record jet components for particle labeling (use stored index)
                for (Int_t j = 0; j < jet->Constituents.GetEntriesFast(); ++j) {
                    const TObject *object = jet->Constituents.At(j);
                    if (object && object->IsA() == ParticleFlowCandidate::Class()) {
                        objectToIndexMap[object] = stored_jet_idx;
                    }
                }
                ++stored_jet_idx;
            }
        }

        // Process MET
        const MissingET *met = (MissingET *)branchMET->At(0);
        data.floatVars["met_pt"] = met->MET;
        data.floatVars["met_phi"] = met->Phi;

        // PF candidates
        TLorentzVector pfcand_sum(0, 0, 0, 0);
        double pfcand_sum_HT = 0.0;
        
        // First loop to calculate the sum
        for(int i = 0; i < branchPFCand->GetEntriesFast(); ++i) {
            const ParticleFlowCandidate *pfcand = (ParticleFlowCandidate*)branchPFCand->At(i);
            
            if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) {
                continue;
            }
        
            TLorentzVector pfcand_p4;
            pfcand_p4.SetPtEtaPhiM(pfcand->PT, pfcand->Eta, pfcand->Phi, pfcand->Mass);
            pfcand_sum += pfcand_p4;
            pfcand_sum_HT += pfcand->PT;
        }
        
        // Store pfcand_sum variables
        data.floatVars["pfcand_sum_mass"] = pfcand_sum.M();
        data.floatVars["pfcand_sum_HT"] = pfcand_sum_HT;
        data.floatVars["pfcand_sum_pt"] = pfcand_sum.Pt();
        data.floatVars["pfcand_sum_eta"] = pfcand_sum.Eta();
        data.floatVars["pfcand_sum_phi"] = pfcand_sum.Phi();
        data.floatVars["pfcand_sum_energy"] = pfcand_sum.E();
        
        // Second loop to process individual particles with the sum information
        for(int i = 0; i < branchPFCand->GetEntriesFast(); ++i) {
            const ParticleFlowCandidate *pfcand = (ParticleFlowCandidate*)branchPFCand->At(i);

            if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) {
                continue;
            }
            
            // Get AK4 jet label
            int part_label = objectToIndexMap.count(pfcand) ? objectToIndexMap[pfcand] : -1;
            
            // Add label to vector
            data.vintVars["part_label"]->push_back(part_label);
            
            // Process particle
            processParticle(pfcand, data, pv, pfcand_sum);
        }

        // GenParticles - find gen muons
        processGenParticleList(data, branchParticle);
        
        // selected truth-particles info
        genhelper->process("generic", branchParticle);

        data.vfloatVars["genpart_pt"]->insert(data.vfloatVars["genpart_pt"]->end(), genhelper->getData().pt.begin(), genhelper->getData().pt.end());
        data.vfloatVars["genpart_eta"]->insert(data.vfloatVars["genpart_eta"]->end(), genhelper->getData().eta.begin(), genhelper->getData().eta.end());
        data.vfloatVars["genpart_phi"]->insert(data.vfloatVars["genpart_phi"]->end(), genhelper->getData().phi.begin(), genhelper->getData().phi.end());
        data.vfloatVars["genpart_energy"]->insert(data.vfloatVars["genpart_energy"]->end(), genhelper->getData().energy.begin(), genhelper->getData().energy.end());
        data.vintVars["genpart_pid"]->insert(data.vintVars["genpart_pid"]->end(), genhelper->getData().pid.begin(), genhelper->getData().pid.end());
        data.intVars["process_index"] = genhelper->getData().user_index;
        
        // Read generator weights
        std::vector<float> gen_weight_vec;
        int nWeights = branchWeight->GetEntriesFast();
        for (int iw = 0; iw < nWeights; ++iw) {
            const Weight* weight = (Weight*)branchWeight->At(iw);
            gen_weight_vec.push_back(weight->Weight);
        }
        data.vfloatVars["gen_weight"]->insert(data.vfloatVars["gen_weight"]->end(), gen_weight_vec.begin(), gen_weight_vec.end());

        tree->Fill();
        ++num_pass_selection;

    } // end event loop

    tree->Write();
    std::cerr << TString::Format("** Written %d events to output %s; %d events passing single muon trigger and dimuon selection", 
                                num_processed, outputFile.Data(), num_pass_selection) << std::endl;

    // Clean up
    if (sp4helper) {
        delete sp4helper;
    }
    if (genhelper) {
        delete genhelper;
    }
    
    delete treeReader;
    delete chain;
    delete fout;
}
