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


double deltaPhi(double phi1, double phi2) { return TVector2::Phi_mpi_pi(phi1 - phi2); }

double deltaR(double eta1, double phi1, double eta2, double phi2) {
  double deta = eta1 - eta2;
  double dphi = deltaPhi(phi1, phi2);
  return std::hypot(deta, dphi);
}

template <class T1, class T2>
double deltaR(const T1 &a, const T2 &b) {
  return deltaR(a->Eta, a->Phi, b->Eta, b->Phi);
}

class JetMatching {
public:
  enum EventType {
    QCD = 0,
    Higgs,
    Top,
    W,
    Z,
  };

//   enum FatJetLabel {
//     Invalid = 0,
//     Top_all = 10,
//     Top_bcq,
//     Top_bqq,
//     Top_bc,
//     Top_bq,
//     Top_ben,
//     Top_bmn,
//     W_all = 20,
//     W_cq,
//     W_qq,
//     Z_all = 30,
//     Z_bb,
//     Z_cc,
//     Z_qq,
//     H_all = 40,
//     H_bb,
//     H_cc,
//     H_qq,
//     H_gg,
//     H_ww4q,
//     H_ww2q1l,
//     QCD_all = 50,
//     QCD_bb,
//     QCD_cc,
//     QCD_b,
//     QCD_c,
//     QCD_others
//   };

public:
  struct JetMatchingResult {
    std::string label;
    std::vector<const GenParticle*> particles;
    std::vector<const GenParticle*> resParticles;
  };
  JetMatchingResult& getResult() { return result_; }
  void clearResult() {
    result_.label = "Invalid";
    result_.particles.clear();
    result_.resParticles.clear();
  }

public:
  JetMatching() {}
  JetMatching(double jetR) : jetR_(jetR) {}

  virtual ~JetMatching() {}

  EventType event_type() const { return event_type_; }

  void getLabel(const Jet *jet, const TClonesArray *branchParticle) {
    genParticles_.clear();
    for (Int_t i = 0; i < branchParticle->GetEntriesFast(); ++i) {
      genParticles_.push_back((GenParticle *)branchParticle->At(i));
    }
    processed_.clear();
    event_type_ = EventType::QCD;

    if (debug_) {
      std::cout << "\n=======\nJet (energy, pT, eta, phi) = " << jet->P4().Energy() << ", " << jet->PT << ", "
                << jet->Eta << ", " << jet->Phi << std::endl
                << std::endl;
    //   printGenInfoHeader();
    //   for (unsigned ipart = 0; ipart < genParticles_.size(); ++ipart) {
    //     printGenParticleInfo(genParticles_[ipart], ipart);
    //   }
    }

    for (const auto *gp : genParticles_) {
      if (processed_.count(gp))
        continue;
      processed_.insert(gp);

      auto pdgid = std::abs(gp->PID);
      if (pdgid == ParticleID::p_h0) {
        // iterative over two daughters
        auto hdaus = getDaughters(gp);
        for (const auto *dau : hdaus) {
          clearResult();
          ak4_label(jet, dau);
          if (getResult().label != "Invalid") {
            return;
          }
        }
      }
    }

    if (genParticles_.size() != processed_.size())
      throw std::logic_error("[JetMatching] Not all genParticles are processed!");

  }

  int find_label_index() {

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

    void ak4_label(const Jet *jet, const GenParticle *parton) {

        auto partonfinal = getFinal(parton);

        if (deltaR(jet, partonfinal) < jetR_) {

            getResult().resParticles.push_back(partonfinal);

            if (debug_){
                using namespace std;
                cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")" << endl;
                std::cout << "parton:     "; printGenParticleInfo(parton, -1);
            }

            auto pdgid = std::abs(partonfinal->PID);
            auto charge = partonfinal->Charge;
            if (pdgid == ParticleID::p_b) {
                getResult().label = (charge > 0) ? "bp" : "bm";
            }else if (pdgid == ParticleID::p_c) {
                getResult().label = (charge > 0) ? "cp" : "cm";
            }else if (pdgid == ParticleID::p_s) {
                getResult().label = (charge > 0) ? "sp" : "sm";
            }else if (pdgid == ParticleID::p_d) {
                getResult().label = (charge > 0) ? "dp" : "dm";
            }else if (pdgid == ParticleID::p_u) {
                getResult().label = (charge > 0) ? "up" : "um";
            }else if (pdgid == ParticleID::p_g) {
                getResult().label = "g";
            }else if (pdgid == ParticleID::p_eminus) {
                getResult().label = (charge > 0) ? "ep" : "em";
            }else if (pdgid == ParticleID::p_muminus) {
                getResult().label = (charge > 0) ? "mp" : "mm";
            }else if (pdgid == ParticleID::p_tauminus) {
                // only consider hadronic tau
                auto taudaus = getTauDaughters(partonfinal);
                if (taudaus.second == 2) {
                    getResult().resParticles.insert(getResult().resParticles.end(), taudaus.first.begin(), taudaus.first.end());
                    getResult().label = (charge > 0) ? "tauhp" : "tauhm";
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
  EventType event_type_ = EventType::QCD;
  JetMatchingResult result_{"Invalid", std::vector<const GenParticle*>(), std::vector<const GenParticle*>()};

  std::vector<std::string> labels_{
    "bp", "bm", "cp", "cm", "sp", "sm", "dp", "dm", "up", "um", "g", "ep", "em", "mp", "mm", "tauhp", "tauhm"
  };
};

#endif