#ifndef FatJetMatching_h
#define FatJetMatching_h

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

class FatJetMatching {
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
  struct FatJetMatchingResult {
    std::string label;
    std::vector<const GenParticle*> particles;
    std::vector<const GenParticle*> resParticles;
  };
  FatJetMatchingResult& getResult() { return result_; }
  void clearResult() {
    result_.label = "Invalid";
    result_.particles.clear();
    result_.resParticles.clear();
  }

public:
  FatJetMatching() {}
  FatJetMatching(double jetR, bool assignQCDLabel) : jetR_(jetR), assignQCDLabel_(assignQCDLabel) {}

  virtual ~FatJetMatching() {}

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
      if (pdgid == ParticleID::p_h0 || pdgid == ParticleID::p_H0 || pdgid == ParticleID::p_Hplus) {
        clearResult();
        higgs_label(jet, gp);
        if (getResult().label != "Invalid") {
          return;
        }
      }
    }

    if (genParticles_.size() != processed_.size())
      throw std::logic_error("[FatJetMatching] Not all genParticles are processed!");

    if (assignQCDLabel_) {
      clearResult();
      qcd_label(jet);
    }
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

// private:
//   std::pair<FatJetLabel, const GenParticle *> top_label(const Jet *jet, const GenParticle *parton) {
//     // top
//     auto top = getFinal(parton);
//     // find the W and test if it's hadronic
//     const GenParticle *w_from_top = nullptr, *b_from_top = nullptr;
//     for (const auto *dau : getDaughters(top)) {
//       if (std::abs(dau->PID) == ParticleID::p_Wplus) {
//         w_from_top = getFinal(dau);
//       } else if (std::abs(dau->PID) <= ParticleID::p_b) {
//         // ! use <= p_b ! -- can also have charms etc.
//         b_from_top = dau;
//       }
//     }
//     if (!w_from_top || !b_from_top)
//       throw std::logic_error("[FatJetMatching::top_label] Cannot find b or W from top decay!");

//     if (isHadronic(w_from_top)) {
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::Top;
//       }
//       if (debug_) {
//         using namespace std;
//         cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")"
//              << endl;
//         cout << "top: ";
//         printGenParticleInfo(top, -1);
//         cout << "b:   ";
//         printGenParticleInfo(b_from_top, -1);
//         cout << "W:   ";
//         printGenParticleInfo(w_from_top, -1);
//       }

//       auto wdaus = getDaughterQuarks(w_from_top);
//       if (wdaus.size() < 2)
//         throw std::logic_error("[FatJetMatching::top_label] W decay has less than 2 quarks!");
//       //    if (wdaus.size() >= 2)
//       {
//         double dr_b = deltaR(jet, b_from_top);
//         double dr_q1 = deltaR(jet, wdaus.at(0));
//         double dr_q2 = deltaR(jet, wdaus.at(1));
//         if (dr_q1 > dr_q2) {
//           // swap q1 and q2 so that dr_q1<=dr_q2
//           std::swap(dr_q1, dr_q2);
//           std::swap(wdaus.at(0), wdaus.at(1));
//         }

//         if (debug_) {
//           using namespace std;
//           cout << "deltaR(jet, b)     : " << dr_b << endl;
//           cout << "deltaR(jet, q1)    : " << dr_q1 << endl;
//           cout << "deltaR(jet, q2)    : " << dr_q2 << endl;
//         }

//         if (dr_b < jetR_) {
//           auto pdgid_q1 = std::abs(wdaus.at(0)->PID);
//           auto pdgid_q2 = std::abs(wdaus.at(1)->PID);
//           if (debug_) {
//             using namespace std;
//             cout << "pdgid(q1)        : " << pdgid_q1 << endl;
//             cout << "pdgid(q2)        : " << pdgid_q2 << endl;
//           }

//           if (dr_q1 < jetR_ && dr_q2 < jetR_) {
//             if (pdgid_q1 >= ParticleID::p_c || pdgid_q2 >= ParticleID::p_c) {
//               return std::make_pair(FatJetLabel::Top_bcq, top);
//             } else {
//               return std::make_pair(FatJetLabel::Top_bqq, top);
//             }
//           } else if (dr_q1 < jetR_ && dr_q2 >= jetR_) {
//             if (pdgid_q1 >= ParticleID::p_c) {
//               return std::make_pair(FatJetLabel::Top_bc, top);
//             } else {
//               return std::make_pair(FatJetLabel::Top_bq, top);
//             }
//           }
//         } else {
//           // test for W if dr(b, jet) > jetR_
//           return w_label(jet, w_from_top);
//         }
//       }
//     } else {
//       // leptonic W
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::Top;
//       }
//       if (debug_) {
//         using namespace std;
//         cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")"
//              << endl;
//         cout << "top: ";
//         printGenParticleInfo(top, -1);
//         cout << "b:   ";
//         printGenParticleInfo(b_from_top, -1);
//         cout << "W:   ";
//         printGenParticleInfo(w_from_top, -1);
//       }

//       const GenParticle *lep = nullptr;
//       for (int idau = w_from_top->D1; idau <= w_from_top->D2; ++idau) {
//         const auto *dau = genParticles_.at(idau);
//         auto pdgid = std::abs(dau->PID);
//         if (pdgid == ParticleID::p_eminus || pdgid == ParticleID::p_muminus) {
//           // use final version here!
//           lep = getFinal(dau);
//           break;
//         }
//       }
//       if (!lep)
//         throw std::logic_error("[FatJetMatching::top_label] Cannot find charged lepton from leptonic W decay!");

//       double dr_b = deltaR(jet, b_from_top);
//       double dr_l = deltaR(jet, lep);
//       if (debug_) {
//         using namespace std;
//         cout << "deltaR(jet, b)     : " << dr_b << endl;
//         cout << "deltaR(jet, l)     : " << dr_l << endl;
//         cout << "pdgid(l)           : " << lep->PID << endl;
//       }

//       if (dr_b < jetR_ && dr_l < jetR_) {
//         auto pdgid = std::abs(lep->PID);
//         if (pdgid == ParticleID::p_eminus) {
//           return std::make_pair(FatJetLabel::Top_ben, top);
//         } else if (pdgid == ParticleID::p_muminus) {
//           return std::make_pair(FatJetLabel::Top_bmn, top);
//         }
//       }
//     }

//     return std::make_pair(FatJetLabel::Invalid, nullptr);
//   }

//   std::pair<FatJetLabel, const GenParticle *> w_label(const Jet *jet, const GenParticle *parton) {
//     auto w = getFinal(parton);
//     if (isHadronic(w)) {
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::W;
//       }

//       if (debug_) {
//         using namespace std;
//         cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")"
//              << endl;
//         cout << "W:   ";
//         printGenParticleInfo(w, -1);
//       }

//       auto wdaus = getDaughterQuarks(w);
//       if (wdaus.size() < 2)
//         throw std::logic_error("[FatJetMatching::w_label] W decay has less than 2 quarks!");
//       //    if (wdaus.size() >= 2)
//       {
//         double dr_q1 = deltaR(jet, wdaus.at(0));
//         double dr_q2 = deltaR(jet, wdaus.at(1));
//         if (dr_q1 > dr_q2) {
//           // swap q1 and q2 so that dr_q1<=dr_q2
//           std::swap(dr_q1, dr_q2);
//           std::swap(wdaus.at(0), wdaus.at(1));
//         }
//         auto pdgid_q1 = std::abs(wdaus.at(0)->PID);
//         auto pdgid_q2 = std::abs(wdaus.at(1)->PID);

//         if (debug_) {
//           using namespace std;
//           cout << "deltaR(jet, q1)    : " << dr_q1 << endl;
//           cout << "deltaR(jet, q2)    : " << dr_q2 << endl;
//           cout << "pdgid(q1)        : " << pdgid_q1 << endl;
//           cout << "pdgid(q2)        : " << pdgid_q2 << endl;
//         }

//         if (dr_q1 < jetR_ && dr_q2 < jetR_) {
//           if (pdgid_q1 >= ParticleID::p_c || pdgid_q2 >= ParticleID::p_c) {
//             return std::make_pair(FatJetLabel::W_cq, w);
//           } else {
//             return std::make_pair(FatJetLabel::W_qq, w);
//           }
//         }
//       }
//     }

//     return std::make_pair(FatJetLabel::Invalid, nullptr);
//   }

//   std::pair<FatJetLabel, const GenParticle *> z_label(const Jet *jet, const GenParticle *parton) {
//     auto z = getFinal(parton);
//     if (isHadronic(z)) {
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::Z;
//       }

//       if (debug_) {
//         using namespace std;
//         cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")"
//              << endl;
//         cout << "Z:   ";
//         printGenParticleInfo(z, -1);
//       }

//       auto zdaus = getDaughterQuarks(z);
//       if (zdaus.size() < 2)
//         throw std::logic_error("[FatJetMatching::z_label] Z decay has less than 2 quarks!");
//       //    if (zdaus.size() >= 2)
//       {
//         double dr_q1 = deltaR(jet, zdaus.at(0));
//         double dr_q2 = deltaR(jet, zdaus.at(1));
//         if (dr_q1 > dr_q2) {
//           // swap q1 and q2 so that dr_q1<=dr_q2
//           std::swap(dr_q1, dr_q2);
//           std::swap(zdaus.at(0), zdaus.at(1));
//         }
//         auto pdgid_q1 = std::abs(zdaus.at(0)->PID);
//         auto pdgid_q2 = std::abs(zdaus.at(1)->PID);

//         if (debug_) {
//           using namespace std;
//           cout << "deltaR(jet, q1)    : " << dr_q1 << endl;
//           cout << "deltaR(jet, q2)    : " << dr_q2 << endl;
//           cout << "pdgid(q1)        : " << pdgid_q1 << endl;
//           cout << "pdgid(q2)        : " << pdgid_q2 << endl;
//         }

//         if (dr_q1 < jetR_ && dr_q2 < jetR_) {
//           if (pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_b) {
//             return std::make_pair(FatJetLabel::Z_bb, z);
//           } else if (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_c) {
//             return std::make_pair(FatJetLabel::Z_cc, z);
//           } else {
//             return std::make_pair(FatJetLabel::Z_qq, z);
//           }
//         }
//       }
//     }

//     return std::make_pair(FatJetLabel::Invalid, nullptr);
//   }

//   std::pair<FatJetLabel, const GenParticle *> higgs_label(const Jet *jet, const GenParticle *parton) {
//     auto higgs = getFinal(parton);
//     auto daus = getDaughters(higgs);

//     bool is_hvv = false;
//     if (daus.size() > 2) {
//       // e.g., h->Vqq or h->qqqq
//       is_hvv = true;
//     } else {
//       // e.g., h->VV*
//       for (const auto *p : daus) {
//         auto pdgid = std::abs(p->PID);
//         if (pdgid == ParticleID::p_Wplus || pdgid == ParticleID::p_Z0) {
//           is_hvv = true;
//           break;
//         }
//       }
//     }

//     if (is_hvv) {
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::Higgs;
//       }

//       // h->WW or h->ZZ
//       std::vector<const GenParticle *> hHH_quarks;
//       std::vector<const GenParticle *> hHH_leptons;
//       for (const auto *p : daus) {
//         auto pdgid = std::abs(p->PID);
//         if (pdgid >= ParticleID::p_d && pdgid <= ParticleID::p_b) {
//           hHH_quarks.push_back(p);
//         } else if (pdgid == ParticleID::p_eminus || pdgid == ParticleID::p_muminus) {
//           hHH_leptons.push_back(getFinal(p));
//         } else if (pdgid == ParticleID::p_Wplus || pdgid == ParticleID::p_Z0) {
//           auto v_daus = getDaughters(getFinal(p));
//           for (const auto *vdau : v_daus) {
//             auto pdgid = std::abs(vdau->PID);
//             if (pdgid >= ParticleID::p_d && pdgid <= ParticleID::p_b) {
//               hHH_quarks.push_back(vdau);
//             } else if (pdgid == ParticleID::p_eminus || pdgid == ParticleID::p_muminus) {
//               hHH_leptons.push_back(getFinal(vdau));
//             }
//           }
//         }
//       }

//       if (debug_) {
//         using namespace std;
//         cout << "Found " << hHH_quarks.size() << " quarks from Higgs decay" << endl;
//         for (const auto *gp : hHH_quarks) {
//           using namespace std;
//           printGenParticleInfo(gp, -1);
//           cout << " ... dR(q, jet) = " << deltaR(gp, jet) << endl;
//         }
//         cout << "Found " << hHH_leptons.size() << " leptons from Higgs decay" << endl;
//         for (const auto *gp : hHH_leptons) {
//           using namespace std;
//           printGenParticleInfo(gp, -1);
//           cout << " ... dR(lep, jet) = " << deltaR(gp, jet) << endl;
//         }
//       }

//       unsigned n_quarks_in_jet = 0;
//       for (const auto *gp : hHH_quarks) {
//         auto dr = deltaR(gp, jet);
//         if (dr < jetR_) {
//           ++n_quarks_in_jet;
//         }
//       }
//       unsigned n_leptons_in_jet = 0;
//       for (const auto *gp : hHH_leptons) {
//         auto dr = deltaR(gp, jet);
//         if (dr < jetR_) {
//           ++n_leptons_in_jet;
//         }
//       }

//       if (n_quarks_in_jet >= 4) {
//         return std::make_pair(FatJetLabel::H_ww4q, higgs);
//       } else if (n_quarks_in_jet == 2 && n_leptons_in_jet == 1) {
//         return std::make_pair(FatJetLabel::H_ww2q1l, higgs);
//       }
//     } else if (isHadronic(higgs, true)) {
//       // direct h->qq
//       if (event_type_ == EventType::QCD) {
//         event_type_ = EventType::Higgs;
//       }

//       if (debug_) {
//         using namespace std;
//         cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")"
//              << endl;
//         cout << "H:   ";
//         printGenParticleInfo(higgs, -1);
//       }

//       auto hdaus = getDaughterQuarks(higgs, true);
//       if (hdaus.size() < 2)
//         throw std::logic_error("[FatJetMatching::higgs_label] Higgs decay has less than 2 quarks!");
//       //    if (zdaus.size() >= 2)
//       {
//         double dr_q1 = deltaR(jet, hdaus.at(0));
//         double dr_q2 = deltaR(jet, hdaus.at(1));
//         if (dr_q1 > dr_q2) {
//           // swap q1 and q2 so that dr_q1<=dr_q2
//           std::swap(dr_q1, dr_q2);
//           std::swap(hdaus.at(0), hdaus.at(1));
//         }
//         auto pdgid_q1 = std::abs(hdaus.at(0)->PID);
//         auto pdgid_q2 = std::abs(hdaus.at(1)->PID);

//         if (debug_) {
//           using namespace std;
//           cout << "deltaR(jet, q1)    : " << dr_q1 << endl;
//           cout << "deltaR(jet, q2)    : " << dr_q2 << endl;
//           cout << "pdgid(q1)        : " << pdgid_q1 << endl;
//           cout << "pdgid(q2)        : " << pdgid_q2 << endl;
//         }

//         if (dr_q1 < jetR_ && dr_q2 < jetR_) {
//           if (pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_b) {
//             return std::make_pair(FatJetLabel::H_bb, higgs);
//           } else if (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_c) {
//             return std::make_pair(FatJetLabel::H_cc, higgs);
//           } else if (pdgid_q1 == ParticleID::p_g && pdgid_q2 == ParticleID::p_g) {
//             return std::make_pair(FatJetLabel::H_gg, higgs);
//           } else {
//             return std::make_pair(FatJetLabel::H_qq, higgs);
//           }
//         }
//       }
//     }

//     return std::make_pair(FatJetLabel::Invalid, nullptr);
//   }

private:

    void higgs_label(const Jet *jet, const GenParticle *parton) {

        auto higgs = getFinal(parton);
        getResult().resParticles.push_back(higgs);

        if (debug_){
            using namespace std;
            cout << "jet: (" << jet->PT << ", " << jet->Eta << ", " << jet->Phi << ", " << jet->P4().Energy() << ")" << endl;
            std::cout << "H:     "; printGenParticleInfo(higgs, -1);
        }

        enum HDecay {h_2p, h_tautau, h_qtau, h_HH, h_null};
        HDecay hdecay = h_null;
        auto hdaus = getDaughters(higgs);
        if (higgs->D2 - higgs->D1 + 1 >= 3) {
            // e.g., h->Vqq or h->qqqq
            throw std::runtime_error("[FatJetMatching::higgs_label] H decays to 3/4 objects: not implemented");
        }else {
            auto pdgid1 = std::abs(hdaus.at(0)->PID), pdgid2 = std::abs(hdaus.at(1)->PID);
            if ((pdgid1 == ParticleID::p_Hplus && pdgid2 == ParticleID::p_Hplus) || (pdgid1 == ParticleID::p_H0 && pdgid2 == ParticleID::p_H0)) {
                hdecay = h_HH;
            }else if (pdgid1 == ParticleID::p_tauminus && pdgid2 == ParticleID::p_tauminus) {
                hdecay = h_tautau;
            }else if (((pdgid1 >= ParticleID::p_u && pdgid1 <= ParticleID::p_b) && pdgid2 == ParticleID::p_tauminus) || 
                      ((pdgid2 >= ParticleID::p_u && pdgid2 <= ParticleID::p_b) && pdgid1 == ParticleID::p_tauminus)) {
                hdecay = h_qtau;
            }else {
                hdecay = h_2p;
            }
        }

        if (hdecay == h_HH){
            // h->H0H0 or H+H-
            enum VMode {h_had, h_lep, h_null};
            VMode hHH_modes[2] = {h_null, h_null};
            std::vector<const GenParticle*> hHH_daus;
            // found daughters of HH, and determine the their decay (had or lep),
            // then switch order to make sure had daughters go first
            for (int idau = higgs->D1; idau <= higgs->D2; ++idau) {
                int i = idau - higgs->D1;
                const auto *dau = genParticles_.at(idau);
                auto daufinal = getFinal(dau);
                getResult().resParticles.push_back(daufinal); // push the first daughter to the list (H0 or H+-)

                for (int jdau = daufinal->D1; jdau <= daufinal->D2; ++jdau){
                    int j = jdau - daufinal->D1;
                    const auto *ddau = genParticles_.at(jdau);
                    // determine the V decay mode
                    if (j == 0) {
                        auto dpdgid = std::abs(ddau->PID);
                        if ((dpdgid >= ParticleID::p_d && dpdgid <= ParticleID::p_b) || dpdgid == ParticleID::p_g)    hHH_modes[i] = h_had;
                        else    hHH_modes[i] = h_lep;
                    }
                    hHH_daus.push_back(ddau);
                }
            }
            if (hHH_modes[0] == h_lep && hHH_modes[1] == h_lep) {
                // require not both H are leptonic
                if (debug_)
                    std::cout << "Both H are leptonic, skip" << std::endl;
                return;
            }
            else if (hHH_modes[0] == h_lep && hHH_modes[1] == h_had) {
                // hadronic H always goes first
                std::swap(hHH_daus.at(0), hHH_daus.at(2));
                std::swap(hHH_daus.at(1), hHH_daus.at(3));
                hHH_modes[0] = h_had;
                hHH_modes[1] = h_lep;
            }

            // let e/mu/tau appears before neutrinos
            if (hHH_modes[1] == h_lep) {
                auto pdgid = std::abs(hHH_daus.at(2)->PID);
                if (pdgid == ParticleID::p_nu_e || pdgid == ParticleID::p_nu_mu || pdgid == ParticleID::p_nu_tau) {
                    std::swap(hHH_daus.at(2), hHH_daus.at(3));
                }
            }

            // go to dedicated h_HH
            // result.resParticles is completed; result.label and result.particles to be assigned inside those functions
            higgs_HH_label(jet, hHH_daus);
            return;

        }else if (hdecay == h_2p) {
            // direct h->qq

            if (hdaus.size() < 2) throw std::logic_error("[FatJetMatching::higgs_label] Higgs decay has less than 2 quarks!");
    //        if (zdaus.size() >= 2)
            {
                double dr_q1        = deltaR(jet, hdaus.at(0));
                double dr_q2        = deltaR(jet, hdaus.at(1));
                if (dr_q1 > dr_q2){
                    // swap q1 and q2 so that dr_q1<=dr_q2
                    std::swap(dr_q1, dr_q2);
                    std::swap(hdaus.at(0), hdaus.at(1));
                }
                auto pdgid_q1 = std::abs(hdaus.at(0)->PID);
                auto pdgid_q2 = std::abs(hdaus.at(1)->PID);

                if (debug_){
                    using namespace std;
                    cout << "deltaR(jet, q1)        : " << dr_q1 << endl;
                    cout << "deltaR(jet, q2)        : " << dr_q2 << endl;
                    cout << "pdgid(q1)                : " << pdgid_q1 << endl;
                    cout << "pdgid(q2)                : " << pdgid_q2 << endl;
                }

                if (dr_q1<jetR_ && dr_q2<jetR_){
                    getResult().particles.push_back(hdaus.at(0));
                    getResult().particles.push_back(hdaus.at(1));

                    if (pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_b) {
                        getResult().label = "H_bb";
                    }
                    else if (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_c) {
                        getResult().label = "H_cc";
                    }
                    else if (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_s) {
                        getResult().label = "H_ss";
                    }
                    else if ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) {
                        getResult().label = "H_qq";
                    }
                    else if ((pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_c) || (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_b)) {
                        getResult().label = "H_bc";
                    }
                    else if ((pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_s) || (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_b)) {
                        getResult().label = "H_bs"; // this does not happen in official case
                    }
                    else if ((pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_s) || (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_c)) {
                        getResult().label = "H_cs";
                    }
                    else if ((pdgid_q1 == ParticleID::p_b && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_b)) {
                        getResult().label = "H_bq";
                    }
                    else if ((pdgid_q1 == ParticleID::p_c && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_c)) {
                        getResult().label = "H_cq";
                    }
                    else if ((pdgid_q1 == ParticleID::p_s && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_s)) {
                        getResult().label = "H_sq";
                    }
                    else if (pdgid_q1 == ParticleID::p_g && pdgid_q2 == ParticleID::p_g) {
                        getResult().label = "H_gg";
                    }else if (pdgid_q1 == ParticleID::p_eminus && pdgid_q2 == ParticleID::p_eminus) {
                        getResult().label = "H_ee";
                    }else if (pdgid_q1 == ParticleID::p_muminus && pdgid_q2 == ParticleID::p_muminus) {
                        getResult().label = "H_mm";
                    }
                }
                return;
            }
        }else if (hdecay == h_tautau) {
            if (hdaus.size() == 2){
                // higgs -> tautau
                // use first version or last version of the tau in dr?
                double dr_tau1        = deltaR(jet, hdaus.at(0));
                double dr_tau2        = deltaR(jet, hdaus.at(1));

                auto tau1_daus_info = getTauDaughters(hdaus.at(0));
                auto tau2_daus_info = getTauDaughters(hdaus.at(1));

                if (debug_){
                    using namespace std;
                    cout << "tau1 decay ID: " << tau1_daus_info.second << endl;
                    cout << "tau2 decay ID: " << tau2_daus_info.second << endl;
                    cout << "deltaR(jet, tau1)        : " << dr_tau1 << endl;
                    cout << "deltaR(jet, tau1-dau): " << deltaR(tau1_daus_info.first.at(0), jet) << endl;
                    cout << "deltaR(jet, tau2)        : " << dr_tau2 << endl;
                    cout << "deltaR(jet, tau2-dau): " << deltaR(tau2_daus_info.first.at(0), jet) << endl;
                }

                // let hadronic tau be the first one
                if (tau1_daus_info.second < 2 && tau2_daus_info.second == 2){
                    std::swap(dr_tau1, dr_tau2);
                    std::swap(hdaus.at(0), hdaus.at(1));
                    std::swap(tau1_daus_info, tau2_daus_info);
                }
                // the first tau must be hadronic
                if (tau1_daus_info.second == 2 && dr_tau1 < jetR_){
                    // push the first tauh
                    getResult().particles.insert(getResult().particles.end(), tau1_daus_info.first.begin(), tau1_daus_info.first.end());

                    // inspect the second tau
                    if ((tau2_daus_info.second == 0 || tau2_daus_info.second == 1) && deltaR(tau2_daus_info.first.at(0), jet) < jetR_){
                        getResult().particles.push_back(tau2_daus_info.first.at(0));
                        if (tau2_daus_info.second == 0) {
                            getResult().label = "H_tauhtaue";
                        }else {
                            getResult().label = "H_tauhtaum";
                        }
                    }else if (tau2_daus_info.second == 2 && dr_tau2 < jetR_){
                        getResult().particles.insert(getResult().particles.end(), tau2_daus_info.first.begin(), tau2_daus_info.first.end());
                        getResult().label = "H_tauhtauh";
                    }
                }
                return;
            }
        }/*else if (hdecay == h_qtau) { // customized categories
            if (hdaus.size() == 2){
                // resonance -> qtau
                if (std::abs(hdaus.at(0)->PID) == ParticleID::p_tauminus){
                    std::swap(hdaus.at(0), hdaus.at(1));
                }
                double dr_q            = deltaR(jet, hdaus.at(0));
                double dr_tau        = deltaR(jet, hdaus.at(1));

                auto tau_daus_info = getTauDaughters(hdaus.at(1));

                if (debug_){
                    using namespace std;
                    cout << "tau decay ID: " << tau_daus_info.second << endl;
                    cout << "deltaR(jet, q)            : " << dr_q << endl;
                    cout << "deltaR(jet, tau)        : " << dr_tau << endl;
                    cout << "deltaR(jet, tau-dau): " << deltaR(tau_daus_info.first.at(0), jet) << endl;
                }

                if (dr_q < jetR_){
                    // check the quark type
                    std::string qtype = "";
                    if (std::abs(hdaus.at(0)->PID) == ParticleID::p_b) {
                        qtype = "b";
                    }else if (std::abs(hdaus.at(0)->PID) == ParticleID::p_c) {
                        qtype = "c";
                    }else if (std::abs(hdaus.at(0)->PID) == ParticleID::p_s) {
                        qtype = "s";
                    }else if (std::abs(hdaus.at(0)->PID) == ParticleID::p_u || std::abs(hdaus.at(0)->PID) == ParticleID::p_d) {
                        qtype = "q";
                    }else if (std::abs(hdaus.at(0)->PID) == ParticleID::p_g) {
                        qtype = "g";
                    }
                    // push the first quark
                    getResult().particles.push_back(hdaus.at(0));

                    // inspect the tau
                    if ((tau_daus_info.second == 0 || tau_daus_info.second == 1) && deltaR(tau_daus_info.first.at(0), jet) < jetR_){
                        getResult().particles.push_back(tau_daus_info.first.at(0));
                        if (tau_daus_info.second == 0) {
                            getResult().label = "Cust_" + qtype + "taue";
                        }else {
                            getResult().label = "Cust_" + qtype + "taum";
                        }
                    }else if (tau_daus_info.second == 2 && dr_tau < jetR_){
                        getResult().particles.insert(getResult().particles.end(), tau_daus_info.first.begin(), tau_daus_info.first.end());
                        getResult().label = "Cust_" + qtype + "tauh";
                    }
                }
                return;
            }
        }*/
    }

    void higgs_HH_label(const Jet* jet, std::vector<const GenParticle*>& hHH_daughters)
    {
        enum HDecay {H_bb, H_cc, H_ss, H_qq, H_bc, /*H_bs*,*/ H_cs, H_bq, H_cq, H_sq, H_gg, H_ee, H_mm, H_ev, H_mv, H_tauhtaue, H_tauhtaum, H_tauhtauh, H_tauev, H_taumv, H_tauhv, H_null};

        // determine the decay mode of two H (H0/H+-)
        HDecay hsdecay[2] = {H_null, H_null};
        int daus_matched[4] = {0, 0, 0, 0};
        for (int i=0; i<2; ++i) {
            const GenParticle* daus[2] = {hHH_daughters.at(i*2), hHH_daughters.at(i*2+1)};

            int pdgid_q1 = std::abs(daus[0]->PID);
            int pdgid_q2 = std::abs(daus[1]->PID);

            // non-tau cases
            if ((pdgid_q1 >= ParticleID::p_d && pdgid_q1 <= ParticleID::p_nu_mu) || pdgid_q1 == ParticleID::p_g) {
                
                if (pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_b) {
                    hsdecay[i] = H_bb;
                }
                else if (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_c) {
                    hsdecay[i] = H_cc;
                }
                else if (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_s) {
                    hsdecay[i] = H_ss;
                }
                else if ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) {
                    hsdecay[i] = H_qq;
                }
                else if ((pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_c) || (pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_b)) {
                    hsdecay[i] = H_bc;
                }
                // else if ((pdgid_q1 == ParticleID::p_b && pdgid_q2 == ParticleID::p_s) || (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_b)) {
                //     hsdecay[i] = H_bs; // this does not happen in official case
                // }
                else if ((pdgid_q1 == ParticleID::p_c && pdgid_q2 == ParticleID::p_s) || (pdgid_q1 == ParticleID::p_s && pdgid_q2 == ParticleID::p_c)) {
                    hsdecay[i] = H_cs;
                }
                else if ((pdgid_q1 == ParticleID::p_b && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_b)) {
                    hsdecay[i] = H_bq;
                }
                else if ((pdgid_q1 == ParticleID::p_c && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_c)) {
                    hsdecay[i] = H_cq;
                }
                else if ((pdgid_q1 == ParticleID::p_s && (pdgid_q2 == ParticleID::p_u || pdgid_q2 == ParticleID::p_d)) || ((pdgid_q1 == ParticleID::p_u || pdgid_q1 == ParticleID::p_d) && pdgid_q2 == ParticleID::p_s)) {
                    hsdecay[i] = H_sq;
                }
                else if (pdgid_q1 == ParticleID::p_g && pdgid_q2 == ParticleID::p_g) {
                    hsdecay[i] = H_gg;
                }else if (pdgid_q1 == ParticleID::p_eminus && pdgid_q2 == ParticleID::p_eminus) {
                    hsdecay[i] = H_ee;
                }else if (pdgid_q1 == ParticleID::p_muminus && pdgid_q2 == ParticleID::p_muminus) {
                    hsdecay[i] = H_mm;
                }else if (pdgid_q1 == ParticleID::p_eminus && pdgid_q2 == ParticleID::p_nu_e) {
                    hsdecay[i] = H_ev;
                }else if (pdgid_q1 == ParticleID::p_muminus && pdgid_q2 == ParticleID::p_nu_mu) {
                    hsdecay[i] = H_mv;
                }

                // check if the quarks are matched to the jet
                if (deltaR(jet, daus[0]) < jetR_) {
                    daus_matched[i*2] = 1;
                    getResult().particles.push_back(daus[0]);
                }
                if ((pdgid_q2 != ParticleID::p_nu_e && pdgid_q2 != ParticleID::p_nu_mu) && deltaR(jet, daus[1]) < jetR_) { // for neutrinos, always assign matched=0
                    daus_matched[i*2+1] = 1;
                    getResult().particles.push_back(daus[1]);
                }
            }else if (pdgid_q1 == ParticleID::p_tauminus && pdgid_q2 == ParticleID::p_tauminus) {
                // H->tautau

                auto tau1_daus_info = getTauDaughters(daus[0]);
                auto tau2_daus_info = getTauDaughters(daus[1]);

                // let hadronic tau be the first one
                if (tau1_daus_info.second < 2 && tau2_daus_info.second == 2){
                    std::swap(daus[0], daus[1]);
                    std::swap(tau1_daus_info, tau2_daus_info);
                }
                if (tau1_daus_info.second == 2){ // first tau must be tauh
                    if (tau2_daus_info.second == 0 || tau2_daus_info.second == 1){
                        if (tau2_daus_info.second == 0) {
                            hsdecay[i] = H_tauhtaue;
                        }else {
                            hsdecay[i] = H_tauhtaum;
                        }
                        // matching of the first tauh and the second taue/taum
                        if (deltaR(jet, daus[0]) < jetR_) {
                            daus_matched[i*2] = 1;
                            getResult().particles.insert(getResult().particles.end(), tau1_daus_info.first.begin(), tau1_daus_info.first.end());
                        }
                        if (deltaR(jet, tau2_daus_info.first.at(0)) < jetR_) {
                            daus_matched[i*2+1] = 1;
                            getResult().particles.push_back(tau2_daus_info.first.at(0));
                        }
                    }else if (tau2_daus_info.second == 2){
                        hsdecay[i] = H_tauhtauh;
                        // matching of both tauh
                        if (deltaR(jet, daus[0]) < jetR_) {
                            daus_matched[i*2] = 1;
                            getResult().particles.insert(getResult().particles.end(), tau1_daus_info.first.begin(), tau1_daus_info.first.end());
                        }
                        if (deltaR(jet, daus[1]) < jetR_) {
                            daus_matched[i*2+1] = 1;
                            getResult().particles.insert(getResult().particles.end(), tau2_daus_info.first.begin(), tau2_daus_info.first.end());
                        }
                    }
                }
            }else if (pdgid_q1 == ParticleID::p_tauminus && pdgid_q2 == ParticleID::p_nu_tau) {

                // H->tauv
                auto tau_daus_info = getTauDaughters(daus[0]);

                if (tau_daus_info.second == 0 || tau_daus_info.second == 1){
                    if (tau_daus_info.second == 0) {
                        hsdecay[i] = H_tauev;
                    }else {
                        hsdecay[i] = H_taumv;
                    }
                    // matching of the taue and taum, neutrinos considered unmatched
                    if (deltaR(jet, tau_daus_info.first.at(0)) < jetR_) {
                        daus_matched[i*2] = 1;
                        getResult().particles.push_back(tau_daus_info.first.at(0));
                    }
                } else if (tau_daus_info.second == 2){
                    hsdecay[i] = H_tauhv;
                    // matching of the tauh
                    if (deltaR(jet, daus[0]) < jetR_) {
                        daus_matched[i*2] = 1;
                        getResult().particles.insert(getResult().particles.end(), tau_daus_info.first.begin(), tau_daus_info.first.end());
                    }
                }
            }

            if (debug_){
                using namespace std;
                cout << "H_" << i+1 << " decay mode    :" << hsdecay[i] << endl;
                cout <<    "    pdgid(dau1)     : " << daus[0]->PID << endl;
                cout <<    "    pdgid(dau2)     : " << daus[1]->PID << endl;
                cout <<    "    deltaR(jet, dau1)    : " << deltaR(jet, daus[0]) << "    dau/tau-dau is matched: " << daus_matched[i*2] << endl;
                cout <<    "    deltaR(jet, dau2)    : " << deltaR(jet, daus[1]) << "    dau/tau-dau is matched: " << daus_matched[i*2+1] << endl;
                int jmax = 0;
                if (hsdecay[i] == H_tauhtaue || hsdecay[i] == H_tauhtaum || hsdecay[i] == H_tauhtauh)
                    jmax = 2;
                else if (hsdecay[i] == H_tauev || hsdecay[i] == H_taumv || hsdecay[i] == H_tauhv)
                    jmax = 1;
                for (int j=0; j<jmax; j++) {
                    auto tau_daus_info = getTauDaughters(daus[j]);
                    auto tau_daus = tau_daus_info.first;
                    int tau_decay = tau_daus_info.second;
                    cout << "    tau_" << j+1 << " :" << endl;
                    cout << "        tau decay mode: " << tau_decay << endl;
                    cout << "        tau daughters: " << endl;
                    for (auto dau : tau_daus) {
                        cout << "            pdgid: " << dau->PID << "    deltaR(jet, dau): " << deltaR(jet, dau) << endl;
                    }
                }
            }

        }
        if (hsdecay[0] == H_null || hsdecay[1] == H_null) {
            // other H modes will not be handled (e.g H->tauetaue)
            return;
        }
        if (hsdecay[1] == H_tauhtaue || hsdecay[1] == H_tauhtaum || hsdecay[1] == H_tauhtauh) {
            // requires both taus should be matched
            if (!daus_matched[2] || !daus_matched[3]) {
                return;
            }
        }
        if (daus_matched[0] + daus_matched[1] + daus_matched[2] + daus_matched[3] < 3) {
            // should have at least has three matched daughters to desinate the HH label
            return;
        }

        // make labels
        // be sure not using hHH_daughters following on (order not switched)

        std::string matched_parts_str = "";
        for (int i=0; i<2; ++i) {
            if (hsdecay[i] == H_bb || hsdecay[i] == H_cc || hsdecay[i] == H_ss || hsdecay[i] == H_qq || 
                hsdecay[i] == H_bc || hsdecay[i] == H_cs || hsdecay[i] == H_bq || hsdecay[i] == H_cq || hsdecay[i] == H_sq || 
                hsdecay[i] == H_gg || hsdecay[i] == H_ee || hsdecay[i] == H_mm || hsdecay[i] == H_ev || hsdecay[i] == H_mv) {
                for (int j=0; j<2; ++j) {
                    // only write to string if the daughter is matched (unless it is a neutrino)
                    int pid = std::abs(hHH_daughters.at(i*2 + j)->PID);
                    if (daus_matched[i*2 + j] || pid == ParticleID::p_nu_e || pid == ParticleID::p_nu_mu) {
                        if (pid == ParticleID::p_b)    matched_parts_str += "b";
                        else if (pid == ParticleID::p_c)    matched_parts_str += "c";
                        else if (pid == ParticleID::p_s)    matched_parts_str += "s";
                        else if (pid == ParticleID::p_u || pid == ParticleID::p_d)    matched_parts_str += "q";
                        else if (pid == ParticleID::p_g)    matched_parts_str += "g";
                        else if (pid == ParticleID::p_eminus)    matched_parts_str += "e";
                        else if (pid == ParticleID::p_muminus)    matched_parts_str += "m";
                        else if (pid == ParticleID::p_nu_e || pid == ParticleID::p_nu_mu)    matched_parts_str += "v";
                    }
                }
            }
            else if (hsdecay[i] == H_tauhtaue || hsdecay[i] == H_tauhtaum || hsdecay[i] == H_tauhtauh) {
                assert(daus_matched[i*2] && daus_matched[i*2+1]);
                if (hsdecay[i] == H_tauhtaue)  matched_parts_str += "tauhtaue";
                else if (hsdecay[i] == H_tauhtaum)    matched_parts_str += "tauhtaum";
                else if (hsdecay[i] == H_tauhtauh)    matched_parts_str += "tauhtauh";
            }
            else if (hsdecay[i] == H_tauev || hsdecay[i] == H_taumv || hsdecay[i] == H_tauhv) {
                assert(daus_matched[i*2]); // because the neutrino is not matched, but there are at least three matching daughters
                if (hsdecay[i] == H_tauev)  matched_parts_str += "tauev";
                else if (hsdecay[i] == H_taumv)    matched_parts_str += "taumv";
                else if (hsdecay[i] == H_tauhv)    matched_parts_str += "tauhv";
            }
        }
        // desinate the label
        std::map<std::vector<std::string>, std::string> acceptable_strs_map = {

            // H0H0 cases (some can also be from H+H-)
            //   the first H0 -> QQ/ee/mm/tautau is all maatched
            {{"bbbb"}, "H_AA_bbbb"}, {{"bbcc", "bcbc", "bccb", "cbbc", "cbcb", "ccbb"}, "H_AA_bbcc"}, {{"bbss", "bsbs", "bssb", "sbbs", "sbsb", "ssbb"}, "H_AA_bbss"}, {{"bbqq", "bqbq", "bqqb", "qbbq", "qbqb", "qqbb"}, "H_AA_bbqq"}, {{"bbgg", "ggbb"}, "H_AA_bbgg"}, {{"bbee", "eebb"}, "H_AA_bbee"}, {{"bbmm", "mmbb"}, "H_AA_bbmm"},
            {{"bbtauhtaue", "tauhtauebb"}, "H_AA_bbtauhtaue"}, {{"bbtauhtaum", "tauhtaumbb"}, "H_AA_bbtauhtaum"}, {{"bbtauhtauh", "tauhtauhbb"}, "H_AA_bbtauhtauh"},

            {{"bbb"}, "H_AA_bbb"}, {{"bbc", "bcb", "cbb"}, "H_AA_bbc"}, {{"bbs", "bsb", "sbb"}, "H_AA_bbs"}, {{"bbq", "bqb", "qbb"}, "H_AA_bbq"}, {{"bbg", "gbb"}, "H_AA_bbg"}, {{"bbe", "ebb"}, "H_AA_bbe"}, {{"bbm", "mbb"}, "H_AA_bbm"},

            {{"cccc"}, "H_AA_cccc"}, {{"ccss", "cscs", "cssc", "sccs", "scsc", "sscc"}, "H_AA_ccss"}, {{"ccqq", "cqcq", "cqqc", "qccq", "qcqc", "qqcc"}, "H_AA_ccqq"}, {{"ccgg", "ggcc"}, "H_AA_ccgg"}, {{"ccee", "eecc"}, "H_AA_ccee"}, {{"ccmm", "mmcc"}, "H_AA_ccmm"},
            {{"cctauhtaue", "tauhtauecc"}, "H_AA_cctauhtaue"}, {{"cctauhtaum", "tauhtaumcc"}, "H_AA_cctauhtaum"}, {{"cctauhtauh", "tauhtauhcc"}, "H_AA_cctauhtauh"},

            {{"ccb", "cbc", "bcc"}, "H_AA_ccb"}, {{"ccc"}, "H_AA_ccc"}, {{"ccs", "csc", "scc"}, "H_AA_ccs"}, {{"ccq", "cqc", "qcc"}, "H_AA_ccq"}, {{"ccg", "gcc"}, "H_AA_ccg"}, {{"cce", "ecc"}, "H_AA_cce"}, {{"ccm", "mcc"}, "H_AA_ccm"},

            {{"ssss"}, "H_AA_ssss"}, {{"ssqq", "sqsq", "sqqs", "qssq", "qsqs", "qqss"}, "H_AA_ssqq"}, {{"ssgg", "ggss"}, "H_AA_ssgg"}, {{"ssee", "eess"}, "H_AA_ssee"}, {{"ssmm", "mmss"}, "H_AA_ssmm"},
            {{"sstauhtaue", "tauhtauess"}, "H_AA_sstauhtaue"}, {{"sstauhtaum", "tauhtaumss"}, "H_AA_sstauhtaum"}, {{"sstauhtauh", "tauhtauhss"}, "H_AA_sstauhtauh"},

            {{"ssb", "sbs", "bss"}, "H_AA_ssb"}, {{"ssc", "scs", "css"}, "H_AA_ssc"}, {{"sss"}, "H_AA_sss"}, {{"ssq", "sqs", "qss"}, "H_AA_ssq"}, {{"ssg", "gss"}, "H_AA_ssg"}, {{"sse", "ess"}, "H_AA_sse"}, {{"ssm", "mss"}, "H_AA_ssm"},

            {{"qqqq"}, "H_AA_qqqq"}, {{"qqgg", "ggqq"}, "H_AA_qqgg"}, {{"qqee", "eeqq"}, "H_AA_qqee"}, {{"qqmm", "mmqq"}, "H_AA_qqmm"},
            {{"qqtauhtaue", "tauhtaueqq"}, "H_AA_qqtauhtaue"}, {{"qqtauhtaum", "tauhtaumqq"}, "H_AA_qqtauhtaum"}, {{"qqtauhtauh", "tauhtauhqq"}, "H_AA_qqtauhtauh"},

            {{"qqb", "qbq", "bqq"}, "H_AA_qqb"}, {{"qqc", "qcq", "cqq"}, "H_AA_qqc"}, {{"qqs", "qsq", "sqq"}, "H_AA_qqs"}, {{"qqq"}, "H_AA_qqq"}, {{"qqg", "gqq"}, "H_AA_qqg"}, {{"qqe", "eqq"}, "H_AA_qqe"}, {{"qqm", "mqq"}, "H_AA_qqm"},

            {{"gggg"}, "H_AA_gggg"}, {{"ggee", "eegg"}, "H_AA_ggee"}, {{"ggmm", "mmgg"}, "H_AA_ggmm"},
            {{"ggtauhtaue", "tauhtauegg"}, "H_AA_ggtauhtaue"}, {{"ggtauhtaum", "tauhtaumgg"}, "H_AA_ggtauhtaum"}, {{"ggtauhtauh", "tauhtauhgg"}, "H_AA_ggtauhtauh"},

            {{"ggb", "bgg"}, "H_AA_ggb"}, {{"ggc", "cgg"}, "H_AA_ggc"}, {{"ggs", "sgg"}, "H_AA_ggs"}, {{"ggq", "qgg"}, "H_AA_ggq"}, {{"ggg"}, "H_AA_ggg"}, {{"gge", "egg"}, "H_AA_gge"}, {{"ggm", "mgg"}, "H_AA_ggm"},

            {{"eeb", "bee"}, "H_AA_bee"}, {{"eec", "cee"}, "H_AA_cee"}, {{"ees", "see"}, "H_AA_see"}, {{"eeq", "qee"}, "H_AA_qee"}, {{"eeg", "gee"}, "H_AA_gee"},
            {{"mmb", "bmm"}, "H_AA_bmm"}, {{"mmc", "cmm"}, "H_AA_cmm"}, {{"mms", "smm"}, "H_AA_smm"}, {{"mmq", "qmm"}, "H_AA_qmm"}, {{"mmg", "gmm"}, "H_AA_gmm"},

            {{"tauhtaueb", "btauhtaue"}, "H_AA_btauhtaue"}, {{"tauhtauec", "ctauhtaue"}, "H_AA_ctauhtaue"}, {{"tauhtaues", "stauhtaue"}, "H_AA_stauhtaue"}, {{"tauhtaueq", "qtauhtaue"}, "H_AA_qtauhtaue"}, {{"tauhtaueg", "gtauhtaue"}, "H_AA_gtauhtaue"},
            {{"tauhtaumb", "btauhtaum"}, "H_AA_btauhtaum"}, {{"tauhtaumc", "ctauhtaum"}, "H_AA_ctauhtaum"}, {{"tauhtaums", "stauhtaum"}, "H_AA_stauhtaum"}, {{"tauhtaumq", "qtauhtaum"}, "H_AA_qtauhtaum"}, {{"tauhtaumg", "gtauhtaum"}, "H_AA_gtauhtaum"},
            {{"tauhtauhb", "btauhtauh"}, "H_AA_btauhtauh"}, {{"tauhtauhc", "ctauhtauh"}, "H_AA_ctauhtauh"}, {{"tauhtauhs", "stauhtauh"}, "H_AA_stauhtauh"}, {{"tauhtauhq", "qtauhtauh"}, "H_AA_qtauhtauh"}, {{"tauhtauhg", "gtauhtauh"}, "H_AA_gtauhtauh"},

            // H+H- case
            {{"qqbq", "qqqb", "bqqq", "qbqq"}, "H_AA_qqqb"}, {{"qqcq", "qqqc", "cqqq", "qcqq"}, "H_AA_qqqc"}, {{"qqsq", "qqqs", "sqqq", "qsqq"}, "H_AA_qqqs"},
            {{"bbcs", "bbsc", "bcbs", "bcsb", "bsbc", "bscb", "cbbs", "cbsb", "csbb", "sbbc", "sbcb", "scbb"}, "H_AA_bbcs"}, {{"bbcq", "bbqc", "bcbq", "bcqb", "bqbc", "bqcb", "cbbq", "cbqb", "cqbb", "qbbc", "qbcb", "qcbb"}, "H_AA_bbcq"}, {{"bbsq", "bbqs", "bsbq", "bsqb", "bqbs", "bqsb", "sbbq", "sbqb", "sqbb", "qbbs", "qbsb", "qsbb"}, "H_AA_bbsq"},
            {{"ccbs", "ccsb", "cbcs", "cbsc", "cscb", "csbc", "bccs", "bcsc", "bscc", "sccb", "scbc", "sbcc"}, "H_AA_ccbs"}, {{"ccbq", "ccqb", "cbcq", "cbqc", "cqcb", "cqbc", "bccq", "bcqc", "bqcc", "qccb", "qcbc", "qbcc"}, "H_AA_ccbq"}, {{"ccsq", "ccqs", "cscq", "csqc", "cqcs", "cqsc", "sccq", "scqc", "sqcc", "qccs", "qcsc", "qscc"}, "H_AA_ccsq"},
            {{"ssbc", "sscb", "sbsc", "sbcs", "scsb", "scbs", "bssc", "bscs", "bcss", "cssb", "csbs", "cbss"}, "H_AA_ssbc"}, {{"ssbq", "ssqb", "sbsq", "sbqs", "sqsb", "sqbs", "bssq", "bsqs", "bqss", "qssb", "qsbs", "qbss"}, "H_AA_ssbq"}, {{"sscq", "ssqc", "scsq", "scqs", "sqsc", "sqcs", "cssq", "csqs", "cqss", "qssc", "qscs", "qcss"}, "H_AA_sscq"},
            {{"qqbc", "qqcb", "qbqc", "qbcq", "qcqb", "qcbq", "bqqc", "bqcq", "bcqq", "cqqb", "cqbq", "cbqq"}, "H_AA_qqbc"}, {{"qqbs", "qqsb", "qbqs", "qbsq", "qsqb", "qsbq", "bqqs", "bqsq", "bsqq", "sqqb", "sqbq", "sbqq"}, "H_AA_qqbs"}, {{"qqcs", "qqsc", "qcqs", "qcsq", "qsqc", "qscq", "cqqs", "cqsq", "csqq", "sqqc", "sqcq", "scqq"}, "H_AA_qqcs"},
            {{"bcsq", "bcqs", "bscq", "bsqc", "bqcs", "bqsc", "cbsq", "cbqs", "csbq", "csqb", "cqbs", "cqsb", "scbq", "scqb", "sbcq", "sbqc", "sqcb", "sqbc", "qcsb", "qcbs", "qscb", "qsbc", "qbcs", "qbsc"}, "H_AA_bcsq"},

            {{"bcs", "bsc", "cbs", "csb", "sbc", "scb"}, "H_AA_bcs"}, {{"bcq", "bqc", "cbq", "cqb", "qbc", "qcb"}, "H_AA_bcq"}, {{"bsq", "bqs", "sbq", "sqb", "qbs", "qsb"}, "H_AA_bsq"}, {{"csq", "cqs", "scq", "sqc", "qcs", "qsc"}, "H_AA_csq"}, 

            {{"bcev", "cbev", "evbc", "evcb"}, "H_AA_bcev"}, {{"csev", "scev", "evcs", "evsc"}, "H_AA_csev"}, {{"bqev", "qbev", "evbq", "evqb"}, "H_AA_bqev"}, {{"cqev", "qcev", "evcq", "evqc"}, "H_AA_cqev"}, {{"sqev", "qsev", "evsq", "evqs"}, "H_AA_sqev"}, {{"qqev", "evqq"}, "H_AA_qqev"},
            {{"bcmv", "cbmv", "mvbc", "mvcb"}, "H_AA_bcmv"}, {{"csmv", "scmv", "mvcs", "mvsc"}, "H_AA_csmv"}, {{"bqmv", "qbmv", "mvbq", "mvqb"}, "H_AA_bqmv"}, {{"cqmv", "qcmv", "mvcq", "mvqc"}, "H_AA_cqmv"}, {{"sqmv", "qsmv", "mvsq", "mvqs"}, "H_AA_sqmv"}, {{"qqmv", "mvqq"}, "H_AA_qqmv"},
            {{"bctauev", "cbtauev", "tauevbc", "tauevcb"}, "H_AA_bctauev"}, {{"cstauev", "sctauev", "tauevcs", "tauevsc"}, "H_AA_cstauev"}, {{"bqtauev", "qbtauev", "tauevbq", "tauevqb"}, "H_AA_bqtauev"}, {{"cqtauev", "qctauev", "tauevcq", "tauevqc"}, "H_AA_cqtauev"}, {{"sqtauev", "qstauev", "tauevsq", "tauevqs"}, "H_AA_sqtauev"}, {{"qqtauev", "tauevqq"}, "H_AA_qqtauev"},
            {{"bctaumv", "cbtaumv", "taumvbc", "taumvcb"}, "H_AA_bctaumv"}, {{"cstaumv", "sctaumv", "taumvcs", "taumvsc"}, "H_AA_cstaumv"}, {{"bqtaumv", "qbtaumv", "taumvbq", "taumvqb"}, "H_AA_bqtaumv"}, {{"cqtaumv", "qctaumv", "taumvcq", "taumvqc"}, "H_AA_cqtaumv"}, {{"sqtaumv", "qstaumv", "taumvsq", "taumvqs"}, "H_AA_sqtaumv"}, {{"qqtaumv", "taumvqq"}, "H_AA_qqtaumv"},
            {{"bctauhv", "cbtauhv", "tauhvbc", "tauhvcb"}, "H_AA_bctauhv"}, {{"cstauhv", "sctauhv", "tauhvcs", "tauhvsc"}, "H_AA_cstauhv"}, {{"bqtauhv", "qbtauhv", "tauhvbq", "tauhvqb"}, "H_AA_bqtauhv"}, {{"cqtauhv", "qctauhv", "tauhvcq", "tauhvqc"}, "H_AA_cqtauhv"}, {{"sqtauhv", "qstauhv", "tauhvsq", "tauhvqs"}, "H_AA_sqtauhv"}, {{"qqtauhv", "tauhvqq"}, "H_AA_qqtauhv"},
        };

        for (auto it = acceptable_strs_map.begin(); it != acceptable_strs_map.end(); ++it) {
            auto& acceptable_strs = it->first;
            std::string& label = it->second;

            // iterate over all string options
            for (auto it_str = acceptable_strs.begin(); it_str != acceptable_strs.end(); ++it_str) {
                if (matched_parts_str == (*it_str)) {
                    getResult().label = label;
                    return;
                }
            }
        }
        throw std::logic_error("[FatJetMatching::higgs_HH_label] Unmatched HH label: " + matched_parts_str);
    }

    void qcd_label(const Jet* jet) {

        int n_b=0, n_c=0, n_s=0;
        for (const auto *gp : genParticles_) {
            int pdgid = std::abs(gp->PID);
            // select quarks right before hadronization (status=71) and pT > 10
            if (pdgid >= ParticleID::p_d && pdgid <= ParticleID::p_b && gp->Status == 71 && gp->PT > 10 && deltaR(jet, gp) < jetR_) {
                
                if (debug_) {
                    std::cout << "Matched quark "; printGenParticleInfo(gp, -1);
                }
                if (pdgid == ParticleID::p_b)    n_b++;
                else if (pdgid == ParticleID::p_c)    n_c++;
                else if (pdgid == ParticleID::p_s)    n_s++;
            }
        }
        if (n_b>=2 && n_c>=2 && n_s>=2)  getResult().label = "QCD_bbccss";
        else if (n_b>=2 && n_c>=2 && n_s==1)  getResult().label = "QCD_bbccs";
        else if (n_b>=2 && n_c>=2 && n_s==0)  getResult().label = "QCD_bbcc";
        else if (n_b>=2 && n_c==1 && n_s>=2)  getResult().label = "QCD_bbcss";
        else if (n_b>=2 && n_c==1 && n_s==1)  getResult().label = "QCD_bbcs";
        else if (n_b>=2 && n_c==1 && n_s==0)  getResult().label = "QCD_bbc";
        else if (n_b>=2 && n_c==0 && n_s>=2)  getResult().label = "QCD_bbss";
        else if (n_b>=2 && n_c==0 && n_s==1)  getResult().label = "QCD_bbs";
        else if (n_b>=2 && n_c==0 && n_s==0)  getResult().label = "QCD_bb";
        else if (n_b==1 && n_c>=2 && n_s>=2)  getResult().label = "QCD_bccss";
        else if (n_b==1 && n_c>=2 && n_s==1)  getResult().label = "QCD_bccs";
        else if (n_b==1 && n_c>=2 && n_s==0)  getResult().label = "QCD_bcc";
        else if (n_b==1 && n_c==1 && n_s>=2)  getResult().label = "QCD_bcss";
        else if (n_b==1 && n_c==1 && n_s==1)  getResult().label = "QCD_bcs";
        else if (n_b==1 && n_c==1 && n_s==0)  getResult().label = "QCD_bc";
        else if (n_b==1 && n_c==0 && n_s>=2)  getResult().label = "QCD_bss";
        else if (n_b==1 && n_c==0 && n_s==1)  getResult().label = "QCD_bs";
        else if (n_b==1 && n_c==0 && n_s==0)  getResult().label = "QCD_b";
        else if (n_b==0 && n_c>=2 && n_s>=2)  getResult().label = "QCD_ccss";
        else if (n_b==0 && n_c>=2 && n_s==1)  getResult().label = "QCD_ccs";
        else if (n_b==0 && n_c>=2 && n_s==0)  getResult().label = "QCD_cc";
        else if (n_b==0 && n_c==1 && n_s>=2)  getResult().label = "QCD_css";
        else if (n_b==0 && n_c==1 && n_s==1)  getResult().label = "QCD_cs";
        else if (n_b==0 && n_c==1 && n_s==0)  getResult().label = "QCD_c";
        else if (n_b==0 && n_c==0 && n_s>=2)  getResult().label = "QCD_ss";
        else if (n_b==0 && n_c==0 && n_s==1)  getResult().label = "QCD_s";
        else if (n_b==0 && n_c==0 && n_s==0)  getResult().label = "QCD_light";

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
      throw std::invalid_argument("[FatJetMatching::isHadronic()] Null particle!");
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
  double jetR_ = 1.5;
  bool assignQCDLabel_ = false;
  bool debug_ = false;
  std::vector<const GenParticle *> genParticles_;
  std::unordered_set<const GenParticle *> processed_;
  EventType event_type_ = EventType::QCD;
  FatJetMatchingResult result_{"Invalid", std::vector<const GenParticle*>(), std::vector<const GenParticle*>()};

  std::vector<std::string> labels_{
    // H->2 prong
    "H_bb", "H_cc", "H_ss", "H_qq", "H_bc", /*"H_bs",*/ "H_cs", "H_bq", "H_cq", "H_sq", "H_gg", "H_ee", "H_mm", "H_tauhtaue", "H_tauhtaum", "H_tauhtauh", "H_tauev", "H_taumv", "H_tauhv",

    // H->4/3 prong
    "H_AA_bbbb", "H_AA_bbcc", "H_AA_bbss", "H_AA_bbqq", "H_AA_bbgg", "H_AA_bbee", "H_AA_bbmm",
    "H_AA_bbtauhtaue", "H_AA_bbtauhtaum", "H_AA_bbtauhtauh",

    "H_AA_bbb", "H_AA_bbc", "H_AA_bbs", "H_AA_bbq", "H_AA_bbg", "H_AA_bbe", "H_AA_bbm",

    "H_AA_cccc", "H_AA_ccss", "H_AA_ccqq", "H_AA_ccgg", "H_AA_ccee", "H_AA_ccmm",
    "H_AA_cctauhtaue", "H_AA_cctauhtaum", "H_AA_cctauhtauh",

    "H_AA_ccb", "H_AA_ccc", "H_AA_ccs", "H_AA_ccq", "H_AA_ccg", "H_AA_cce", "H_AA_ccm",

    "H_AA_ssss", "H_AA_ssqq", "H_AA_ssgg", "H_AA_ssee", "H_AA_ssmm",
    "H_AA_sstauhtaue", "H_AA_sstauhtaum", "H_AA_sstauhtauh",

    "H_AA_ssb", "H_AA_ssc", "H_AA_sss", "H_AA_ssq", "H_AA_ssg", "H_AA_sse", "H_AA_ssm",

    "H_AA_qqqq", "H_AA_qqgg", "H_AA_qqee", "H_AA_qqmm",
    "H_AA_qqtauhtaue", "H_AA_qqtauhtaum", "H_AA_qqtauhtauh",

    "H_AA_qqb", "H_AA_qqc", "H_AA_qqs", "H_AA_qqq", "H_AA_qqg", "H_AA_qqe", "H_AA_qqm",

    "H_AA_gggg", "H_AA_ggee", "H_AA_ggmm",
    "H_AA_ggtauhtaue", "H_AA_ggtauhtaum", "H_AA_ggtauhtauh",

    "H_AA_ggb", "H_AA_ggc", "H_AA_ggs", "H_AA_ggq", "H_AA_ggg", "H_AA_gge", "H_AA_ggm",

    "H_AA_bee", "H_AA_cee", "H_AA_see", "H_AA_qee", "H_AA_gee",
    "H_AA_bmm", "H_AA_cmm", "H_AA_smm", "H_AA_qmm", "H_AA_gmm",

    "H_AA_btauhtaue", "H_AA_ctauhtaue", "H_AA_stauhtaue", "H_AA_qtauhtaue", "H_AA_gtauhtaue",
    "H_AA_btauhtaum", "H_AA_ctauhtaum", "H_AA_stauhtaum", "H_AA_qtauhtaum", "H_AA_gtauhtaum",
    "H_AA_btauhtauh", "H_AA_ctauhtauh", "H_AA_stauhtauh", "H_AA_qtauhtauh", "H_AA_gtauhtauh",

    // (H+H-: H_AA_bbcs, H_AA_bbsq, H_AA_ssbc, H_AA_ssbq not available)
    "H_AA_qqqb", "H_AA_qqqc", "H_AA_qqqs",
    "H_AA_bbcq",
    "H_AA_ccbs", "H_AA_ccbq", "H_AA_ccsq",
    "H_AA_sscq",
    "H_AA_qqbc", "H_AA_qqbs", "H_AA_qqcs",
    "H_AA_bcsq",

    "H_AA_bcs", "H_AA_bcq", "H_AA_bsq", "H_AA_csq", 

    "H_AA_bcev", "H_AA_csev", "H_AA_bqev", "H_AA_cqev", "H_AA_sqev", "H_AA_qqev",
    "H_AA_bcmv", "H_AA_csmv", "H_AA_bqmv", "H_AA_cqmv", "H_AA_sqmv", "H_AA_qqmv",
    "H_AA_bctauev", "H_AA_cstauev", "H_AA_bqtauev", "H_AA_cqtauev", "H_AA_sqtauev", "H_AA_qqtauev",
    "H_AA_bctaumv", "H_AA_cstaumv", "H_AA_bqtaumv", "H_AA_cqtaumv", "H_AA_sqtaumv", "H_AA_qqtaumv",
    "H_AA_bctauhv", "H_AA_cstauhv", "H_AA_bqtauhv", "H_AA_cqtauhv", "H_AA_sqtauhv", "H_AA_qqtauhv",

    // QCD
    "QCD_bbccss", "QCD_bbccs", "QCD_bbcc", "QCD_bbcss", "QCD_bbcs", "QCD_bbc", "QCD_bbss", "QCD_bbs", "QCD_bb",
    "QCD_bccss", "QCD_bccs", "QCD_bcc", "QCD_bcss", "QCD_bcs", "QCD_bc", "QCD_bss", "QCD_bs", "QCD_b",
    "QCD_ccss", "QCD_ccs", "QCD_cc", "QCD_css", "QCD_cs", "QCD_c", "QCD_ss", "QCD_s", "QCD_light",

    // custom labels
    "H_bs",
  };
};

#endif