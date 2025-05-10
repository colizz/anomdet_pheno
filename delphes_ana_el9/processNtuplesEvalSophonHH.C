#include <iostream>
#include <unordered_set>
#include <utility>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>

#include "EventData.h"
#include "OrtHelperSophonHH.h"

// #ifdef __CLING__
// R__LOAD_LIBRARY(libDelphes)
// #include "classes/DelphesClasses.h"
// #include "external/ExRootAnalysis/ExRootTreeReader.h"
// #else
// class ExRootTreeReader;
// #endif


void openFile(const std::string& filePath, TFile*& file, TTree*& tree, EventData& data) {
    file = TFile::Open(filePath.c_str(), "READ");
    if (!file || file->IsZombie()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    tree = nullptr;
    file->GetObject("tree", tree);
    if (!tree) {
        file->Close();
        throw std::runtime_error("Failed to get TTree from file: " + filePath);
    }
    data.setBranchAddresses(tree);
}


void closeFile(TFile*& file) {
    file->Close();
    file = nullptr;
}


void createOutputFile(const std::string& filePath, TFile*& file, TTree*& tree, EventData& data) {
    file = TFile::Open(filePath.c_str(), "RECREATE");
    if (!file || file->IsZombie()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    // prepare the file with LZ4 compression for fast loading
    file->SetCompressionAlgorithm(ROOT::kLZ4);
    file->SetCompressionLevel(4);

    tree = new TTree("tree", "tree");
    data.setOutputBranch(tree);
}


void writeOutputFile(TFile*& file) {
    file->Write();
    file->Close();
    file = nullptr;
}

//------------------------------------------------------------------------------
// This macro aims to process ntuples to ntuples with the SophonHH model inferenced

void processNtuplesEvalSophonHH(TString inputFile, TString outputFile, TString modelPath, bool debug = false) {
    // gSystem->Load("libDelphes");

    // define branches
    std::vector<std::pair<std::string, std::string>> branchListIn = {
        {"pass_selection", "int"},
        {"pass_4j3b_selection", "int"},
        {"pass_4j2b_selection", "int"},
        {"pass_boosted_trigger", "int"},
        {"HT", "float"},
        {"pfcand_sum_mass", "float"},
        {"pfcand_sum_HT", "float"},
        {"pfcand_sum_pt", "float"},
        {"pfcand_sum_eta", "float"},
        {"pfcand_sum_phi", "float"},
        {"pfcand_sum_energy", "float"},
        
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

        // Particle variables
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
                
        // GenParticle variables
        {"gen_bhadron_fromhh", "vector<int>"},
        {"gen_bhadron_px", "vector<float>"},
        {"gen_bhadron_py", "vector<float>"},
        {"gen_bhadron_pz", "vector<float>"},
        {"gen_bhadron_energy", "vector<float>"},
        {"gen_bhadron_mass", "vector<float>"},
        {"gen_bhadron_pt", "vector<float>"},
        {"gen_bhadron_eta", "vector<float>"},
        {"gen_bhadron_phi", "vector<float>"},
        {"gen_bhadron_charge", "vector<int>"},
        {"gen_bhadron_pid", "vector<int>"},

        // Gen-level info
        {"genpart_pt", "vector<float>"},
        {"genpart_eta", "vector<float>"},
        {"genpart_phi", "vector<float>"},
        {"genpart_energy", "vector<float>"},
        {"genpart_pid", "vector<int>"},
        {"process_index", "int"},
        {"gen_weight", "vector<float>"},
    };
    std::vector<std::pair<std::string, std::string>> branchListOut = {
        // Particle variables
        {"event_sophonHH_probs", "vector<float>"},

        // Copy other branches as observers
        {"pass_selection", "int"},
        {"pass_4j3b_selection", "int"},
        {"pass_4j2b_selection", "int"},
        {"pass_boosted_trigger", "int"},
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

        // GenParticle variables
        {"gen_bhadron_fromhh", "vector<int>"},
        {"gen_bhadron_px", "vector<float>"},
        {"gen_bhadron_py", "vector<float>"},
        {"gen_bhadron_pz", "vector<float>"},
        {"gen_bhadron_energy", "vector<float>"},
        {"gen_bhadron_mass", "vector<float>"},
        {"gen_bhadron_pt", "vector<float>"},
        {"gen_bhadron_eta", "vector<float>"},
        {"gen_bhadron_phi", "vector<float>"},
        {"gen_bhadron_charge", "vector<int>"},
        {"gen_bhadron_pid", "vector<int>"},

        // Gen-level info
        {"genpart_pt", "vector<float>"},
        {"genpart_eta", "vector<float>"},
        {"genpart_phi", "vector<float>"},
        {"genpart_energy", "vector<float>"},
        {"genpart_pid", "vector<int>"},
        {"process_index", "int"},
        {"gen_weight", "vector<float>"},
    };

    // Create output files
    TFile *fout = nullptr;
    TTree *tree_out = nullptr;
    EventData data_out(branchListOut);
    createOutputFile(outputFile.Data(), fout, tree_out, data_out);

    // Read input
    TFile *file_in = nullptr;
    TTree *tree_in = nullptr;
    EventData data_in(branchListIn);
    openFile(inputFile.Data(), file_in, tree_in, data_in);
    Long64_t allEntries = tree_in->GetEntries();

    std::cerr << "** Input file:    " << inputFile << std::endl;
    std::cerr << "** Total events:  " << allEntries << std::endl;

    // Initialize onnx helper
    auto orthelper = OrtHelperSophonHH(modelPath.Data(), debug);

    // Loop over all events
    int num_processed = 0;
    for (Long64_t entry = 0; entry < allEntries; ++entry) {
        if (entry % 1000 == 0) {
            std::cerr << "processing " << entry << " of " << allEntries << " events." << std::endl;
        }
        // Load selected branches with data from specified event
        tree_in->GetEntry(entry);
        ++num_processed;

        // reset data
        data_out.reset();
        
        // Infer the SophonHH model
        std::map<std::string, std::vector<float>*> particleVars;
        std::map<std::string, float> globalVars;
        for (auto v: std::vector<std::string>({"part_px", "part_py", "part_pz", "part_energy", "part_pt", "part_eta", "part_phi", "part_deta", "part_dphi", "part_d0val", "part_d0err", "part_dzval", "part_dzerr"})) {
            particleVars[v] = data_in.vfloatVars.at(v);
        }
        for (auto v: std::vector<std::string>({"part_charge", "part_pid"})) {
            particleVars[v] = new std::vector<float>();
            for (size_t idx = 0; idx < data_in.vintVars.at(v)->size(); idx++) {
                particleVars[v]->push_back(data_in.vintVars.at(v)->at(idx));
            }
        }
        globalVars["pfcand_sum_pt"] = data_in.floatVars.at("pfcand_sum_pt");
        globalVars["pfcand_sum_energy"] = data_in.floatVars.at("pfcand_sum_energy");

        orthelper.infer_model(particleVars, globalVars);
        const auto &output = orthelper.get_output();

        // Get inference output
        for (size_t i = 0; i < 138; i++) {
            data_out.vfloatVars.at("event_sophonHH_probs")->push_back(output[i]);
        }

        // Fill the rest of branches
        std::vector<std::string> exclude_branches_out = {"event_sophonHH_probs"};
        for (auto& pair : branchListOut) {
            if (std::find(exclude_branches_out.begin(), exclude_branches_out.end(), pair.first) == exclude_branches_out.end()) {
                data_out.copyBranchFromOther(data_in, pair.first);
            }
        }

        tree_out->Fill();

    } // end loop of events

    closeFile(file_in);
    writeOutputFile(fout);

    std::cerr << TString::Format("** Written %d events to output %s", num_processed, outputFile.Data()) << std::endl;

}

//------------------------------------------------------------------------------
