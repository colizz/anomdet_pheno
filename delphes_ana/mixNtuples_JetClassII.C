#include <TFile.h>
#include <TTree.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <stdexcept>
#include "nlohmann/json.hpp"

// Define a struct to hold all branch variables
struct EventData {
    std::vector<std::pair<std::string, std::string>> branchList;
    std::map<std::string, bool> boolVars;
    std::map<std::string, int> intVars;
    std::map<std::string, float> floatVars;
    std::map<std::string, std::vector<bool>*> boolArrayVars;
    std::map<std::string, std::vector<int>*> intArrayVars;
    std::map<std::string, std::vector<float>*> floatArrayVars;

    EventData(std::vector<std::pair<std::string, std::string>>& branchList_) {
        branchList = branchList_;
        for (const auto& pair : branchList_) {
            if (pair.second == "bool") {
                boolVars[pair.first] = 0;
            }
            else if (pair.second == "int") {
                intVars[pair.first] = 0;
            }
            else if (pair.second == "float") {
                floatVars[pair.first] = 0;
            }
            else if (pair.second == "vector<bool>") {
                boolArrayVars[pair.first] = nullptr;
            }
            else if (pair.second == "vector<int>") {
                intArrayVars[pair.first] = nullptr;
            }
            else if (pair.second == "vector<float>") {
                floatArrayVars[pair.first] = nullptr;
            }
            else {
                throw std::runtime_error("Invalid branch type: " + pair.second);
            }
        }
    }
    ~EventData() {
        for (auto& pair : boolArrayVars) {
            if (pair.second) {
                delete pair.second;
            }
        }
        for (auto& pair : intArrayVars) {
            if (pair.second) {
                delete pair.second;
            }
        }
        for (auto& pair : floatArrayVars) {
            if (pair.second) {
                delete pair.second;
            }
        }
    }
    void reinitialize() {
        for (auto& pair : boolArrayVars) {
            if (pair.second)
                pair.second->clear();
            else
                pair.second = new std::vector<bool>;
        }
        for (auto& pair : intArrayVars) {
            if (pair.second)
                pair.second->clear();
            else
                pair.second = new std::vector<int>;
        }
        for (auto& pair : floatArrayVars) {
            if (pair.second)
                pair.second->clear();
            else
                pair.second = new std::vector<float>;
        }
    }
};

void printEventData(const EventData& data) {
    // std::cout << "Event number: " << data.event_no << std::endl;
    for (auto& pair : data.floatVars) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    for (auto& pair : data.floatArrayVars) {
        std::cout << pair.first << ": ";
        if (pair.second) {
            for (float val : *pair.second) {
                std::cout << val << " ";
            }
        }
        std::cout << std::endl;
    }
}

// Function to set branch addresses for an input tree
void SetBranchAddresses(TTree* tree, EventData& data) {
    for (auto& pair : data.boolVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.intVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.floatVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.boolArrayVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.intArrayVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.floatArrayVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
}

// Function to link the output branches to the output tree
void SetOutputBranch(TTree* tree, EventData& data) {
    // follow the order
    for (const auto& pair : data.branchList) {
        if (pair.second == "bool") {
            tree->Branch(pair.first.c_str(), &data.boolVars.at(pair.first));
        }
        else if (pair.second == "int") {
            tree->Branch(pair.first.c_str(), &data.intVars.at(pair.first));
        }
        else if (pair.second == "float") {
            tree->Branch(pair.first.c_str(), &data.floatVars.at(pair.first));
        }
        else if (pair.second == "vector<bool>") {
            tree->Branch(pair.first.c_str(), &data.boolArrayVars.at(pair.first), /*bufsize=*/102400);
        }
        else if (pair.second == "vector<int>") {
            tree->Branch(pair.first.c_str(), &data.intArrayVars.at(pair.first), /*bufsize=*/102400);
        }
        else if (pair.second == "vector<float>") {
            tree->Branch(pair.first.c_str(), &data.floatArrayVars.at(pair.first), /*bufsize=*/102400);
        }
        else {
            throw std::runtime_error("Invalid branch type: " + pair.second);
        }
    }
}

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
    SetBranchAddresses(tree, data);
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
    tree = new TTree("tree", "tree");
    SetOutputBranch(tree, data);
}

void writeOutputFile(TFile*& file) {
    file->Write();
    file->Close();
    file = nullptr;
}

bool passSection(const EventData& data, const std::string& selectionMode) {
    if (selectionMode == "all") {
        return true;
    }
    // else if (selectionMode == "pass_selection") {
    //     return data.floatVars.at("pass_selection") > 0;
    // }
    // else if (selectionMode == "msdgt130") {
    //     return data.floatVars.at("jet_sdmass") > 130;
    // }
    // else if (selectionMode == "qcdlt0p1") {
    //     float probs = 0;
    //     for (int i=161; i<188; i++) {
    //         probs += data.arrayVars.at("jet_probs")->at(i);
    //     }
    //     // probs = data.arrayVars.at("jet_probs")->at(161); // for mergeQCD model
    //     return probs < 0.1;
    // }
    // else {
    //     throw std::runtime_error("Invalid selection mode: " + selectionMode);
    // }
    return false;
}

void processEvent(const EventData& data_in, EventData& data_out) {
    // formal JetClass output
    data_out.reinitialize();

    // PF candidates
    for (size_t i = 0; i < data_in.floatArrayVars.at("part_px")->size(); i++) {
        for (const std::string& branch : {"part_px", "part_py", "part_pz", "part_energy", "part_deta", "part_dphi", "part_d0val", "part_d0err", "part_dzval", "part_dzerr"}) {
            data_out.floatArrayVars.at(branch)->push_back(data_in.floatArrayVars.at(branch)->at(i));
        }
        data_out.intArrayVars.at("part_pid")->push_back(data_in.floatArrayVars.at("part_pid")->at(i));
        data_out.intArrayVars.at("part_charge")->push_back(data_in.floatArrayVars.at("part_charge")->at(i));

        data_out.boolArrayVars.at("part_isElectron")->push_back(data_in.floatArrayVars.at("part_pid")->at(i) == 11 || data_in.floatArrayVars.at("part_pid")->at(i) == -11);
        data_out.boolArrayVars.at("part_isMuon")->push_back(data_in.floatArrayVars.at("part_pid")->at(i) == 13 || data_in.floatArrayVars.at("part_pid")->at(i) == -13);
        data_out.boolArrayVars.at("part_isPhoton")->push_back(data_in.floatArrayVars.at("part_pid")->at(i) == 22);
        data_out.boolArrayVars.at("part_isChargedHadron")->push_back(data_in.floatArrayVars.at("part_charge")->at(i) != 0 && !data_out.boolArrayVars.at("part_isElectron")->back() && !data_out.boolArrayVars.at("part_isMuon")->back());
        data_out.boolArrayVars.at("part_isNeutralHadron")->push_back(data_in.floatArrayVars.at("part_charge")->at(i) == 0 && !data_out.boolArrayVars.at("part_isPhoton")->back());
    }

    // jet features
    data_out.floatVars.at("jet_pt") = data_in.floatVars.at("jet_pt");
    data_out.floatVars.at("jet_eta") = data_in.floatVars.at("jet_eta");
    data_out.floatVars.at("jet_phi") = data_in.floatVars.at("jet_phi");
    data_out.floatVars.at("jet_energy") = data_in.floatVars.at("jet_energy");
    data_out.intVars.at("jet_nparticles") = data_in.floatVars.at("jet_nparticles");
    data_out.floatVars.at("jet_sdmass") = data_in.floatVars.at("jet_sdmass");
    data_out.floatVars.at("jet_tau1") = data_in.floatVars.at("jet_tau1");
    data_out.floatVars.at("jet_tau2") = data_in.floatVars.at("jet_tau2");
    data_out.floatVars.at("jet_tau3") = data_in.floatVars.at("jet_tau3");
    data_out.floatVars.at("jet_tau4") = data_in.floatVars.at("jet_tau4");
    data_out.intVars.at("jet_label") = (data_in.floatVars.at("jet_label") < 15) ? data_in.floatVars.at("jet_label") : data_in.floatVars.at("jet_label") - 3;

    // aux vars
    for (size_t i = 0; i < data_in.floatArrayVars.at("genres_pt")->size(); i++) {
        data_out.floatArrayVars.at("aux_genpart_pt")->push_back(data_in.floatArrayVars.at("genres_pt")->at(i));
        data_out.floatArrayVars.at("aux_genpart_eta")->push_back(data_in.floatArrayVars.at("genres_eta")->at(i));
        data_out.floatArrayVars.at("aux_genpart_phi")->push_back(data_in.floatArrayVars.at("genres_phi")->at(i));
        data_out.floatArrayVars.at("aux_genpart_mass")->push_back(data_in.floatArrayVars.at("genres_mass")->at(i));
        data_out.intArrayVars.at("aux_genpart_pid")->push_back(data_in.floatArrayVars.at("genres_pid")->at(i));
    }
    for (size_t i = 0; i < data_in.floatArrayVars.at("genpart_pt")->size(); i++) {
        data_out.floatArrayVars.at("aux_genpart_pt")->push_back(data_in.floatArrayVars.at("genpart_pt")->at(i));
        data_out.floatArrayVars.at("aux_genpart_eta")->push_back(data_in.floatArrayVars.at("genpart_eta")->at(i));
        data_out.floatArrayVars.at("aux_genpart_phi")->push_back(data_in.floatArrayVars.at("genpart_phi")->at(i));
        data_out.floatArrayVars.at("aux_genpart_mass")->push_back(data_in.floatArrayVars.at("genpart_mass")->at(i));
        data_out.intArrayVars.at("aux_genpart_pid")->push_back(data_in.floatArrayVars.at("genpart_pid")->at(i));
    }
}

void mergeROOTFiles(
    const std::vector<std::vector<std::string>>& filePaths, const std::vector<short>& index, const std::vector<short>& order,
    std::vector<std::pair<std::string, std::string>>& branchListIn,
    std::vector<std::pair<std::string, std::string>>& branchListOut,
    const std::string& inputDirPrefix, const std::string& outputDirPath, const std::string& outputFileName, int outputFileStartIndex, const std::string& trainValMode,
    const std::string& selectionMode
    ) {

    EventData data_in(branchListIn), data_out(branchListOut);
    int output_file_idx = outputFileStartIndex;
    int store_per_event = 100000;
    bool debug = false;

    TFile* outputFile = nullptr;
    TTree* outputTree = nullptr;
    std::vector<TFile*> inputFiles = std::vector<TFile*>(filePaths.size(), nullptr);
    std::vector<TTree*> inputTrees = std::vector<TTree*>(filePaths.size(), nullptr);
    std::vector<int> fileCount(filePaths.size(), 0);
    std::vector<long> eventCount(filePaths.size(), 0);
    std::vector<long> eventTotalNum(filePaths.size(), 0);

    // Loop through the order list to read and write events accordingly
    long num_processed = 0;
    for (int i : order) {
        if (i < 0 || i >= (short)filePaths.size()) {
            throw std::runtime_error("Invalid index " + std::to_string(i) + " in order list.");
        }

        // open the output file if not opened
        if (outputFile == nullptr) {
            std::ostringstream oss;
            oss << std::setw(4) << std::setfill('0') << output_file_idx;
            std::string output_file_idx_str = oss.str();
            createOutputFile(outputDirPath + TString::Format("/%s_%s.root", outputFileName.c_str(), output_file_idx_str.c_str()).Data(), outputFile, outputTree, data_out);
        }
        // get the data from the corresponding file
        if (inputFiles[i] == nullptr) {
            if (fileCount[i] >= (int)filePaths[i].size()) {
                throw std::runtime_error("No more files to read at index " + std::to_string(fileCount[i]) + ". This should never happen.");
            }
            openFile(inputDirPrefix + "/" + filePaths[i][fileCount[i]], inputFiles[i], inputTrees[i], data_in);
            if (trainValMode == "train") {
                eventCount[i] = 0; // reset event count
            }
            else if (trainValMode == "val") {
                eventCount[i] = int(eventTotalNum[i] * 0.8);
            }
            eventTotalNum[i] = inputTrees[i]->GetEntries();
            std::cout << "Start reading new file at index " << i << ": " << filePaths[i][fileCount[i]] << ", Total events: " << eventTotalNum[i] << ", Start from pos: " << eventCount[i] <<  std::endl;
        }
        
        // get entry
        inputTrees[i]->GetEntry(eventCount[i]);
        eventCount[i]++;
        // data.event_no = num_processed;
        // data.event_class = index[i];
        processEvent(data_in, data_out);

        if (debug) {
            std::cout << ">> Reading file at index " << i << ": this is #event " << eventCount[i] - 1 << " from file " << filePaths[i][fileCount[i]] << std::endl;
            // printEventData(data);
            // std::cout << data.floatVars["jet_pt"] << " " << data.floatVars["jet_eta"] << " " << data.floatVars["jet_sdmass"] << std::endl;
        }

        // fill branches
        if (passSection(data_out, selectionMode)) {
            outputTree->Fill();
        }

        // check if reach the end of file
        if ((trainValMode == "train" && eventCount[i] >= int(eventTotalNum[i] * 0.8)) || (trainValMode == "val" && eventCount[i] >= eventTotalNum[i])) {
            closeFile(inputFiles[i]);
            inputFiles[i] = nullptr;
            std::cout << "File " << filePaths[i][fileCount[i]] << " reading completed." << std::endl;
            fileCount[i]++;
        }

        // check if we collect enough events to store
        if ((num_processed + 1) % store_per_event == 0) {
            std::cout << "Processed " << num_processed << " events." << std::endl;
            writeOutputFile(outputFile);
            outputFile = nullptr;
            output_file_idx++;
        }
        num_processed++;
    }

    // write file
    if (outputFile != nullptr) {
        writeOutputFile(outputFile);
    }
}

void mixNtuples_JetClassII(std::string inputJson, std::string inputDirPrefix, std::string outputDirPath, std::string outputFileName, int outputFileStartIndex, std::string trainValMode, std::string selectionMode="all") {

    // Read the json file to get nevents_target and filelist for each sample
    std::vector<short> index;
    std::vector<int> nevents_target;
    std::vector<std::vector<std::string>> filelist;

    std::ifstream infile(inputJson);
    nlohmann::json j;
    infile >> j;

    for (auto& element : j["samples"].items()) {
        std::cout << element.key() << " : nevents_target = " << element.value()["nevents_target"] << std::endl;
        index.push_back(element.value()["index"]);
        nevents_target.push_back(element.value()["nevents_target"]);
        std::vector<std::string> files;
        for (auto& file : element.value()["filelist"]) {
            files.push_back(file);
        }
        filelist.push_back(files);
    }

    // Generate the random list
    std::vector<short> order;

    for (size_t i = 0; i < nevents_target.size(); ++i) {
        if (trainValMode == "train") {
            order.insert(order.end(), int(nevents_target[i] * 0.8), i);
        }
        else if (trainValMode == "val") {
            order.insert(order.end(), nevents_target[i] - int(nevents_target[i] * 0.8), i);
        }
        else {
            throw std::runtime_error("Invalid trainValMode: " + trainValMode);
        }
    }
    unsigned seed = 42;
    std::default_random_engine engine(seed);
    std::shuffle(order.begin(), order.end(), engine);

    // Read the branches
    std::vector<std::pair<std::string, std::string>> branchListIn, branchListOut;
    for (const auto& item : j["input_branch_list"]) {
        branchListIn.emplace_back(item[0].get<std::string>(), item[1].get<std::string>());
    }
    for (const auto& item : j["output_branch_list"]) {
        branchListOut.emplace_back(item[0].get<std::string>(), item[1].get<std::string>());
    }

    // Merge the ROOT files
    mergeROOTFiles(filelist, index, order, branchListIn, branchListOut, inputDirPrefix, outputDirPath, outputFileName, outputFileStartIndex, trainValMode, selectionMode);
}
