#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "TString.h"
#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"

#include "GenPartProcessor.h"
#include "EventData.h"

#include "OrtHelperSophonAK4.h"
#include "OrtHelperSophon.h"

// #ifdef __CLING__
// R__LOAD_LIBRARY(libDelphes)
// #include "classes/DelphesClasses.h"
// #include "external/ExRootAnalysis/ExRootTreeReader.h"
// #else
// class ExRootTreeReader;
// #endif

void makeSophonInput(const Jet *jet, const Vertex *pv, std::map<std::string, std::vector<float>> &particleVars, std::map<std::string, float> &jetVars) {

    // Loop over all jet's constituents
    std::vector<ParticleInfo> particles;
    for (Int_t j = 0; j < jet->Constituents.GetEntriesFast(); ++j) {
        const TObject *object = jet->Constituents.At(j);

        // Check if the constituent is accessible
        if (!object)
            continue;

        if (object->IsA() == GenParticle::Class()) {
            particles.emplace_back((GenParticle *)object);
        } else if (object->IsA() == ParticleFlowCandidate::Class()) {
            particles.emplace_back((ParticleFlowCandidate *)object);
        }
        const auto &p = particles.back();
        if (std::abs(p.pz) > 10000 || std::abs(p.eta) > 5 || p.pt <= 0) {
            particles.pop_back();
        }
    }

    // Sort particles by pt
    std::sort(particles.begin(), particles.end(), [](const auto &a, const auto &b) { return a.pt > b.pt; });

    // Fill particleVars and jetVars
    for (const auto &p : particles) {
        particleVars["part_px"].push_back(p.px);
        particleVars["part_py"].push_back(p.py);
        particleVars["part_pz"].push_back(p.pz);
        particleVars["part_energy"].push_back(p.energy);
        particleVars["part_pt"].push_back(p.pt);
        particleVars["part_deta"].push_back((jet->Eta > 0 ? 1 : -1) * (p.eta - jet->Eta));
        particleVars["part_dphi"].push_back(deltaPhi(p.phi, jet->Phi));
        particleVars["part_charge"].push_back(p.charge);
        particleVars["part_pid"].push_back(p.pid);
        particleVars["part_d0val"].push_back(p.d0);
        particleVars["part_d0err"].push_back(p.d0err);
        particleVars["part_dzval"].push_back((pv && p.dz != 0) ? (p.dz - pv->Z) : p.dz);
        particleVars["part_dzerr"].push_back(p.dzerr);
    }
    jetVars["jet_pt"] = jet->PT;
    jetVars["jet_eta"] = jet->Eta;
    jetVars["jet_phi"] = jet->Phi;
    jetVars["jet_energy"] = jet->P4().Energy();

    return;
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

//------------------------------------------------------------------------------

void makeNtuplesHH4bBoosted(TString inputFile, TString outputFile, TString fatJetBranch, TString modelPathAK4, TString modelPathFatJet, bool debug = false, bool require_pass_fj_trigger = false) {
    // gSystem->Load("libDelphes");

    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // define branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        // AK8 jet features
        {"fj_1_px", "float"},
        {"fj_1_py", "float"},
        {"fj_1_pz", "float"},
        {"fj_1_energy", "float"},
        {"fj_1_pt", "float"},
        {"fj_1_eta", "float"},
        {"fj_1_phi", "float"},
        {"fj_1_mass", "float"},
        {"fj_1_sdmass", "float"},
        {"fj_1_trmass", "float"},
        {"fj_1_sophon_probXbb", "float"},
        {"fj_1_sophon_probXcc", "float"},
        {"fj_1_sophon_probXqq", "float"},
        {"fj_1_sophon_probXbc", "float"},
        {"fj_1_sophon_probXcs", "float"},
        {"fj_1_sophon_probXbq", "float"},
        {"fj_1_sophon_probXcq", "float"},
        {"fj_1_sophon_probXbqq", "float"},
        {"fj_1_sophon_probQCD", "float"},

        {"fj_2_px", "float"},
        {"fj_2_py", "float"},
        {"fj_2_pz", "float"},
        {"fj_2_energy", "float"},
        {"fj_2_pt", "float"},
        {"fj_2_eta", "float"},
        {"fj_2_phi", "float"},
        {"fj_2_mass", "float"},
        {"fj_2_sdmass", "float"},
        {"fj_2_trmass", "float"},
        {"fj_2_sophon_probXbb", "float"},
        {"fj_2_sophon_probXcc", "float"},
        {"fj_2_sophon_probXqq", "float"},
        {"fj_2_sophon_probXbc", "float"},
        {"fj_2_sophon_probXcs", "float"},
        {"fj_2_sophon_probXbq", "float"},
        {"fj_2_sophon_probXcq", "float"},
        {"fj_2_sophon_probXbqq", "float"},
        {"fj_2_sophon_probQCD", "float"},

        // GEN level info
        // - Higgs variables
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
        // - gen partons
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
    EventData data(branchList);
    data.setOutputBranch(tree);

    // Read input
    TChain *chain = new TChain("Delphes");
    chain->Add(inputFile);
    ExRootTreeReader *treeReader = new ExRootTreeReader(chain);
    Long64_t allEntries = treeReader->GetEntries();

    std::cerr << "** Input file:    " << inputFile << std::endl;
    std::cerr << "** Total events:  " << allEntries << std::endl;

    // Analyze
    TClonesArray *branchVertex = treeReader->UseBranch("Vertex"); // used for pileup
    TClonesArray *branchParticle = treeReader->UseBranch("Particle");
    TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
    TClonesArray *branchJet = treeReader->UseBranch("JetPUPPI");
    TClonesArray *branchFatJet = treeReader->UseBranch(fatJetBranch);
    TClonesArray *branchMuon = treeReader->UseBranch("Muon");
    TClonesArray *branchElectron = treeReader->UseBranch("Electron");
    TClonesArray *branchMET = treeReader->UseBranch("PuppiMissingET");

    double fatJetR = fatJetBranch.Contains("AK15") ? 1.5 : 0.8;
    std::cerr << "fatJetR = " << fatJetR << std::endl;

    // Initialize onnx helper for Sophon AK4 and AK8 models
    auto sp8helper = OrtHelperSophon(modelPathFatJet.Data(), debug);
    auto genhelper = GenPartProcessor(debug);

    // Loop over all events
    int num_processed = 0;
    int num_pass_selection = 0;
    for (Long64_t entry = 0; entry < allEntries; ++entry) {
        if (entry % 1000 == 0) {
            std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
        }
        // std::cout << "== New events ==" << std::endl;

        // Load selected branches with data from specified event
        treeReader->ReadEntry(entry);

        // Reset output branches
        data.reset();

        // ======= Add selections =======
        // Selections for boosted analysis
        //  - trigger: at least one AK8 jet with pT > 200, |eta| < 2.5, and trimmed mass > 50
        //  - >=2 AK8 jets, with sdmass > 80
        bool pass_selection = false;

        // First triggered by muons
        bool pass_trigger = false;
        for (Int_t i = 0; i < branchFatJet->GetEntriesFast(); ++i) {
            const Jet *fj = (Jet *)branchFatJet->At(i);
            if (fj->PT > 200 && std::abs(fj->Eta) < 2.5 && fj->TrimmedP4[0].M() > 50) {
                pass_trigger = true;
                break;
            }
        }      
        if (!pass_trigger) {
            cerr << "Event " << entry << " found not passing the boosted trigger... This should never happen." << endl;
            continue;
        }
        ++num_processed;


        // Load the primary vertex
        const Vertex *pv = (branchVertex != nullptr) ? ((Vertex *)branchVertex->At(0)) : nullptr;

        // Apply fatjet selection
        int selfj_inds[2] = {0, 0}, n_selfj = 0;
        for (Int_t i = 0; i < branchFatJet->GetEntriesFast(); ++i) {
            const Jet *fj = (Jet *)branchFatJet->At(i);

            if (fj->PT > 200 && std::abs(fj->Eta) < 2.5 && fj->SoftDroppedP4[0].M() > 80 && n_selfj < 2) {
                selfj_inds[n_selfj] = i;
                ++n_selfj;
            }
        }
        if (n_selfj < 2) {
            // not passing the selection
            tree->Fill();
            continue;
        }

        pass_selection = true;
        for (int i = 0; i < 2; ++i) {
            const Jet *fj = (Jet *)branchFatJet->At(selfj_inds[i]);

            TLorentzVector fj_p4 = fj->P4();
            data.floatVars.at(Form("fj_%d_px", i+1)) = fj_p4.Px();
            data.floatVars.at(Form("fj_%d_py", i+1)) = fj_p4.Py();
            data.floatVars.at(Form("fj_%d_pz", i+1)) = fj_p4.Pz();
            data.floatVars.at(Form("fj_%d_energy", i+1)) = fj_p4.Energy();
            data.floatVars.at(Form("fj_%d_pt", i+1)) = fj->PT;
            data.floatVars.at(Form("fj_%d_eta", i+1)) = fj->Eta;
            data.floatVars.at(Form("fj_%d_phi", i+1)) = fj->Phi;
            data.floatVars.at(Form("fj_%d_mass", i+1)) = fj->Mass;
            data.floatVars.at(Form("fj_%d_sdmass", i+1)) = fj->SoftDroppedP4[0].M();
            data.floatVars.at(Form("fj_%d_trmass", i+1)) = fj->TrimmedP4[0].M();

            // infer Sophon model
            std::map<std::string, std::vector<float>> particleVars;
            std::map<std::string, float> jetVars;
            makeSophonInput(fj, pv, particleVars, jetVars);
            sp8helper.infer_model(particleVars, jetVars);
            const auto &spoutput = sp8helper.get_output();

            // fill Sophon scores
            data.floatVars.at(Form("fj_%d_sophon_probXbb", i+1)) = spoutput[0];
            data.floatVars.at(Form("fj_%d_sophon_probXcc", i+1)) = spoutput[1];
            data.floatVars.at(Form("fj_%d_sophon_probXqq", i+1)) = spoutput[3];
            data.floatVars.at(Form("fj_%d_sophon_probXbc", i+1)) = spoutput[4];
            data.floatVars.at(Form("fj_%d_sophon_probXcs", i+1)) = spoutput[5];
            data.floatVars.at(Form("fj_%d_sophon_probXbq", i+1)) = spoutput[6];
            data.floatVars.at(Form("fj_%d_sophon_probXcq", i+1)) = spoutput[7];
            data.floatVars.at(Form("fj_%d_sophon_probXbqq", i+1)) = spoutput[70] + spoutput[127]; // bqq + bcs
            data.floatVars.at(Form("fj_%d_sophon_probQCD", i+1)) = std::accumulate(spoutput.begin() + 161, spoutput.begin() + 188, 0.0);
        }

        // GenParticles
        int higgs_count = 0;
        TLorentzVector higgs1_p4, higgs2_p4;
        
        for(int i = 0; i < branchParticle->GetEntriesFast(); ++i) {
            const GenParticle *genparticle = (GenParticle*)branchParticle->At(i);
            processGenParticle(genparticle, data, branchParticle, higgs_count, higgs1_p4, higgs2_p4);
        }

        // Fill event
        tree->Fill();
        ++num_pass_selection;

    } // end loop of events

    tree->Write();
    std::cerr << TString::Format("** Written %d events to output %s; %d events passing customized selection", num_processed, outputFile.Data(), num_pass_selection)
              << std::endl;

    delete treeReader;
    delete chain;
    delete fout;
}

//------------------------------------------------------------------------------
