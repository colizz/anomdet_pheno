#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"

#include "JetMatching.h"
#include "EventData.h"

#include "OrtHelperSophon.h"

// #ifdef __CLING__
// R__LOAD_LIBRARY(libDelphes)
// #include "classes/DelphesClasses.h"
// #include "external/ExRootAnalysis/ExRootTreeReader.h"
// #else
// class ExRootTreeReader;
// #endif

const GenParticle *getFinal(const GenParticle *particle, std::vector<const GenParticle *>& genParticles) {
  // will mark intermediate particles as processed
  if (!particle)
    return nullptr;
  const GenParticle *final = particle;

  while (final->D1 >= 0) {
    const GenParticle *chain = nullptr;
    for (int idau = final->D1; idau <= final->D2; ++idau) {
      if (genParticles.at(idau)->PID == particle->PID) {
        chain = genParticles.at(idau);
        break;
      }
    }
    if (!chain)
      break;
    final = chain;
  }
  return final;
}

std::vector<const GenParticle *> getDaughters(const GenParticle *particle, std::vector<const GenParticle *>& genParticles) {
  std::vector<const GenParticle *> daughters;
  for (int idau = particle->D1; idau <= particle->D2; ++idau) {
    daughters.push_back(genParticles.at(idau));
  }
  return daughters;
}

//------------------------------------------------------------------------------

void makeNtuplesEvalSophonFatJet(TString inputFile, TString outputFile, TString modelPathFatJet, TString fatJetBranch = "JetPUPPIAK8", bool debug = false) {
    // gSystem->Load("libDelphes");

    TFile *fout = new TFile(outputFile, "RECREATE");
    TTree *tree = new TTree("tree", "tree");

    // define branches
    std::vector<std::pair<std::string, std::string>> branchList = {
        // GEN-matching features
        {"part_h_pt", "float"},
        {"part_h_eta", "float"},
        {"part_h_phi", "float"},
        {"jet_dr_hdau1", "float"},
        {"jet_dr_hdau2", "float"},
        // jet features
        {"jet_pt", "float"},
        {"jet_eta", "float"},
        {"jet_phi", "float"},
        {"jet_energy", "float"},
        {"jet_sdmass", "float"},
        {"jet_tau1", "float"},
        {"jet_tau2", "float"},
        {"jet_tau3", "float"},
        {"jet_tau4", "float"},
        {"jet_nparticles", "int"},
        {"jet_probs", "vector<float>"}
    };
    EventData data(branchList);
    data.setOutputBranch(tree);

    // Read input
    TChain *chain = new TChain("Delphes");
    chain->Add(inputFile);
    ExRootTreeReader *treeReader = new ExRootTreeReader(chain);
    Long64_t allEntries = treeReader->GetEntries();

    std::cerr << "** Input file:    " << inputFile << std::endl;
    std::cerr << "** FatJet branch: " << fatJetBranch << std::endl;
    std::cerr << "** Total events:  " << allEntries << std::endl;

    // Analyze
    TClonesArray *branchVertex = treeReader->UseBranch("Vertex"); // used for pileup
    TClonesArray *branchParticle = treeReader->UseBranch("Particle");
    TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
    TClonesArray *branchJet = treeReader->UseBranch(fatJetBranch);

    double fatJetR = fatJetBranch.Contains("AK15") ? 1.5 : 0.8;
    std::cerr << "fatJetR = " << fatJetR << std::endl;

    // Initialize onnx helper
    auto orthelper = OrtHelperSophon(modelPathFatJet.Data(), debug);

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

        // find the Higgs boson
        std::vector<const GenParticle *> genParticles;
        std::vector<const GenParticle *> target;
        for (Int_t i = 0; i < branchParticle->GetEntriesFast(); ++i) {
            genParticles.push_back((GenParticle *)branchParticle->At(i));
        }
        for (const auto *gp : genParticles) {
            int abspid = std::abs(gp->PID);
            if (abspid == 25 || abspid == 37) {
                // std::cerr << "Higgs boson found with pT = " << gp->PT << ", eta = " << gp->Eta << ", phi = " << gp->Phi << std::endl;
                target.push_back(getFinal(gp, genParticles));
                break;
            }
        }
        if (target.empty()) {
            std::cerr << "No Higgs boson found in event " << entry << std::endl;
            continue;
        }
        const auto *higgs = target.at(0);
        // get daughters of the Higgs boson
        auto hdaus = getDaughters(higgs, genParticles);

        // Loop over all jets in event
        std::vector<int> genjet_used_inds = {};
        for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
            const Jet *jet = (Jet *)branchJet->At(i);

            // must match with H->qq
            if (!(deltaR(jet, hdaus.at(0)) < fatJetR && deltaR(jet, hdaus.at(1)) < fatJetR))
                continue;

            data.reset();

            // GEN-matching features
            data.floatVars.at("part_h_pt") = higgs->PT;
            data.floatVars.at("part_h_eta") = higgs->Eta;
            data.floatVars.at("part_h_phi") = higgs->Phi;
            data.floatVars.at("jet_dr_hdau1") = deltaR(jet, hdaus.at(0));
            data.floatVars.at("jet_dr_hdau2") = deltaR(jet, hdaus.at(1));

            // Jet features
            data.floatVars.at("jet_pt") = jet->PT;
            data.floatVars.at("jet_eta") = jet->Eta;
            data.floatVars.at("jet_phi") = jet->Phi;
            data.floatVars.at("jet_energy") = jet->P4().Energy();

            data.floatVars.at("jet_sdmass") = jet->SoftDroppedP4[0].M();
            data.floatVars.at("jet_tau1") = jet->Tau[0];
            data.floatVars.at("jet_tau2") = jet->Tau[1];
            data.floatVars.at("jet_tau3") = jet->Tau[2];
            data.floatVars.at("jet_tau4") = jet->Tau[3];

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

            // Construct particleVars and jetVars
            std::map<std::string, std::vector<float>> particleVars;
            std::map<std::string, float> jetVars;

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

            // Infer the Sophon model
            orthelper.infer_model(particleVars, jetVars);
            const auto &output = orthelper.get_output();

            // Get inference output
            for (size_t i = 0; i < 188; i++) {
                data.vfloatVars.at("jet_probs")->push_back(output[i]);
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
