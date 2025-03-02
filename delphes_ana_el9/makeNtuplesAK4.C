#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"

#include "JetMatching.h"
#include "EventData.h"

// #ifdef __CLING__
// R__LOAD_LIBRARY(libDelphes)
// #include "classes/DelphesClasses.h"
// #include "external/ExRootAnalysis/ExRootTreeReader.h"
// #else
// class ExRootTreeReader;
// #endif

void makeNtuplesAK4(TString inputFile, TString outputFile, TString jetBranch = "JetPUPPI", bool debug = false) {
    // gSystem->Load("libDelphes");

    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // define branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        // particle (jet constituent) features
        {"part_px", "vector<float>"},
        {"part_py", "vector<float>"},
        {"part_pz", "vector<float>"},
        {"part_energy", "vector<float>"},
        {"part_deta", "vector<float>"},
        {"part_dphi", "vector<float>"},
        {"part_d0val", "vector<float>"},
        {"part_d0err", "vector<float>"},
        {"part_dzval", "vector<float>"},
        {"part_dzerr", "vector<float>"},
        {"part_charge", "vector<int>"},
        {"part_isElectron", "vector<bool>"},
        {"part_isMuon", "vector<bool>"},
        {"part_isPhoton", "vector<bool>"},
        {"part_isChargedHadron", "vector<bool>"},
        {"part_isNeutralHadron", "vector<bool>"},
        // jet features
        {"jet_pt", "float"},
        {"jet_eta", "float"},
        {"jet_phi", "float"},
        {"jet_energy", "float"},
        {"jet_mass", "float"},
        {"jet_nparticles", "int"},
        {"jet_label", "int"},
        // aux genpart features
        {"aux_genpart_pt", "vector<float>"},
        {"aux_genpart_eta", "vector<float>"},
        {"aux_genpart_phi", "vector<float>"},
        {"aux_genpart_mass", "vector<float>"},
        {"aux_genpart_pid", "vector<int>"},
    };
    EventData data(branchList);
    data.setOutputBranch(tree);

    // Read input
    TChain *chain = new TChain("Delphes");
    chain->Add(inputFile);
    ExRootTreeReader *treeReader = new ExRootTreeReader(chain);
    Long64_t allEntries = treeReader->GetEntries();

    std::cerr << "** Input file:    " << inputFile << std::endl;
    std::cerr << "** Jet branch:    " << jetBranch << std::endl;
    std::cerr << "** Total events:  " << allEntries << std::endl;

    // Analyze
    TClonesArray *branchVertex = treeReader->UseBranch("Vertex"); // used for pileup
    TClonesArray *branchParticle = treeReader->UseBranch("Particle");
    TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
    TClonesArray *branchJet = treeReader->UseBranch(jetBranch);

    double jetR = 0.2;
    std::cerr << "jetR = " << jetR << std::endl;

    JetMatching ak4match(jetR, debug);

    // Loop over all events
    int num_processed = 0;
    for (Long64_t entry = 0; entry < allEntries; ++entry) {
        if (entry % 1000 == 0) {
            std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
            std::cerr << "processed " << num_processed << " jets." << std::endl;
        }
        // std::cout << "== New events ==" << std::endl;

        // Load selected branches with data from specified event
        treeReader->ReadEntry(entry);

        // Loop over all jets in event
        std::vector<int> genjet_used_inds = {};
        for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
            const Jet *jet = (Jet *)branchJet->At(i);

            data.reset();

            // Get the GEN label
            ak4match.getLabel(jet, branchParticle);
            auto ak4label = ak4match.getResult().label;

            if (ak4label == "Invalid")
                continue;

            if (debug) {
                std::cerr << ">> debug ak4label     : " << ak4label << "  " << std::endl;
                std::cerr << "   resParticles      : "; for (auto& p: ak4match.getResult().resParticles) {std::cerr << p->PID << " ";} std::cerr << std::endl;
                std::cerr << "   decayParticles    : "; for (auto& p: ak4match.getResult().decayParticles) {std::cerr << p->PID << " ";} std::cerr << std::endl;
                std::cerr << "   tauDecayParticles : "; for (auto& p: ak4match.getResult().tauDecayParticles) {std::cerr << p->PID << " ";} std::cerr << std::endl;
                std::cerr << "   qcdPartons        : "; for (auto& p: ak4match.getResult().qcdPartons) {std::cerr << p->PID << " ";} std::cerr << std::endl;
            }

            // GEN label and original particles (as auxiliary vars)
            data.intVars.at("jet_label") = ak4match.findLabelIndex();

            auto fillAuxVars = [](EventData& data, const auto& part) {
                data.vfloatVars.at("aux_genpart_pt")->push_back(part->PT);
                data.vfloatVars.at("aux_genpart_eta")->push_back(part->Eta);
                data.vfloatVars.at("aux_genpart_phi")->push_back(part->Phi);
                data.vfloatVars.at("aux_genpart_mass")->push_back(part->Mass);
                data.vintVars.at("aux_genpart_pid")->push_back(part->PID);
            };
            for (const auto &p : ak4match.getResult().resParticles) {
                fillAuxVars(data, p);
            }

            // Jet features
            data.floatVars.at("jet_pt") = jet->PT;
            data.floatVars.at("jet_eta") = jet->Eta;
            data.floatVars.at("jet_phi") = jet->Phi;
            data.floatVars.at("jet_energy") = jet->P4().Energy();
            data.floatVars.at("jet_mass") = jet->Mass;

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

            // sort particles by pt
            std::sort(particles.begin(), particles.end(), [](const auto &a, const auto &b) { return a.pt > b.pt; });

            // Load the primary vertex
            const Vertex *pv = (branchVertex != nullptr) ? ((Vertex *)branchVertex->At(0)) : nullptr;

            data.intVars["jet_nparticles"] = particles.size();
            for (const auto &p : particles) {
                data.vfloatVars.at("part_px")->push_back(p.px);
                data.vfloatVars.at("part_py")->push_back(p.py);
                data.vfloatVars.at("part_pz")->push_back(p.pz);
                data.vfloatVars.at("part_energy")->push_back(p.energy);
                data.vfloatVars.at("part_deta")->push_back((jet->Eta > 0 ? 1 : -1) * (p.eta - jet->Eta));
                data.vfloatVars.at("part_dphi")->push_back(deltaPhi(p.phi, jet->Phi));
                data.vfloatVars.at("part_d0val")->push_back(p.d0);
                data.vfloatVars.at("part_d0err")->push_back(p.d0err);
                data.vfloatVars.at("part_dzval")->push_back((pv && p.dz != 0) ? (p.dz - pv->Z) : p.dz);
                data.vfloatVars.at("part_dzerr")->push_back(p.dzerr);
                data.vintVars.at("part_charge")->push_back(p.charge);
                data.vboolVars.at("part_isElectron")->push_back(p.pid == 11 || p.pid == -11);
                data.vboolVars.at("part_isMuon")->push_back(p.pid == 13 || p.pid == -13);
                data.vboolVars.at("part_isPhoton")->push_back(p.pid == 22);
                data.vboolVars.at("part_isChargedHadron")->push_back(p.charge != 0 && !(p.pid == 11 || p.pid == -11 || p.pid == 13 || p.pid == -13));
                data.vboolVars.at("part_isNeutralHadron")->push_back(p.charge == 0 && !(p.pid == 22));
            }

            tree->Fill();
            ++num_processed;
        } // end loop of jets
    } // end loop of events

    tree->Write();
    std::cerr << TString::Format("** Written %d jets to output %s", num_processed, outputFile.Data()) << std::endl;

    delete treeReader;
    delete chain;
    delete fout;
}

//------------------------------------------------------------------------------
