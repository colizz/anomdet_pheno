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

#include "EventData.h"


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
    tree = new TTree("tree", "tree");
    data.setOutputBranch(tree);
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
    return false;
}


void processEvent(const EventData& data_in, EventData& data_out) {
    data_out.reset();
    data_out.copy(data_in);
}


void mergeROOTFiles(
    const std::vector<std::vector<std::string>>& filePaths, const std::vector<short>& index, const std::vector<short>& order,
    std::vector<std::pair<std::string, std::string>>& branchListIn,
    std::vector<std::pair<std::string, std::string>>& branchListOut,
    const std::string& inputDirPrefix, const std::string& outputDirPath, const std::string& outputFileName, int outputFileStartIndex, const std::tuple<float, float>& loadRange,
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
            eventTotalNum[i] = inputTrees[i]->GetEntries();
            eventCount[i] = int(eventTotalNum[i] * std::get<0>(loadRange)); // reset event count to the starting point
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
        if (eventCount[i] >= int(eventTotalNum[i] * std::get<1>(loadRange))) {
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

void mixNtuples(std::string inputJson, std::string inputDirPrefix, std::string outputDirPath, std::string outputFileName, int outputFileStartIndex, std::tuple<float, float> loadRange, std::string selectionMode="all") {

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
        order.insert(order.end(), int(nevents_target[i] * std::get<1>(loadRange)) - int(nevents_target[i] * std::get<0>(loadRange)), i);
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
    mergeROOTFiles(filelist, index, order, branchListIn, branchListOut, inputDirPrefix, outputDirPath, outputFileName, outputFileStartIndex, loadRange, selectionMode);
}
