/*
 *  Delphes: a framework for fast simulation of a generic collider experiment
 *  Copyright (C) 2012-2014  Universite catholique de Louvain (UCL), Belgium
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <signal.h>

#include "TApplication.h"
#include "TROOT.h"

#include "TDatabasePDG.h"
#include "TFile.h"
#include "TLorentzVector.h"
#include "TObjArray.h"
#include "TParticlePDG.h"
#include "TStopwatch.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesHepMC2Reader.h"
#include "modules/Delphes.h"

#include "ExRootAnalysis/ExRootProgressBar.h"
#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"

// minimal interface to fastjet
#include "fastjet/PseudoJet.hh"
#include "fastjet/ClusterSequence.hh"

using namespace std;

//---------------------------------------------------------------------------

static bool interrupted = false;

void SignalHandler(int sig)
{
  interrupted = true;
}

//---------------------------------------------------------------------------

bool passParticleFilter(TObjArray *stableParticleOutputArray, TObjArray *allParticleOutputArray, TObjArray *partonOutputArray)
// This filter acts before Delphes processing
{
  // cluster genjets from stable particles
  fastjet::Strategy               strategy = fastjet::Best;
  fastjet::RecombinationScheme    recombScheme = fastjet::E_scheme;
  fastjet::JetDefinition         *jetDef = NULL;
  jetDef = new fastjet::JetDefinition(fastjet::antikt_algorithm, 1.5, recombScheme, strategy);
  std::vector<fastjet::PseudoJet> fjInputs;

  for(int i = 0; i < stableParticleOutputArray->GetEntries(); ++i)
  {
    Candidate *candidate = static_cast<Candidate *>(stableParticleOutputArray->At(i));
    int abspid = abs(candidate->PID);
    TLorentzVector momentum = candidate->Momentum;

    // exclude neutrinos
    if (abspid == 12 || abspid == 14 || abspid == 16) continue;

    // store as input to Fastjet
    fjInputs.push_back(fastjet::PseudoJet(momentum.Px(), momentum.Py(), momentum.Pz(), momentum.E()));
  }
  if (fjInputs.size() == 0) {
      cout << "Error: event with no final state particles" << endl;
  }

  // run Fastjet algorithm
  fastjet::ClusterSequence clustSeq(fjInputs, *jetDef);

  // extract inclusive jets sorted by pT (note minimum pT of 15.0 GeV)
  std::vector<fastjet::PseudoJet> inclusiveJets = clustSeq.inclusive_jets(15.0);
  std::vector<fastjet::PseudoJet> sortedJets = sorted_by_pt(inclusiveJets);

  // check if the leading jet has pT > 80
  if (sortedJets.size() == 0 || sortedJets[0].pt() <= 80) {
    return false;
  }

  return true;
}

bool passIsoMu24Filter(Delphes *modularDelphes)
{
  bool pass = false;
  TObjArray *muons = modularDelphes->ImportArray("UniqueObjectFinder/muons");

  for(int i = 0; i < muons->GetEntries(); ++i)
  {
    Candidate *muon = static_cast<Candidate *>(muons->At(i));
    if (muon->Momentum.Pt() > 24 && std::abs(muon->Momentum.Eta()) < 2.4 && muon->IsolationVar < 0.15) {
      pass = true;
      break;
    }
  }
  //std::cout << "// muonpt = " << muon->Momentum.Pt() << std::endl; 
  return pass;
}

bool passIsoEle32Filter(Delphes *modularDelphes)
{
  bool pass = false;
  TObjArray *electrons = modularDelphes->ImportArray("UniqueObjectFinder/electrons");

  for(int i = 0; i < electrons->GetEntries(); ++i)
  {
    Candidate *electron = static_cast<Candidate *>(electrons->At(i));
    if (electron->Momentum.Pt() > 32 && std::abs(electron->Momentum.Eta()) < 2.5 && electron->IsolationVar < 0.1) {
      pass = true;
      break;
    }
  }
  // std::cout << "// elept = " << electron->Momentum.Pt() << std::endl;  
  return pass;
}

bool passFatJetFilter(Delphes *modularDelphes)
{
  bool pass = false;
  TObjArray *fatjets = modularDelphes->ImportArray("JetEnergyScalePUPPIAK15/jets");

  for(int i = 0; i < fatjets->GetEntries(); ++i) {
    Candidate *candidate = static_cast<Candidate *>(fatjets->At(i));
    if (candidate->Momentum.Pt() >= 120 && std::abs(candidate->Momentum.Eta()) < 2.5) {
      pass = true;
      break;
    }
  }
  return pass;
}

bool passDelphesObjectFilter(Delphes *modularDelphes)
// This filter acts after Delphes processing
{
  return ((passIsoEle32Filter(modularDelphes) || passIsoMu24Filter(modularDelphes)) && passFatJetFilter(modularDelphes));
}

int main(int argc, char *argv[])
{
  char appName[] = "DelphesHepMC2";
  stringstream message;
  FILE *inputFile = 0;
  TFile *outputFile = 0;
  TStopwatch readStopWatch, procStopWatch;
  ExRootTreeWriter *treeWriter = 0;
  ExRootTreeBranch *branchEvent = 0, *branchWeight = 0;
  ExRootConfReader *confReader = 0;
  Delphes *modularDelphes = 0;
  DelphesFactory *factory = 0;
  TObjArray *stableParticleOutputArray = 0, *allParticleOutputArray = 0, *partonOutputArray = 0;
  DelphesHepMC2Reader *reader = 0;
  Int_t i, maxEvents, skipEvents;
  Long64_t length, eventCounter, genSelectedCounter, fillCounter;

  if(argc < 3)
  {
    cout << " Usage: " << appName << " config_file"
         << " output_file"
         << " [input_file(s)]" << endl;
    cout << " config_file - configuration file in Tcl format," << endl;
    cout << " output_file - output file in ROOT format," << endl;
    cout << " input_file(s) - input file(s) in HepMC format," << endl;
    cout << " with no input_file, or when input_file is -, read standard input." << endl;
    return 1;
  }

  signal(SIGINT, SignalHandler);

  gROOT->SetBatch();

  int appargc = 1;
  char *appargv[] = {appName};
  TApplication app(appName, &appargc, appargv);

  try
  {
    outputFile = TFile::Open(argv[2], "CREATE");

    if(outputFile == NULL)
    {
      message << "can't create output file " << argv[2];
      throw runtime_error(message.str());
    }

    treeWriter = new ExRootTreeWriter(outputFile, "Delphes");

    branchEvent = treeWriter->NewBranch("Event", HepMCEvent::Class());
    branchWeight = treeWriter->NewBranch("Weight", Weight::Class());

    confReader = new ExRootConfReader;
    confReader->ReadFile(argv[1]);

    maxEvents = confReader->GetInt("::MaxEvents", 0);
    skipEvents = confReader->GetInt("::SkipEvents", 0);

    if(maxEvents < 0)
    {
      throw runtime_error("MaxEvents must be zero or positive");
    }

    if(skipEvents < 0)
    {
      throw runtime_error("SkipEvents must be zero or positive");
    }

    modularDelphes = new Delphes("Delphes");
    modularDelphes->SetConfReader(confReader);
    modularDelphes->SetTreeWriter(treeWriter);

    factory = modularDelphes->GetFactory();
    allParticleOutputArray = modularDelphes->ExportArray("allParticles");
    stableParticleOutputArray = modularDelphes->ExportArray("stableParticles");
    partonOutputArray = modularDelphes->ExportArray("partons");

    reader = new DelphesHepMC2Reader;

    modularDelphes->InitTask();

    i = 3;
    do
    {
      if(interrupted) break;

      if(i == argc || strncmp(argv[i], "-", 2) == 0)
      {
        cout << "** Reading standard input" << endl;
        inputFile = stdin;
        length = -1;
      }
      else
      {
        cout << "** Reading " << argv[i] << endl;
        inputFile = fopen(argv[i], "r");

        if(inputFile == NULL)
        {
          message << "can't open " << argv[i];
          throw runtime_error(message.str());
        }

        fseek(inputFile, 0L, SEEK_END);
        length = ftello(inputFile);
        fseek(inputFile, 0L, SEEK_SET);

        if(length <= 0)
        {
          fclose(inputFile);
          ++i;
          continue;
        }
      }

      reader->SetInputFile(inputFile);

      ExRootProgressBar progressBar(length);

      // Loop over all objects
      eventCounter = 0;
      genSelectedCounter = 0;
      fillCounter = 0;
      treeWriter->Clear();
      modularDelphes->Clear();
      reader->Clear();
      readStopWatch.Start();
      while((maxEvents <= 0 || eventCounter - skipEvents < maxEvents) && reader->ReadBlock(factory, allParticleOutputArray, stableParticleOutputArray, partonOutputArray) && !interrupted)
      {
        if(reader->EventReady())
        {
          ++eventCounter;

          readStopWatch.Stop();

	        if(eventCounter > skipEvents && passParticleFilter(stableParticleOutputArray, allParticleOutputArray, partonOutputArray))
          {
            ++genSelectedCounter;
            procStopWatch.Start();
            modularDelphes->ProcessTask();
            procStopWatch.Stop();

            reader->AnalyzeEvent(branchEvent, eventCounter, &readStopWatch, &procStopWatch);
            reader->AnalyzeWeight(branchWeight);

            if (passDelphesObjectFilter(modularDelphes)) {
              treeWriter->Fill();
              ++fillCounter;
            }

            treeWriter->Clear();
          }

          modularDelphes->Clear();
          reader->Clear();

          readStopWatch.Start();
        }
        progressBar.Update(ftello(inputFile), eventCounter);
      }

      fseek(inputFile, 0L, SEEK_END);
      progressBar.Update(ftello(inputFile), eventCounter, kTRUE);
      progressBar.Finish();

      if(inputFile != stdin) fclose(inputFile);

      ++i;
    } while(i < argc);

    modularDelphes->FinishTask();
    treeWriter->Write();

    cout << "** Exiting..." << endl;
    cout << "** Processed " << eventCounter << " events" << endl;
    cout << "** GEN-selected " << genSelectedCounter << " events" << endl;
    cout << "** Filled " << fillCounter << " events" << endl;

    delete reader;
    delete modularDelphes;
    delete confReader;
    delete treeWriter;
    delete outputFile;

    return 0;
  }
  catch(runtime_error &e)
  {
    if(treeWriter) delete treeWriter;
    if(outputFile) delete outputFile;
    cerr << "** ERROR: " << e.what() << endl;
    return 1;
  }
}
