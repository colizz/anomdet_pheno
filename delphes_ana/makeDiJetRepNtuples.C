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

void makeDiJetRepNtuples(TString inputFile, TString outputFile, TString modelPath, TString jetBranch = "JetPUPPIAK8") {
  // gSystem->Load("libDelphes");

  TFile *fout = new TFile(outputFile, "RECREATE");
  TTree *tree = new TTree("tree", "tree");

  // define branches
  std::map<std::string, float> floatVars;

  floatVars["pass_selection"] = 0;

  floatVars["jet_1_label"] = 0;
  floatVars["jet_1_pt"] = 0;
  floatVars["jet_1_eta"] = 0;
  floatVars["jet_1_phi"] = 0;
  floatVars["jet_1_energy"] = 0;
  floatVars["jet_1_nparticles"] = 0;
  floatVars["jet_1_sdmass"] = 0;
  floatVars["jet_1_trmass"] = 0;
  floatVars["jet_1_tau1"] = 0;
  floatVars["jet_1_tau2"] = 0;
  floatVars["jet_1_tau3"] = 0;
  floatVars["jet_1_tau4"] = 0;

  floatVars["jet_2_label"] = 0;
  floatVars["jet_2_pt"] = 0;
  floatVars["jet_2_eta"] = 0;
  floatVars["jet_2_phi"] = 0;
  floatVars["jet_2_energy"] = 0;
  floatVars["jet_2_nparticles"] = 0;
  floatVars["jet_2_sdmass"] = 0;
  floatVars["jet_2_trmass"] = 0;
  floatVars["jet_2_tau1"] = 0;
  floatVars["jet_2_tau2"] = 0;
  floatVars["jet_2_tau3"] = 0;
  floatVars["jet_2_tau4"] = 0;

  std::map<std::string, std::vector<float>> arrayVars;

  arrayVars["jet_1_probs"];
  arrayVars["jet_1_hidneurons"];

  arrayVars["jet_2_probs"];
  arrayVars["jet_2_hidneurons"];

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
  int num_pass_selection = 0;
  for (Long64_t entry = 0; entry < allEntries; ++entry) {
    if (entry % 100 == 0) {
      std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
    }

    // Load selected branches with data from specified event
    treeReader->ReadEntry(entry);
  
    // Loop over all jets in event
  
    // note: the delphes event already passes the trigger:
    //  - HT of all AK8 jets > 800
    //  - the leading jet with pT > 200 and |eta| < 2.5 should satisfy trimmed mass > 50
    
    // here is a dummy trigger check
    bool pass_trigger = true;
    if (!pass_trigger) {
      continue;
    }
    ++num_processed;

    // initialize all branches
    for (auto &v : floatVars) {
      v.second = 0;
    }
    for (auto &v : arrayVars) {
      v.second.clear();
    }

    // determine if passing customized selection; if not, store empty event
    // selection: the *leading two jet* has pT > 200 and |eta| < 2.5, and their inv mass > 2000
    bool pass_selection = false;
    if (branchJet->GetEntriesFast() >= 2) {
      const Jet *jet1 = (Jet *)branchJet->At(0);
      const Jet *jet2 = (Jet *)branchJet->At(1);
      if ((jet1->PT > 200 && std::abs(jet1->Eta) < 2.5) && (jet2->PT > 200 && std::abs(jet2->Eta) < 2.5) && ((jet1->P4() + jet2->P4()).M() > 2000)) {
        pass_selection = true;
      }
    }
    if (!pass_selection) {
      // fill empty event
      tree->Fill();
      continue;
    }

    floatVars["pass_selection"] = 1;

    for (Int_t i = 0; i < 2; ++i) {
      const Jet *jet = (Jet *)branchJet->At(i);
      std::string jet_suffix = "jet_" + std::to_string(i + 1);

      floatVars[jet_suffix + "_pt"] = jet->PT;
      floatVars[jet_suffix + "_eta"] = jet->Eta;
      floatVars[jet_suffix + "_phi"] = jet->Phi;
      floatVars[jet_suffix + "_energy"] = jet->P4().Energy();

      floatVars[jet_suffix + "_sdmass"] = jet->SoftDroppedP4[0].M();
      floatVars[jet_suffix + "_trmass"] = jet->TrimmedP4[0].M();
      floatVars[jet_suffix + "_tau1"] = jet->Tau[0];
      floatVars[jet_suffix + "_tau2"] = jet->Tau[1];
      floatVars[jet_suffix + "_tau3"] = jet->Tau[2];
      floatVars[jet_suffix + "_tau4"] = jet->Tau[3];

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

      floatVars[jet_suffix + "_nparticles"] = particles.size();

      // initialize input to be sent to the inference engine
      std::map<std::string, float> jetVars = {
          {"jet_pt", jet->PT},
          {"jet_eta", jet->Eta},
          {"jet_phi", jet->Phi},
          {"jet_energy", jet->P4().Energy()},
      };
      std::map<std::string, std::vector<float>> particleVars;
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

      // infer the ParT model
      orthelper.infer_model(particleVars, jetVars);
      const auto &output = orthelper.get_output();

      for (size_t i = 0; i < 188; i++) {
        arrayVars[jet_suffix + "_probs"].push_back(output[i]);
      }
      for (size_t i = 188; i < output.size(); i++) {
        arrayVars[jet_suffix + "_hidneurons"].push_back(output[i]);
      }

    } // end loop of jets

    // fill event
    tree->Fill();
    ++num_pass_selection;

  } // end loop of events

  tree->Write();
  std::cerr << TString::Format("** Written %d events to output %s; %d events passing customized selection", num_processed, outputFile.Data(), num_pass_selection) << std::endl;

  delete treeReader;
  delete chain;
  delete fout;
}

//------------------------------------------------------------------------------
