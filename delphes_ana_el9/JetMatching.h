#ifndef JetMatching_h
#define JetMatching_h

#include <iostream>
#include <cassert>
#include <unordered_set>
#include <utility>
#include "TClonesArray.h"
#include "Math/LorentzVector.h"
#include <Math/Vector4D.h>
#include "classes/DelphesClasses.h"

#include "ParticleID.h"
#include "ParticleInfo.h"

class JetMatching {

public:
    struct FatJetMatchingResult {
        std::string label;
        std::vector<const GenParticle*> resParticles;
        std::vector<const GenParticle*> decayParticles;
        std::vector<const GenParticle*> tauDecayParticles;
        std::vector<const GenParticle*> qcdPartons;
    };
    FatJetMatchingResult& getResult() { return result_; }
    void clearResult() {
        result_.label = "Invalid";
        result_.resParticles.clear();
        result_.decayParticles.clear();
        result_.tauDecayParticles.clear();
        result_.qcdPartons.clear();
    }

public:
    JetMatching() {}
    JetMatching(double jetR, bool debug) : jetR_(jetR), debug_(debug) {}

    virtual ~JetMatching() {}

    void getLabel(const Jet *jet, const TClonesArray *branchParticle) {

        genParticles_.clear();
        for (Int_t i = 0; i < branchParticle->GetEntriesFast(); ++i) {
            genParticles_.push_back((GenParticle *)branchParticle->At(i));
        }
        processed_.clear();

        if (debug_) {
        std::cout << "\n=======\nJet (energy, pT, eta, phi) = " << jet->P4().Energy() << ", " << jet->PT << ", "
                    << jet->Eta << ", " << jet->Phi << std::endl
                    << std::endl;
        //   printGenInfoHeader();
        //   for (unsigned ipart = 0; ipart < genParticles_.size(); ++ipart) {
        //     printGenParticleInfo(genParticles_[ipart], ipart);
        //   }
        }

        auto daughters = std::vector<const GenParticle*>();
        for (const auto *gp : genParticles_) {
            if (processed_.count(gp))
                continue;
            processed_.insert(gp);

            auto pdgid = std::abs(gp->PID);
            if (pdgid == ParticleID::p_h0) {
                // iterative over two daughters
                auto higgs = getFinal(gp);
                auto hdaus = getDaughters(higgs);
                daughters.insert(daughters.end(), hdaus.begin(), hdaus.end());
            }
        }
        if (genParticles_.size() != processed_.size())
            throw std::logic_error("[JetMatching] Not all genParticles are processed!");

        if (daughters.size() != 4)
            throw std::logic_error("[JetMatching] Higgs decay daughters are not 4!");

        clearResult();
        ak4JetLabel(jet, daughters);
        if (getResult().label != "Invalid") {
            // std::cout << "Indentified particle: " << getResult().label << std::endl;
            return;
        }

    }

    int findLabelIndex() {

        if (getResult().label == "Invalid") {
            throw std::logic_error("[FatJetInfoFiller::fill]: label is Invalid");
        }
        
        int label_index = -1;
        auto it = std::find(labels_.begin(), labels_.end(), getResult().label);
        if (it != labels_.end()) {
            label_index = std::distance(labels_.begin(), it);
        }else {
            throw std::logic_error("[FatJetInfoFiller::fill]: unexpected label " + getResult().label);
        }
        return label_index;
    }

private:

    void ak4JetLabel(const Jet *jet, std::vector<const GenParticle*> daughters) {

        std::vector<float> drs;
        for (const auto *dau : daughters) {
            drs.push_back(deltaR(jet, dau));
        }

        // identify single-parton matching case

        const GenParticle *parton = nullptr;
        if (drs[0] < jetR_ && drs[1] >= jetR_ && drs[2] >= jetR_ && drs[3] >= jetR_) {
            parton = daughters[0];
        }else if (drs[0] >= jetR_ && drs[1] < jetR_ && drs[2] >= jetR_ && drs[3] >= jetR_) {
            parton = daughters[1];
        }else if (drs[0] >= jetR_ && drs[1] >= jetR_ && drs[2] < jetR_ && drs[3] >= jetR_) {
            parton = daughters[2];
        }else if (drs[0] >= jetR_ && drs[1] >= jetR_ && drs[2] >= jetR_ && drs[3] < jetR_) {
            parton = daughters[3];
        }
        if (parton != nullptr) {

            getResult().resParticles.push_back(parton);

            if (debug_){
                using namespace std;
                cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")" << endl;
                std::cout << "parton:     "; printGenParticleInfo(parton, -1);
            }

            auto abspid = std::abs(parton->PID);
            auto pid = parton->PID;
            if (abspid == ParticleID::p_b) {
                getResult().label = (pid > 0) ? "b" : "bbar";
            }else if (abspid == ParticleID::p_c) {
                getResult().label = (pid > 0) ? "c" : "cbar";
            }else if (abspid == ParticleID::p_s) {
                getResult().label = (pid > 0) ? "s" : "sbar";
            }else if (abspid == ParticleID::p_d) {
                getResult().label = (pid > 0) ? "d" : "dbar";
            }else if (abspid == ParticleID::p_u) {
                getResult().label = (pid > 0) ? "u" : "ubar";
            }else if (abspid == ParticleID::p_g) {
                getResult().label = "g";
            }else if (abspid == ParticleID::p_eminus) {
                getResult().label = (pid > 0) ? "em" : "ep";
            }else if (abspid == ParticleID::p_muminus) {
                getResult().label = (pid > 0) ? "mm" : "mp";
            }else if (abspid == ParticleID::p_tauminus) {
                // only consider hadronic tau
                auto taudaus = getTauDaughters(parton);
                if (taudaus.second == 2) {
                    getResult().resParticles.insert(getResult().resParticles.end(), taudaus.first.begin(), taudaus.first.end());
                    getResult().label = (pid > 0) ? "tauhm" : "tauhp";
                }
            }
        }

        // identify pair-matching
        for (int i = 0; i < 2; ++i) {
            auto dau1 = daughters.at(i * 2);
            auto dau2 = daughters.at(i * 2 + 1);
            if (deltaR(jet, dau1) < jetR_ && deltaR(jet, dau2) < jetR_) {
                getResult().resParticles.push_back(dau1);
                getResult().resParticles.push_back(dau2);

                auto abspid = std::abs(dau1->PID);
                if (abspid == ParticleID::p_b) {
                    getResult().label = "bbbar";
                }else if (abspid == ParticleID::p_c) {
                    getResult().label = "ccbar";
                }else if (abspid == ParticleID::p_s) {
                    getResult().label = "ssbar";
                }else if (abspid == ParticleID::p_d) {
                    getResult().label = "ddbar";
                }else if (abspid == ParticleID::p_u) {
                    getResult().label = "uubar";
                }else if (abspid == ParticleID::p_g) {
                    getResult().label = "gg";
                }else if (abspid == ParticleID::p_eminus) {
                    getResult().label = "epem";
                }else if (abspid == ParticleID::p_muminus) {
                    getResult().label = "mpmm";
                }else if (abspid == ParticleID::p_tauminus) {
                    auto tau1daus = getTauDaughters(dau1);
                    auto tau2daus = getTauDaughters(dau2);
                    if (tau1daus.second == 2 && tau2daus.second == 2) {
                        getResult().resParticles.insert(getResult().resParticles.end(), tau1daus.first.begin(), tau1daus.first.end());
                        getResult().resParticles.insert(getResult().resParticles.end(), tau2daus.first.begin(), tau2daus.first.end());
                        getResult().label = "tauhptauhm";
                    }
                }
            }
        }

    }

private:
    void printGenInfoHeader() const {
        using namespace std;
        cout << right << setw(6) << "#"
            << " " << setw(10) << "pdgId"
            << "  "
            << "Chg"
            << "  " << setw(10) << "Mass"
            << "  " << setw(48) << " Momentum" << left << "  " << setw(10) << "Mothers"
            << " " << setw(30) << "Daughters" << endl;
    }

    void printGenParticleInfo(const GenParticle *genParticle, const int idx) const {
        using namespace std;
        cout << right << setw(3) << genParticle->Status;
        cout << right << setw(3) << idx << " " << setw(10) << genParticle->PID << "  ";
        cout << right << "  " << setw(3) << genParticle->Charge << "  "
             << TString::Format("%10.3g", genParticle->Mass < 1e-5 ? 0 : genParticle->Mass);
        cout << left << setw(50)
             << TString::Format("  (E=%6.4g pT=%6.4g eta=%7.3g phi=%7.3g)",
                                genParticle->P4().Energy(),
                                genParticle->PT,
                                genParticle->Eta,
                                genParticle->Phi);

        TString mothers;
        if (genParticle->M1 >= 0) {
            mothers += genParticle->M1;
        }
        if (genParticle->M2 >= 0) {
            mothers += ",";
            mothers += genParticle->M2;
        }
        cout << "  " << setw(10) << mothers;

        TString daughters;
        for (int iDau = genParticle->D1; iDau <= genParticle->D2; ++iDau) {
            if (daughters.Length())
                daughters += ",";
            daughters += iDau;
        }
        cout << " " << setw(30) << daughters << endl;
    }

    const GenParticle *getFinal(const GenParticle *particle) {
        // will mark intermediate particles as processed
        if (!particle)
            return nullptr;
        processed_.insert(particle);
        const GenParticle *final = particle;

        while (final->D1 >= 0) {
            const GenParticle *chain = nullptr;
            for (int idau = final->D1; idau <= final->D2; ++idau) {
                if (genParticles_.at(idau)->PID == particle->PID) {
                    chain = genParticles_.at(idau);
                    processed_.insert(chain);
                    break;
                }
            }
            if (!chain)
                break;
            final = chain;
        }
        return final;
    }

    bool isHadronic(const GenParticle *particle, bool allow_gluon = false) const {
        // particle needs to be the final version before decay
        if (!particle)
            throw std::invalid_argument("[JetMatching::isHadronic()] Null particle!");
        for (const auto *dau : getDaughters(particle)) {
            auto pdgid = std::abs(dau->PID);
            if (pdgid >= ParticleID::p_d && pdgid <= ParticleID::p_b)
                return true;
            if (allow_gluon && pdgid == ParticleID::p_g)
                return true;
        }
        return false;
    }

    std::vector<const GenParticle *> getDaughters(const GenParticle *particle) const {
        std::vector<const GenParticle *> daughters;
        for (int idau = particle->D1; idau <= particle->D2; ++idau) {
            daughters.push_back(genParticles_.at(idau));
        }
        return daughters;
    }

    std::vector<const GenParticle *> getDaughterQuarks(const GenParticle *particle, bool allow_gluon = false) {
        std::vector<const GenParticle *> daughters;
        for (int idau = particle->D1; idau <= particle->D2; ++idau) {
            const auto *dau = genParticles_.at(idau);
            auto pdgid = std::abs(dau->PID);
            if (pdgid >= ParticleID::p_d && pdgid <= ParticleID::p_b) {
                daughters.push_back(dau);
            }
            if (allow_gluon && pdgid == ParticleID::p_g) {
                daughters.push_back(dau);
            }
        }
        return daughters;
    }

    std::pair<std::vector<const GenParticle*>, int> getTauDaughters(const GenParticle* particle) {
        const auto tau = getFinal(particle);
        auto daughters = getDaughters(tau);
        for (const auto & dau: daughters){
            auto pdgid = std::abs(dau->PID);
            if (pdgid == ParticleID::p_eminus)  return std::make_pair(daughters, 0);
            if (pdgid == ParticleID::p_muminus)  return std::make_pair(daughters, 1);
        }
        return std::make_pair(daughters, 2); // hadronic mode
    }


private:
    double jetR_ = 0.4;
    bool debug_ = false;
    std::vector<const GenParticle *> genParticles_;
    std::unordered_set<const GenParticle *> processed_;
    FatJetMatchingResult result_{"Invalid", std::vector<const GenParticle*>(), std::vector<const GenParticle*>(), std::vector<const GenParticle*>(), std::vector<const GenParticle*>()};

    std::vector<std::string> labels_{
        "b", "bbar", "c", "cbar", "s", "sbar", "d", "dbar", "u", "ubar", "g", "em", "ep", "mm", "mp", "tauhm", "tauhp",
        "bbbar", "ccbar", "ssbar", "ddbar", "uubar", "gg", "epem", "mpmm", "tauhptauhm"
    };

};

#endif