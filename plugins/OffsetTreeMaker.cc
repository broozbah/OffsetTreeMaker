// -*- C++ -*-
//
// Package:    treemaker/OffsetTreeMaker
// Class:      OffsetTreeMaker
// 
/**\class OffsetTreeMaker OffsetTreeMaker.cc treemaker/OffsetTreeMaker/plugins/OffsetTreeMaker.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  charles harrington
//         Created:  Mon, 09 Nov 2015 17:09:43 GMT
//
//

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/Common/interface/Ref.h"
#include <SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h>
#include "DataFormats/JetReco/interface/PFJet.h"
#include "parsePileUpJSON2.h"
#include <vector>

//root files
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TRandom3.h>

using namespace std;

const int ETA_BINS = 82;
const int MAXNPV = 50;
const int MAXJETS = 4;

float etabins[ETA_BINS+1] =
  {-5.191, -4.889, -4.716, -4.538, -4.363, -4.191, -4.013, -3.839, -3.664, -3.489, -3.314, -3.139, -2.964, -2.853, -2.65,
   -2.5, -2.322, -2.172, -2.043, -1.93, -1.83, -1.74, -1.653, -1.566, -1.479, -1.392, -1.305, -1.218, -1.131, -1.044, -0.957,
   -0.879, -0.783, -0.696, -0.609, -0.522, -0.435, -0.348, -0.261, -0.174, -0.087, 0,
   0.087, 0.174, 0.261, 0.348, 0.435, 0.522, 0.609, 0.696, 0.783, 0.879, 0.957, 1.044, 1.131, 1.218, 1.305, 1.392, 1.479,
   1.566, 1.653, 1.74, 1.83, 1.93, 2.043, 2.172, 2.322, 2.5, 2.65, 2.853, 2.964, 3.139, 3.314, 3.489, 3.664, 3.839, 4.013,
   4.191, 4.363, 4.538, 4.716, 4.889, 5.191};

class OffsetTreeMaker : public edm::EDAnalyzer {
  public:
    explicit OffsetTreeMaker(const edm::ParameterSet&);

  private:
    virtual void beginJob() override;
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override;
    int getEtaIndex(float eta);
    enum Flavor{
      chm = 0, chu, nh, ne, hfh, hfe, lep, untrk, numFlavors, X //undefined
    };
    Flavor getFlavor(reco::PFCandidate::ParticleType id);

    int counter;
    TFile* root_file;
    TTree* tree;

    TH1F* h;
    TRandom3* rand;

    int nEta;
    float energy[ETA_BINS], eRMS[ETA_BINS], et[ETA_BINS];
    UChar_t f[numFlavors][ETA_BINS];  //energy fraction by flavor

    float weight;

    ULong64_t event;
    int run, lumi, bx;
    float mu;
    float rho, rhoC0, rhoCC;

    int nPVall, nPV;
    float pv_ndof[MAXNPV], pv_z[MAXNPV], pv_rho[MAXNPV];

    float ht;
    int nJets;
    float jet_eta[MAXJETS], jet_phi[MAXJETS], jet_pt[MAXJETS], jet_area[MAXJETS];
    float jet_ch[MAXJETS], jet_nh[MAXJETS], jet_ne[MAXJETS], jet_hfh[MAXJETS], jet_hfe[MAXJETS], jet_lep[MAXJETS];

    TString RootFileName_;
    int numSkip_;
    bool isMC_, reweight_;

    edm::EDGetTokenT< vector<reco::Vertex> > pvTag_;
    edm::EDGetTokenT< vector<reco::Track> > trackTag_;
    edm::EDGetTokenT< vector<PileupSummaryInfo> > muTag_;
    edm::EDGetTokenT< vector<reco::PFCandidate> > pfTag_;
    edm::EDGetTokenT<double> rhoTag_;
    edm::EDGetTokenT<double> rhoC0Tag_;
    edm::EDGetTokenT<double> rhoCCTag_;
    edm::EDGetTokenT< vector<reco::PFJet> > pfJetTag_;
    edm::EDGetTokenT< vector<reco::PFJet> > corrPfJetTag_;
};

OffsetTreeMaker::OffsetTreeMaker(const edm::ParameterSet& iConfig)
{
  numSkip_ = iConfig.getParameter<int> ("numSkip");
  RootFileName_ = iConfig.getParameter<string>("RootFileName");
  isMC_ = iConfig.getParameter<bool>("isMC");
  reweight_ = iConfig.getParameter<bool> ("reweight");
  pvTag_ = consumes< vector<reco::Vertex> >( iConfig.getParameter<edm::InputTag>("pvTag") );
  trackTag_ = consumes< vector<reco::Track> >( iConfig.getParameter<edm::InputTag>("trackTag") );
  muTag_ = consumes< vector<PileupSummaryInfo> >( iConfig.getParameter<edm::InputTag>("muTag") );
  pfTag_ = consumes< vector<reco::PFCandidate> >( iConfig.getParameter<edm::InputTag>("pfTag") );
  rhoTag_ = consumes<double>( iConfig.getParameter<edm::InputTag>("rhoTag") );
  rhoC0Tag_ = consumes<double>( iConfig.getParameter<edm::InputTag>("rhoC0Tag") );
  rhoCCTag_ = consumes<double>( iConfig.getParameter<edm::InputTag>("rhoCCTag") );
  pfJetTag_ = consumes< vector<reco::PFJet> >( iConfig.getParameter<edm::InputTag>("pfJetTag") );
  corrPfJetTag_ = consumes< vector<reco::PFJet> >( iConfig.getParameter<edm::InputTag>("corrPfJetTag") );
}

// ------------ method called once each job just before starting event loop  ------------
void  OffsetTreeMaker::beginJob() {

  root_file = new TFile(RootFileName_,"RECREATE");
  tree = new TTree("T","Offset Tree");

  counter = -1;
  h = new TH1F("mu", "mu", 100, 0, 50);
  rand = new TRandom3;

  if (isMC_ && reweight_)
    tree->Branch("weight", &weight, "weight/F");

  if (!isMC_){
    parsePileUpJSON2();

    tree->Branch("run", &run, "run/I");
    tree->Branch("lumi", &lumi, "lumi/I");
    tree->Branch("bx", &bx, "bx/I");
    tree->Branch("event", &event, "event/l");
  }

  tree->Branch("mu", &mu, "mu/F");

  tree->Branch("rho",   &rho,   "rho/F");
  tree->Branch("rhoC0", &rhoC0, "rhoC0/F");
  tree->Branch("rhoCC", &rhoCC, "rhoCC/F");

  tree->Branch("nPVall",  &nPVall, "nPVall/I");
  tree->Branch("nPV",     &nPV,    "nPV/I");
  tree->Branch("pv_ndof", pv_ndof, "pv_ndof[nPVall]/F");
  tree->Branch("pv_z",    pv_z,    "pv_z[nPVall]/F");
  tree->Branch("pv_rho",  pv_rho,  "pv_rho[nPVall]/F");

  tree->Branch("nEta",   &nEta,  "nEta/I");
  tree->Branch("energy", energy, "energy[nEta]/F");
  tree->Branch("et",     et,     "et[nEta]/F");
  tree->Branch("eRMS",   eRMS,   "eRMS[nEta]/F");

  tree->Branch("fchm",   f[chm],   "fchm[nEta]/b");
  tree->Branch("fchu",   f[chu],   "fchu[nEta]/b");
  tree->Branch("fnh",    f[nh],    "fnh[nEta]/b");
  tree->Branch("fne",    f[ne],    "fne[nEta]/b");
  tree->Branch("fhfh",   f[hfh],   "fhfh[nEta]/b");
  tree->Branch("fhfe",   f[hfe],   "fhfe[nEta]/b");
  tree->Branch("flep",   f[lep],   "flep[nEta]/b");
  tree->Branch("funtrk", f[untrk], "funtrk[nEta]/b");

  tree->Branch("ht", &ht, "ht/F");

  tree->Branch("nJets",    &nJets,   "nJets/I");
  tree->Branch("jet_eta",  jet_eta,  "jet_eta[nJets]/F");
  tree->Branch("jet_phi",  jet_phi,  "jet_phi[nJets]/F");
  tree->Branch("jet_pt",   jet_pt,   "jet_pt[nJets]/F");
  tree->Branch("jet_area", jet_area, "jet_area[nJets]/F");

  tree->Branch("jet_ch",  jet_ch,  "jet_ch[nJets]/F");
  tree->Branch("jet_nh",  jet_nh,  "jet_nh[nJets]/F");
  tree->Branch("jet_ne",  jet_ne,  "jet_ne[nJets]/F");
  tree->Branch("jet_hfh", jet_hfh, "jet_hfh[nJets]/F");
  tree->Branch("jet_hfe", jet_hfe, "jet_hfe[nJets]/F");
  tree->Branch("jet_lep", jet_lep, "jet_lep[nJets]/F");
}

// ------------ method called for each event  ------------
void OffsetTreeMaker::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

  //counter++;
  //if (counter%numSkip_ != 0) return;

//------------ Pileup ------------//

  if (isMC_){
    edm::Handle< vector<PileupSummaryInfo> > pileups;
    iEvent.getByToken(muTag_, pileups);

    mu = pileups->at(1).getTrueNumInteractions();

    if (reweight_){

      float weights[] =
      {1.00000, 0.18151, 0.15735, 0.09638, 0.09874, 0.03999, 0.00650, 0.00138, 0.03547, 0.06018, 
       0.05149, 0.07109, 0.06168, 0.04222, 0.00329, 0.00002, 0.00006, 0.00616, 0.01464, 0.00606, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 
       0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000 };

      int index = h->FindBin(mu);
      weight = weights[index-1];
    }
  }
  else{
    run = int(iEvent.id().run());
    lumi = int(iEvent.getLuminosityBlock().luminosityBlock());
    bx = iEvent.bunchCrossing();
    event = iEvent.id().event();

    mu = getAvgPU( run, lumi );
  }

//------------ Primary Vertices ------------//

  edm::Handle< vector<reco::Vertex> > primaryVertices;
  iEvent.getByToken(pvTag_, primaryVertices);

  nPVall = primaryVertices->size();
  nPV = 0;

  for (int i=0; i<nPVall; i++){
    reco::Vertex pv = primaryVertices->at(i);

    pv_ndof[i] = pv.ndof();
    pv_z[i] = pv.z();
    pv_rho[i] = pv.position().rho();

    if( !pv.isFake() && pv_ndof[i] > 4 && pv_z[i] <= 24 && pv_rho[i] <= 2 )
      nPV++;
  }

//------------ Rho ------------//

  edm::Handle<double> rhoHandle;
  iEvent.getByToken(rhoTag_, rhoHandle);
  rho = *rhoHandle;

  edm::Handle<double> rhoC0Handle;
  iEvent.getByToken(rhoC0Tag_, rhoC0Handle);
  rhoC0 = *rhoC0Handle;

  edm::Handle<double> rhoCCHandle;
  iEvent.getByToken(rhoCCTag_, rhoCCHandle);
  rhoCC = *rhoCCHandle;

//------------ PF Particles ------------//

  edm::Handle< vector<reco::PFCandidate> > pfCandidates;
  iEvent.getByToken(pfTag_, pfCandidates);

  memset(energy, 0, sizeof(energy));    //reset arrays to zero
  memset(et, 0, sizeof(et));
  nEta = ETA_BINS;

  float eFlavor[numFlavors][ETA_BINS] = {};
  float e2[ETA_BINS] = {};  //energy squared
  int nPart[ETA_BINS] = {}; //number of particles per eta bin

  vector<reco::PFCandidate>::const_iterator i_pf, endpf = pfCandidates->end();
  for (i_pf = pfCandidates->begin();  i_pf != endpf;  ++i_pf) {

    int etaIndex = getEtaIndex( i_pf->eta() );
    Flavor flavor = getFlavor( i_pf->particleId() );

    if (etaIndex == -1 || flavor == X) continue;

    bool attached = false;
    reco::TrackRef pftrack( i_pf->trackRef() );

    if (flavor == chm && !pftrack.isNull() ) { //check charged hadrons ONLY
      
      vector<reco::Vertex>::const_iterator i_pv, endpv = primaryVertices->end();
      for (i_pv = primaryVertices->begin(); i_pv != endpv && !attached;  ++i_pv) {
        
        if ( !i_pv->isFake() && i_pv->ndof() >= 4 && fabs(i_pv->z()) < 24 ) {

          reco::Vertex::trackRef_iterator i_vtxTrk, endvtxTrk = i_pv->tracks_end();
          for(i_vtxTrk = i_pv->tracks_begin(); i_vtxTrk != endvtxTrk && !attached; ++i_vtxTrk) {
              
            reco::TrackRef vtxTrk(i_vtxTrk->castTo<reco::TrackRef>());
            if (vtxTrk == pftrack)
              attached = true;
          } 
        }
      }
      if (!attached) flavor = chu; //unmatched charged hadron
    }
    float e = i_pf->energy();

    energy[etaIndex] += e;
    et[etaIndex] += i_pf->et();
    eFlavor[flavor][etaIndex] += e;
    e2[etaIndex] += (e*e);
    nPart[etaIndex] ++;
  }

//------------ Tracks ------------//

  edm::Handle< vector<reco::Track> > tracks;
  iEvent.getByToken(trackTag_, tracks);

  vector<reco::Track>::const_iterator i_trk, endtrk = tracks->end();
  for (i_trk = tracks->begin(); i_trk != endtrk; ++i_trk) {

    if ( !i_trk->quality(reco::Track::tight) ) continue;
    bool matched = false;

    vector<reco::PFCandidate>::const_iterator i_pf, endpf = pfCandidates->end();
    for (i_pf = pfCandidates->begin();  i_pf != endpf && !matched;  ++i_pf) {

      if ( &(*i_trk) == i_pf->trackRef().get() )
        matched = true;      
    }
    if (matched) continue;

    int etaIndex = getEtaIndex( i_trk->eta() );
    if (etaIndex == -1) continue;

    float e = i_trk->p();

    energy[etaIndex] += e;
    et[etaIndex] += i_trk->pt();
    eFlavor[untrk][etaIndex] += e;
    e2[etaIndex] += (e*e);
    nPart[etaIndex] ++;
  }

  for (int i=0; i<nEta; i++){

    for (int flav = 0; flav<numFlavors; flav++){
      UChar_t f_value; float eFlav = eFlavor[flav][i]; float E = energy[i];

      if (eFlav == 0)      f_value = 0;
      else if (eFlav == E) f_value = 255;
      else                 f_value = int( eFlav * 256 / E );

      f[flav][i] = f_value;
    }

    nPart[i] == 0 ? eRMS[i] = 0 : eRMS[i] = sqrt( e2[i]/nPart[i] );
  }

//------------ PF Jets ------------//

  edm::Handle< vector<reco::PFJet> > corrPfJets;
  iEvent.getByToken(corrPfJetTag_, corrPfJets);

  ht = 0;
  vector<reco::PFJet>::const_iterator i_jet, endjet = corrPfJets->end();
  for (i_jet = corrPfJets->begin();  i_jet != endjet;  ++i_jet) {

    float pt = i_jet->pt();
    if (pt > 10) ht += pt;
  }

  corrPfJets->size()<MAXJETS ? nJets = corrPfJets->size() : nJets = MAXJETS;
  for (int i=0; i<nJets; i++){
    reco::PFJet jet = corrPfJets->at(i);

    jet_eta[i] = jet.eta();
    jet_phi[i] = jet.phi();
    jet_pt[i] = jet.pt();
    jet_area[i] = jet.jetArea();

    jet_ch[i] = jet.chargedHadronEnergyFraction();
    jet_nh[i] = jet.neutralHadronEnergyFraction();
    jet_ne[i] = jet.photonEnergyFraction();
    jet_hfh[i] = jet.HFHadronEnergyFraction();
    jet_hfe[i] = jet.HFEMEnergyFraction();
    jet_lep[i] = jet.electronEnergyFraction() + jet.muonEnergyFraction();
  }

//------------ Fill Tree ------------//

  tree->Fill();
}


// ------------ method called once each job just after ending the event loop  ------------
void OffsetTreeMaker::endJob() {

  if (root_file !=0) {

    root_file->Write();
    delete root_file;
    root_file = 0;
  }
}

int OffsetTreeMaker::getEtaIndex(float eta){

  for (int i=0; i<ETA_BINS; i++){
    if (etabins[i] <= eta && eta < etabins[i+1]) return i;
  }
  if (eta == etabins[ETA_BINS]) return ETA_BINS-1;
  else return -1;
}

OffsetTreeMaker::Flavor OffsetTreeMaker::getFlavor(reco::PFCandidate::ParticleType id)
{
    if (id == reco::PFCandidate::h)
        return chm;     //initially matched charged hadron
    else if (id == reco::PFCandidate::e)
        return lep;
    else if (id == reco::PFCandidate::mu)
        return lep;
    else if (id == reco::PFCandidate::gamma)
        return ne;
    else if (id == reco::PFCandidate::h0)
        return nh;
    else if (id == reco::PFCandidate::h_HF)
        return hfh;
    else if (id == reco::PFCandidate::egamma_HF)
        return hfe;
    else
        return X;
}

//define this as a plug-in
DEFINE_FWK_MODULE(OffsetTreeMaker);
