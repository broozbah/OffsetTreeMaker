//Skim a TTree

#include "TFile.h"
#include "TTree.h"
using namespace std;

void tuple_skim(){
  
  int numSkip = "some primary number"; // or whatever needed

  TFile* file = TFile::Open("file.root");

  TTree* tree = (TTree*) file->Get("T");
  Long64_t nEntries = tree->GetEntries();

  ULong64_t event;
  int run;
  tree->SetBranchAddress("event", &event);
  tree->SetBranchAddress("run", &run);
  tree->SetBranchStatus("*",1);

  TFile *newfile = new TFile("output.root","recreate");
  TTree *newtree = tree->CloneTree(0);

  for (Long64_t n=0; n<nEntries; n++) {
    tree->GetEntry(n);
    if (event % numSkip == 0) newtree->Fill();
  }

  newtree->Print();
  newfile->Write();
  delete file;
  delete newfile;
}
