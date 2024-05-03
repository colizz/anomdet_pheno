#include <TFile.h>
#include <TTree.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <stdexcept>
#include "nlohmann/json.hpp"

// Define a struct to hold all branch variables
struct EventData {
    std::map<std::string, float> floatVars;
    std::map<std::string, std::vector<float>*> arrayVars;
    int event_no = 0; // new file branch
    int event_class = 0; // new file branch

    EventData(std::map<std::string, std::vector<std::string>>& branchMap) {
        for (const auto& branch : branchMap["float"]) {
            floatVars[branch] = 0;
        }
        for (const auto& branch : branchMap["array"]) {
            arrayVars[branch] = nullptr;
        }
    }
    ~EventData() {
        for (auto& pair : arrayVars) {
            if (pair.second) {
                delete pair.second;
            }
        }
    }
};

void printEventData(const EventData& data) {
    std::cout << "Event number: " << data.event_no << std::endl;
    for (auto& pair : data.floatVars) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    for (auto& pair : data.arrayVars) {
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
    for (auto& pair : data.floatVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.arrayVars) {
        tree->SetBranchAddress(pair.first.c_str(), &pair.second);
    }
}

// Function to link the output branches to the output tree
void SetOutputBranch(TTree* tree, EventData& data) {
    for (auto& pair : data.floatVars) {
        tree->Branch(pair.first.c_str(), &pair.second);
    }
    for (auto& pair : data.arrayVars) {
        tree->Branch(pair.first.c_str(), &pair.second, /*bufsize=*/102400);
    }
    tree->Branch("event_no", &data.event_no);
    tree->Branch("event_class", &data.event_class);
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
    else if (selectionMode == "pass_selection") {
        return data.floatVars.at("pass_selection") > 0;
    }
    else if (selectionMode == "msdgt130") {
        return data.floatVars.at("jet_sdmass") > 130;
    }
    else if (selectionMode == "qcdlt0p1") {
        float probs = 0;
        for (int i=161; i<188; i++) {
            probs += data.arrayVars.at("jet_probs")->at(i);
        }
        // probs = data.arrayVars.at("jet_probs")->at(161); // for mergeQCD model
        return probs < 0.1;
    }
    else {
        throw std::runtime_error("Invalid selection mode: " + selectionMode);
    }
}

void mergeROOTFiles(
    const std::vector<std::vector<std::string>>& filePaths, const std::vector<short>& index, const std::vector<short>& order,
    std::map<std::string, std::vector<std::string>>& branchMap,
    const std::string& inputDirPrefix, const std::string& outputDirPath, const std::string& selectionMode
    ) {

    EventData data(branchMap);
    int output_file_idx = 0;
    int store_per_event = 500000;
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
            createOutputFile(outputDirPath + TString::Format("/ntuples_%d.root", output_file_idx).Data(), outputFile, outputTree, data);
        }
        // get the data from the corresponding file
        if (inputFiles[i] == nullptr) {
            if (fileCount[i] >= (int)filePaths[i].size()) {
                throw std::runtime_error("No more files to read at index " + std::to_string(fileCount[i]) + ". This should never happen.");
            }
            openFile(inputDirPrefix + "/" + filePaths[i][fileCount[i]], inputFiles[i], inputTrees[i], data);
            eventCount[i] = 0; // reset event count
            eventTotalNum[i] = inputTrees[i]->GetEntries();
            std::cout << "Start reading new file at index " << i << ": " << filePaths[i][fileCount[i]] << ", Total events: " << eventTotalNum[i] << std::endl;
        }
        
        // get entry
        inputTrees[i]->GetEntry(eventCount[i]);
        eventCount[i]++;
        data.event_no = num_processed;
        data.event_class = index[i];

        if (debug) {
            std::cout << ">> Reading file at index " << i << ": this is #event " << eventCount[i] - 1 << " from file " << filePaths[i][fileCount[i]] << std::endl;
            // printEventData(data);
            // std::cout << data.floatVars["jet_pt"] << " " << data.floatVars["jet_eta"] << " " << data.floatVars["jet_sdmass"] << std::endl;
        }

        // fill branches
        if (passSection(data, selectionMode)) {
            outputTree->Fill();
        }

        // check if reach the end of file
        if (eventCount[i] >= eventTotalNum[i]) {
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

void mixNtuples(std::string inputJson, std::string inputDirPrefix, std::string outputDirPath, std::string selectionMode="all") {

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
        order.insert(order.end(), nevents_target[i], i);
    }
    unsigned seed = 42;
    std::default_random_engine engine(seed);
    std::shuffle(order.begin(), order.end(), engine);

    // Read the branches
    std::map<std::string, std::vector<std::string>> branchMap;
    for (auto& element : j["branches"].items()) {
        std::vector<std::string> branches = element.value().get<std::vector<std::string>>();
        branchMap[element.key()] = branches;
    }

    // Merge the ROOT files
    mergeROOTFiles(filelist, index, order, branchMap, inputDirPrefix, outputDirPath, selectionMode);
}
