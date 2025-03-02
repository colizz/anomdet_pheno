#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
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

//------------------------------------------------------------------------------

void makeNtuplesWcbResolvedAna(TString inputFile, TString outputFile, TString fatJetBranch, TString modelPathAK4, TString modelPathFJ, bool debug = false) {
    // gSystem->Load("libDelphes");

    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // define branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        // AK4 jet features
        {"jet_px", "vector<float>"},
        {"jet_py", "vector<float>"},
        {"jet_pz", "vector<float>"},
        {"jet_energy", "vector<float>"},
        {"jet_pt", "vector<float>"},
        {"jet_eta", "vector<float>"},
        {"jet_phi", "vector<float>"},
        {"jet_flavor", "vector<int>"},
        {"jet_sophonAK4_probB", "vector<float>"},
        {"jet_sophonAK4_probC", "vector<float>"},
        {"jet_sophonAK4_probL", "vector<float>"},
        // pass fatjet criteria
        {"pass_fj_selection", "bool"},
        {"pass_fjak15_selection", "bool"},
        // lepton feature
        {"lep_px", "float"},
        {"lep_py", "float"},
        {"lep_pz", "float"},
        {"lep_energy", "float"},
        {"lep_pt", "float"},
        {"lep_eta", "float"},
        {"lep_phi", "float"},
        {"lep_charge", "int"},
        {"lep_pid", "int"}, // determines the channel (ele/muon)
        {"lep_iso", "float"},
        {"n_el", "int"},
        {"n_mu", "int"},
        // MET features
        {"met_pt", "float"},
        {"met_phi", "float"},
        // GEN level info
        {"genpart_pt", "vector<float>"},
        {"genpart_eta", "vector<float>"},
        {"genpart_phi", "vector<float>"},
        {"genpart_energy", "vector<float>"},
        {"genpart_pid", "vector<int>"},
        {"is_wcb", "bool"},
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
    TClonesArray *branchFatJet = treeReader->UseBranch("JetPUPPIAK8");
    TClonesArray *branchFatJetAK15 = treeReader->UseBranch("JetPUPPIAK15");
    TClonesArray *branchMuon = treeReader->UseBranch("Muon");
    TClonesArray *branchElectron = treeReader->UseBranch("Electron");
    TClonesArray *branchMET = treeReader->UseBranch("PuppiMissingET");

    // Initialize onnx helper for Sophon AK4 and AK8 models
    auto sp4helper = OrtHelperSophonAK4(modelPathAK4.Data(), debug);
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
        // Selections for Wcb analysis
        //  - triggered ele/mu (already done in Delphes)
        //  - >=4 AK4 jets with pT > 20 GeV, and non-overlapping with lepton: dR(AK4, lep) > 0.4
        bool pass_selection = false;

        // First triggered by muons
        bool pass_trigger = false;
        TLorentzVector lep_p4;
        if (!pass_trigger) {
            for (int i = 0; i < branchMuon->GetEntries(); ++i) {
                const Muon *muon = (Muon *)branchMuon->At(i);
                if (muon->PT > 24 && std::abs(muon->Eta) < 2.4 && muon->IsolationVar < 0.15) {
                    if (!pass_trigger) {
                        lep_p4 = muon->P4();
                        data.floatVars.at("lep_px") = lep_p4.Px();
                        data.floatVars.at("lep_py") = lep_p4.Py();
                        data.floatVars.at("lep_pz") = lep_p4.Pz();
                        data.floatVars.at("lep_energy") = lep_p4.Energy();
                        data.floatVars.at("lep_pt") = muon->PT;
                        data.floatVars.at("lep_eta") = muon->Eta;
                        data.floatVars.at("lep_phi") = muon->Phi;
                        data.intVars.at("lep_charge") = muon->Charge;
                        data.intVars.at("lep_pid") = -13 * muon->Charge;
                        data.floatVars.at("lep_iso") = muon->IsolationVar;
                        pass_trigger = true;
                    }
                    data.intVars.at("n_el")++;
                }
            }
        }
        // Then triggered by electrons
        if (!pass_trigger) {
            for (int i = 0; i < branchElectron->GetEntries(); ++i) {
                const Electron *electron = (Electron *)branchElectron->At(i);
                if (electron->PT > 32 && std::abs(electron->Eta) < 2.5 && electron->IsolationVar < 0.1) {
                    if (!pass_trigger) {
                        lep_p4 = electron->P4();
                        data.floatVars.at("lep_px") = lep_p4.Px();
                        data.floatVars.at("lep_py") = lep_p4.Py();
                        data.floatVars.at("lep_pz") = lep_p4.Pz();
                        data.floatVars.at("lep_energy") = lep_p4.Energy();
                        data.floatVars.at("lep_pt") = electron->PT;
                        data.floatVars.at("lep_eta") = electron->Eta;
                        data.floatVars.at("lep_phi") = electron->Phi;
                        data.intVars.at("lep_charge") = electron->Charge;
                        data.intVars.at("lep_pid") = -11 * electron->Charge;
                        data.floatVars.at("lep_iso") = electron->IsolationVar;
                        pass_trigger = true;
                    }
                    data.intVars.at("n_mu")++;
                }
            }
        }
        if (!pass_trigger) {
            cerr << "No triggered lepton found in event " << entry << "... This should never happen." << endl;
            continue;
        }
        ++num_processed;


        // Load the primary vertex
        const Vertex *pv = (branchVertex != nullptr) ? ((Vertex *)branchVertex->At(0)) : nullptr;

        // Apply AK4 selection
        int n_jets_qualified = 0;
        for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
            const Jet *jet = (Jet *)branchJet->At(i);

            if (jet->PT >= 20 && std::abs(jet->Eta) < 2.5) {
                TLorentzVector jet_p4 = jet->P4();
                if (jet_p4.DeltaR(lep_p4) > 0.4) {
                    ++n_jets_qualified;
                }
            }
        }
        pass_selection = (n_jets_qualified >= 4);

        // if not passing event selection, fill empty event!
        if (!pass_selection) {
            tree->Fill();
            continue;
        }

        // check if fatjet exists
        for (Int_t i = 0; i < branchFatJet->GetEntriesFast(); ++i) {
            const Jet *fj = (Jet *)branchFatJet->At(i);
            if (fj->PT >= 200 && std::abs(fj->Eta) < 2.5) {
                TLorentzVector fj_p4 = fj->P4();
                if (fj_p4.DeltaR(lep_p4) > 0.8) {
                    data.boolVars.at("pass_fj_selection") = true;
                    break;
                }
            }
        }
        for (Int_t i = 0; i < branchFatJetAK15->GetEntriesFast(); ++i) {
            const Jet *fj = (Jet *)branchFatJetAK15->At(i);
            if (fj->PT >= 120 && std::abs(fj->Eta) < 2.5) {
                TLorentzVector fj_p4 = fj->P4();
                if (fj_p4.DeltaR(lep_p4) > 1.5) {
                    data.boolVars.at("pass_fjak15_selection") = true;
                    break;
                }
            }
        }

        // Loop over all jets in event
        for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
            const Jet *jet = (Jet *)branchJet->At(i);
            TLorentzVector jet_p4 = jet->P4();

            data.vfloatVars.at("jet_px")->push_back(jet_p4.Px());
            data.vfloatVars.at("jet_py")->push_back(jet_p4.Py());
            data.vfloatVars.at("jet_pz")->push_back(jet_p4.Pz());
            data.vfloatVars.at("jet_energy")->push_back(jet_p4.Energy());
            data.vfloatVars.at("jet_pt")->push_back(jet->PT);
            data.vfloatVars.at("jet_eta")->push_back(jet->Eta);
            data.vfloatVars.at("jet_phi")->push_back(jet->Phi);
            data.vintVars.at("jet_flavor")->push_back(jet->Flavor);

            // infer SophonAK4 model
            std::map<std::string, std::vector<float>> particleVars;
            std::map<std::string, float> jetVars;
            makeSophonInput(jet, pv, particleVars, jetVars);
            sp4helper.infer_model(particleVars, jetVars);
            const auto &sp4output = sp4helper.get_output();

            // fill SophonAK4 scores
            data.vfloatVars.at("jet_sophonAK4_probB")->push_back(sp4output[0] + sp4output[1] + sp4output[17]);
            data.vfloatVars.at("jet_sophonAK4_probC")->push_back(sp4output[2] + sp4output[3] + sp4output[18]);
            data.vfloatVars.at("jet_sophonAK4_probL")->push_back(
                std::accumulate(sp4output.begin() + 4, sp4output.begin() + 11, 0.0) + \
                std::accumulate(sp4output.begin() + 19, sp4output.begin() + 23, 0.0)
                );
        }

        // MET
        const MissingET *met = (MissingET *)branchMET->At(0);
        data.floatVars.at("met_pt") = met->MET;
        data.floatVars.at("met_phi") = met->Phi;

        // Gen level info
        genhelper.process("wcbana", branchParticle);

        data.vfloatVars.at("genpart_pt")->insert(data.vfloatVars.at("genpart_pt")->end(), genhelper.getData().pt.begin(), genhelper.getData().pt.end());
        data.vfloatVars.at("genpart_eta")->insert(data.vfloatVars.at("genpart_eta")->end(), genhelper.getData().eta.begin(), genhelper.getData().eta.end());
        data.vfloatVars.at("genpart_phi")->insert(data.vfloatVars.at("genpart_phi")->end(), genhelper.getData().phi.begin(), genhelper.getData().phi.end());
        data.vfloatVars.at("genpart_energy")->insert(data.vfloatVars.at("genpart_energy")->end(), genhelper.getData().energy.begin(), genhelper.getData().energy.end());
        data.vintVars.at("genpart_pid")->insert(data.vintVars.at("genpart_pid")->end(), genhelper.getData().pid.begin(), genhelper.getData().pid.end());
        data.boolVars.at("is_wcb") = genhelper.getData().user_index == 1;

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
