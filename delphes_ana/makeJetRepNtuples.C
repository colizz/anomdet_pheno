#include <iostream>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "classes/DelphesClasses.h"
#include "external/ExRootAnalysis/ExRootTreeReader.h"

#include "FatJetMatching.h"
#include "OrtHelper.h"

// #ifdef __CLING__
// R__LOAD_LIBRARY(libDelphes)
// #include "classes/DelphesClasses.h"
// #include "external/ExRootAnalysis/ExRootTreeReader.h"
// #else
// class ExRootTreeReader;
// #endif

//------------------------------------------------------------------------------

void makeJetRepNtuples(TString inputFile, TString outputFile, TString modelPath, TString jetBranch = "JetPUPPIAK8") {
  // gSystem->Load("libDelphes");

  TFile *fout = new TFile(outputFile, "RECREATE");
  TTree *tree = new TTree("tree", "tree");

  // define branches
  std::map<std::string, float> floatVars;

  floatVars["jet_label"] = 0;

  floatVars["jet_pt"] = 0;
  floatVars["jet_eta"] = 0;
  floatVars["jet_phi"] = 0;
  floatVars["jet_energy"] = 0;
  floatVars["jet_nparticles"] = 0;
  floatVars["jet_sdmass"] = 0;
  floatVars["jet_trmass"] = 0;
  floatVars["jet_tau1"] = 0;
  floatVars["jet_tau2"] = 0;
  floatVars["jet_tau3"] = 0;
  floatVars["jet_tau4"] = 0;

  std::map<std::string, std::vector<float>> arrayVars;

  arrayVars["part_px"];
  arrayVars["part_py"];
  arrayVars["part_pz"];
  arrayVars["part_energy"];
  arrayVars["part_pt"];
  arrayVars["part_deta"];
  arrayVars["part_dphi"];
  arrayVars["part_charge"];
  arrayVars["part_pid"];
  arrayVars["part_d0val"];
  arrayVars["part_d0err"];
  arrayVars["part_dzval"];
  arrayVars["part_dzerr"];

  arrayVars["jet_probs"];
  arrayVars["jet_hidneurons"];

  // book
  for (auto &v : floatVars) {
    tree->Branch(v.first.c_str(), &v.second);
  }

  for (auto &v : arrayVars) {
    tree->Branch(v.first.c_str(), &v.second, /*bufsize=*/1024000);
  }

  // read input
  TChain *chain = new TChain("Delphes");
  chain->Add(inputFile);
  ExRootTreeReader *treeReader = new ExRootTreeReader(chain);
  Long64_t allEntries = treeReader->GetEntries();

  std::cerr << "** Input file: " << inputFile << std::endl;
  std::cerr << "** Jet branch: " << jetBranch << std::endl;
  std::cerr << "** Total events: " << allEntries << std::endl;

  // analyze
  TClonesArray *branchVertex = treeReader->UseBranch("Vertex"); // used for pileup
  TClonesArray *branchParticle = treeReader->UseBranch("Particle");
  TClonesArray *branchPFCand = treeReader->UseBranch("ParticleFlowCandidate");
  TClonesArray *branchJet = treeReader->UseBranch(jetBranch);

  double jetR = jetBranch.Contains("AK15") ? 1.5 : 0.8;
  std::cerr << "jetR = " << jetR << std::endl;

  // initialize onnx helper
  OrtHelper orthelper = OrtHelper(modelPath.Data());

  // Loop over all events
  int num_processed = 0;
  for (Long64_t entry = 0; entry < allEntries; ++entry) {
    if (entry % 100 == 0) {
      std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
    }

    // Load selected branches with data from specified event
    treeReader->ReadEntry(entry);

    // Loop over all jets in event
    // note: only fill the leading jet with pT > 200 and |eta| < 2.5, should satisfy trimmed mass > 50
    bool filled = false;
    for (Int_t i = 0; i < branchJet->GetEntriesFast(); ++i) {
      const Jet *jet = (Jet *)branchJet->At(i);

      if (!(jet->PT > 200 && std::abs(jet->Eta) < 2.5 && jet->TrimmedP4[0].M() > 50))
        continue;

      if (filled)
        continue;

      filled = true;

      for (auto &v : floatVars) {
        v.second = 0;
      }
      for (auto &v : arrayVars) {
        v.second.clear();
      }

      floatVars["jet_pt"] = jet->PT;
      floatVars["jet_eta"] = jet->Eta;
      floatVars["jet_phi"] = jet->Phi;
      floatVars["jet_energy"] = jet->P4().Energy();

      floatVars["jet_sdmass"] = jet->SoftDroppedP4[0].M();
      floatVars["jet_trmass"] = jet->TrimmedP4[0].M();
      floatVars["jet_tau1"] = jet->Tau[0];
      floatVars["jet_tau2"] = jet->Tau[1];
      floatVars["jet_tau3"] = jet->Tau[2];
      floatVars["jet_tau4"] = jet->Tau[3];

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

      floatVars["jet_nparticles"] = particles.size();
      for (const auto &p : particles) {
        arrayVars["part_px"].push_back(p.px);
        arrayVars["part_py"].push_back(p.py);
        arrayVars["part_pz"].push_back(p.pz);
        arrayVars["part_energy"].push_back(p.energy);
        arrayVars["part_pt"].push_back(p.pt);
        arrayVars["part_deta"].push_back((jet->Eta > 0 ? 1 : -1) * (p.eta - jet->Eta));
        arrayVars["part_dphi"].push_back(deltaPhi(p.phi, jet->Phi));
        arrayVars["part_charge"].push_back(p.charge);
        arrayVars["part_pid"].push_back(p.pid);
        arrayVars["part_d0val"].push_back(p.d0);
        arrayVars["part_d0err"].push_back(p.d0err);
        arrayVars["part_dzval"].push_back((pv && p.dz != 0) ? (p.dz - pv->Z) : p.dz);
        arrayVars["part_dzerr"].push_back(p.dzerr);
      }

      // infer the ParT model
      orthelper.infer_model(arrayVars, floatVars);
      const auto &output = orthelper.get_output();

      for (size_t i = 0; i < 188; i++) {
        arrayVars["jet_probs"].push_back(output[i]);
      }
      for (size_t i = 188; i < output.size(); i++) {
        arrayVars["jet_hidneurons"].push_back(output[i]);
      }

      tree->Fill();
      ++num_processed;
    } // end loop of jets

    if (!filled) {
      std::cerr << "No jet filled for event " << entry << std::endl;
    }
  } // end loop of events

  tree->Write();
  std::cerr << TString::Format("** Written %d jets to output %s", num_processed, outputFile.Data()) << std::endl;

  delete treeReader;
  delete chain;
  delete fout;
}

//------------------------------------------------------------------------------
