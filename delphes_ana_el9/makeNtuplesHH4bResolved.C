#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "EventData.h"
#include "FatJetMatching.h"
#include "OrtHelperSophonAK4.h"

// Function to process jet-related information
void processJet(const Jet* jet, EventData& data, OrtHelperSophonAK4* sp4helper = nullptr, const Vertex* pv = nullptr) {
    data.vfloatVars["jet_pt"]->push_back(jet->PT);
    data.vfloatVars["jet_eta"]->push_back(jet->Eta);
    data.vfloatVars["jet_phi"]->push_back(jet->Phi);
    data.vfloatVars["jet_energy"]->push_back(jet->P4().Energy());
    data.vintVars["jet_flavor"]->push_back(jet->Flavor);
    data.vfloatVars["jet_sdmass"]->push_back(jet->SoftDroppedP4[0].M());
    data.vfloatVars["jet_trmass"]->push_back(jet->TrimmedP4[0].M());
    data.vfloatVars["jet_tau1"]->push_back(jet->Tau[0]);
    data.vfloatVars["jet_tau2"]->push_back(jet->Tau[1]);
    data.vfloatVars["jet_tau3"]->push_back(jet->Tau[2]);
    data.vfloatVars["jet_tau4"]->push_back(jet->Tau[3]);

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
        data.vfloatVars["jet_sophonAK4_probB"]->push_back(sp4output[0] + sp4output[1] + sp4output[17]);
        data.vfloatVars["jet_sophonAK4_probC"]->push_back(sp4output[2] + sp4output[3] + sp4output[18]);
        data.vfloatVars["jet_sophonAK4_probL"]->push_back(
            std::accumulate(sp4output.begin() + 4, sp4output.begin() + 11, 0.0) + 
            std::accumulate(sp4output.begin() + 19, sp4output.begin() + 23, 0.0)
        );
    } else {
        // If no SophonAK4 model, fill with default values
        data.vfloatVars["jet_sophonAK4_probB"]->push_back(-1.0);
        data.vfloatVars["jet_sophonAK4_probC"]->push_back(-1.0);
        data.vfloatVars["jet_sophonAK4_probL"]->push_back(-1.0);
    }
}

// Function to process particle information
void processParticle(const ParticleFlowCandidate* pfcand, EventData& data, const Vertex* pv, int part_label) {
    if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) {
        return;
    }

    TLorentzVector p4 = pfcand->P4();

    data.vintVars["part_label"]->push_back(part_label);
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
}

// Function to process GenParticle information
void processGenParticle(const GenParticle* genparticle, EventData& data, TClonesArray* branchParticle, int& higgs_count, TLorentzVector& higgs1_p4, TLorentzVector& higgs2_p4) {
    if(((std::abs(genparticle->PID) == 25)||(std::abs(genparticle->PID) == 35)) && genparticle->Status == 22) {
        TLorentzVector p4 = genparticle->P4();
        
        if(higgs_count == 0) {
            // First Higgs
            data.floatVars["gen_higgs1_pt"] = genparticle->PT;
            data.floatVars["gen_higgs1_eta"] = genparticle->Eta;
            data.floatVars["gen_higgs1_phi"] = genparticle->Phi;
            data.floatVars["gen_higgs1_mass"] = genparticle->Mass;
            higgs1_p4 = p4;
            higgs_count++;
        } else if(higgs_count == 1) {
            // Second Higgs
            data.floatVars["gen_higgs2_pt"] = genparticle->PT;
            data.floatVars["gen_higgs2_eta"] = genparticle->Eta;
            data.floatVars["gen_higgs2_phi"] = genparticle->Phi;
            data.floatVars["gen_higgs2_mass"] = genparticle->Mass;
            higgs2_p4 = p4;
            higgs_count++;
            
            // Now that we have both Higgs, calculate dihiggs variables
            TLorentzVector dihiggs_p4 = higgs1_p4 + higgs2_p4;
            data.floatVars["gen_dihiggs_mass"] = dihiggs_p4.M();
            data.floatVars["gen_dihiggs_HT"] = data.floatVars["gen_higgs1_pt"] + data.floatVars["gen_higgs2_pt"];
        }
        // Ignore additional Higgs bosons if there are more than 2
    }
    else if(genparticle->PT > 0 && ((std::abs(genparticle->PID)>=ParticleID::p_d && std::abs(genparticle->PID)<=ParticleID::p_b) || std::abs(genparticle->PID)==21) && genparticle->Status == 71) {
        bool isFromHiggsDecay = false;
        int motherIndex = genparticle->M1;
        while(motherIndex != -1) {
            const GenParticle *motherParticle = (GenParticle*) branchParticle->At(motherIndex);
            if(std::abs(motherParticle->PID) == 25 || std::abs(motherParticle->PID) == 35) {
                isFromHiggsDecay = true;
                break;
            }
            motherIndex = motherParticle->M1;
        }

        TLorentzVector p4 = genparticle->P4();
        
        data.vintVars["gen_parton_fromhh"]->push_back(isFromHiggsDecay ? 1 : 0);
        data.vfloatVars["gen_parton_px"]->push_back(p4.Px());
        data.vfloatVars["gen_parton_py"]->push_back(p4.Py());
        data.vfloatVars["gen_parton_pz"]->push_back(p4.Pz());
        data.vfloatVars["gen_parton_energy"]->push_back(p4.E());
        data.vfloatVars["gen_parton_mass"]->push_back(genparticle->Mass);
        data.vfloatVars["gen_parton_pt"]->push_back(genparticle->PT);
        data.vfloatVars["gen_parton_eta"]->push_back(genparticle->Eta);
        data.vfloatVars["gen_parton_phi"]->push_back(genparticle->Phi);
        data.vintVars["gen_parton_charge"]->push_back(genparticle->Charge);
        data.vintVars["gen_parton_pid"]->push_back(genparticle->PID);
    }
}

void makeNtuplesHH4bResolved(TString inputFile, TString outputFile, TString modelPath, TString jetBranch = "JetPUPPI") {
    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // Define all branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        {"pass_selection", "float"},
        {"HT", "float"},
        {"pfcand_sum_mass", "float"},
        {"pfcand_sum_HT", "float"},
        
        // Higgs variables
        {"gen_higgs1_pt", "float"},
        {"gen_higgs1_eta", "float"},
        {"gen_higgs1_phi", "float"},
        {"gen_higgs1_mass", "float"},
        {"gen_higgs2_pt", "float"},
        {"gen_higgs2_eta", "float"},
        {"gen_higgs2_phi", "float"},
        {"gen_higgs2_mass", "float"},
        {"gen_dihiggs_mass", "float"},
        {"gen_dihiggs_HT", "float"},
        
        // Jet variables (as vectors)
        {"jet_pt", "vector<float>"},
        {"jet_eta", "vector<float>"},
        {"jet_phi", "vector<float>"},
        {"jet_energy", "vector<float>"},
        {"jet_flavor", "vector<int>"},
        {"jet_sdmass", "vector<float>"},
        {"jet_trmass", "vector<float>"},
        {"jet_tau1", "vector<float>"},
        {"jet_tau2", "vector<float>"},
        {"jet_tau3", "vector<float>"},
        {"jet_tau4", "vector<float>"},
        {"jet_nparticles", "vector<float>"},
        {"jet_sophonAK4_probB", "vector<float>"},
        {"jet_sophonAK4_probC", "vector<float>"},
        {"jet_sophonAK4_probL", "vector<float>"},
        
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
        
        // GenParticle variables
        {"gen_parton_fromhh", "vector<int>"},
        {"gen_parton_px", "vector<float>"},
        {"gen_parton_py", "vector<float>"},
        {"gen_parton_pz", "vector<float>"},
        {"gen_parton_energy", "vector<float>"},
        {"gen_parton_mass", "vector<float>"},
        {"gen_parton_pt", "vector<float>"},
        {"gen_parton_eta", "vector<float>"},
        {"gen_parton_phi", "vector<float>"},
        {"gen_parton_charge", "vector<int>"},
        {"gen_parton_pid", "vector<int>"}
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
    std::cerr << "** Model path: " << modelPath << std::endl;
    std::cerr << "** Jet branch: " << jetBranch << std::endl;
    std::cerr << "** Total events: " << allEntries << std::endl;

    // set branches
    TClonesArray *branchVertex = treeReader->UseBranch("Vertex");
    TClonesArray *branchParticle = treeReader->UseBranch("Particle");
    TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
    TClonesArray *branchJet = treeReader->UseBranch(jetBranch);

    double jetR = 0.4;
    std::cerr << "jetR = " << jetR << std::endl;
    
    // Initialize SophonAK4 helper
    OrtHelperSophonAK4 *sp4helper = nullptr;
    bool use_sophon = !modelPath.IsNull() && modelPath != "";
    
    if (use_sophon) {
        std::cerr << "Initializing SophonAK4 model..." << std::endl;
        sp4helper = new OrtHelperSophonAK4(modelPath.Data(), false); // debug=false
        std::cerr << "SophonAK4 model initialized." << std::endl;
    } else {
        std::cerr << "No SophonAK4 model provided, will not use jet tagging." << std::endl;
    }

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

        // event selection
        bool pass_selection = false;
        float HT = 0;
        
        // find jets that pass selection criteria for HT calculation and event selection
        std::vector<std::pair<float, int>> selected_jets;
        for(Int_t idx_jet = 0; idx_jet < branchJet->GetEntriesFast(); ++idx_jet) {
            const Jet *jet = (Jet*) branchJet->At(idx_jet);
            if (jet->PT > 30 && std::abs(jet->Eta) < 2.5) {
                selected_jets.emplace_back(jet->PT, idx_jet);
                HT += jet->PT;
            }
        }
        
        // sort by pt
        std::sort(selected_jets.begin(), selected_jets.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });

        // check njets for event selection
        if (selected_jets.size() < 4) {
            tree->Fill();
            continue;
        }

        // Check if event passes selection criteria
        const Jet *jet1 = (Jet*)branchJet->At(selected_jets[0].second);
        const Jet *jet2 = (Jet*)branchJet->At(selected_jets[1].second);
        const Jet *jet3 = (Jet*)branchJet->At(selected_jets[2].second);
        const Jet *jet4 = (Jet*)branchJet->At(selected_jets[3].second);

        if ((jet1->PT > 75 && std::abs(jet1->Eta) < 2.5) && 
            (jet2->PT > 60 && std::abs(jet2->Eta) < 2.5) && 
            (jet3->PT > 45 && std::abs(jet3->Eta) < 2.5) && 
            (jet4->PT > 40 && std::abs(jet4->Eta) < 2.5) && 
            (HT > 330)) {
            pass_selection = true;
            data.floatVars["pass_selection"] = 1;
            data.floatVars["HT"] = HT;
        }

        // Create mapping for particles to jets
        std::map<const TObject*, int> objectToIndexMap;
        
        // Get primary vertex for SophonAK4
        const Vertex *pv = (branchVertex != nullptr) ? ((Vertex *)branchVertex->At(0)) : nullptr;
        
        // Process ALL jets, regardless of selection
        for(Int_t idx_jet = 0; idx_jet < branchJet->GetEntriesFast(); ++idx_jet) {
            const Jet *jet = (Jet*) branchJet->At(idx_jet);
            processJet(jet, data, sp4helper, pv);
            
            // Record jet components for particle labeling
            for (Int_t j = 0; j < jet->Constituents.GetEntriesFast(); ++j) {
                const TObject *object = jet->Constituents.At(j);
                if (object && object->IsA() == ParticleFlowCandidate::Class()) {
                    // Find the position of this jet in the selected_jets list
                    int jetIndex = -1;
                    for (size_t k = 0; k < selected_jets.size(); ++k) {
                        if (selected_jets[k].second == idx_jet) {
                            jetIndex = k + 1;
                            break;
                        }
                    }
                    objectToIndexMap[object] = jetIndex; // Will be -1 if not in selected jets
                }
            }
        }

        if (!pass_selection) {
            tree->Fill();
            continue;
        }

        // PF candidates
        TLorentzVector pfcand_sum(0, 0, 0, 0);
        double pfcand_sum_HT = 0.0;
        
        for(int i = 0; i < branchPFCand->GetEntriesFast(); ++i) {
            const ParticleFlowCandidate *pfcand = (ParticleFlowCandidate*)branchPFCand->At(i);
            int part_label = objectToIndexMap[pfcand] ? objectToIndexMap[pfcand] : -1;
            
            if (std::abs(pfcand->Eta) > 5 || pfcand->PT <= 0) {
                continue;
            }

            TLorentzVector pfcand_p4;
            pfcand_p4.SetPtEtaPhiM(pfcand->PT, pfcand->Eta, pfcand->Phi, pfcand->Mass);
            pfcand_sum += pfcand_p4;
            pfcand_sum_HT += pfcand->PT;

            processParticle(pfcand, data, pv, part_label);
        }

        data.floatVars["pfcand_sum_mass"] = pfcand_sum.M();
        data.floatVars["pfcand_sum_HT"] = pfcand_sum_HT;

        // GenParticles
        int higgs_count = 0;
        TLorentzVector higgs1_p4, higgs2_p4;
        
        for(int i = 0; i < branchParticle->GetEntriesFast(); ++i) {
            const GenParticle *genparticle = (GenParticle*)branchParticle->At(i);
            processGenParticle(genparticle, data, branchParticle, higgs_count, higgs1_p4, higgs2_p4);
        }

        tree->Fill();
        ++num_pass_selection;

    } // end event loop

    tree->Write();
    std::cerr << TString::Format("** Written %d events to output %s; %d events passing customized selection", 
                                num_processed, outputFile.Data(), num_pass_selection) << std::endl;

    // Clean up
    if (sp4helper) {
        delete sp4helper;
    }
    
    delete treeReader;
    delete chain;
    delete fout;
}
