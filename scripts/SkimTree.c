#include "TTree.h"
#include "TPaletteAxis.h"
#include "TColor.h"
#include "TGaxis.h"
#include "TLatex.h"
#include "TText.h"
#include "TMath.h"
#include "TLine.h"
#include "TGraph.h"
#include "TStyle.h"
#include "TH2F.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TFile.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
using namespace std;


void SkimTree(TTree *&treeIn, Int_t startEpoch, Int_t endEpoch, Bool_t removeJumps) ;

void MakeSkimming(string startTime = "01-05-2015 04:00:00", string endTime = "19-11-2015 00:00:00", Bool_t removeJumps = false) {

  TFile *inFile = new TFile("./../ResultTrees/BCamData_RAW.root"); 	// Input file 
  TTree *treeB = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing RAW data from the .csv files arranged in a big tree
  TTree *treeM = (TTree*) inFile->Get("treeMagnet");

  struct tm tm;			// Conver input times into UNIX epoch timestamp
  strptime(startTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t startEpoch = mktime(&tm);
  strptime(endTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t endEpoch = mktime(&tm);
  
  TFile *outFile = new TFile("./../ResultTrees/BCamData_Skimmed.root","recreate");

  SkimTree(treeB, startEpoch, endEpoch, removeJumps);
  SkimTree(treeM, startEpoch, endEpoch, removeJumps);

  treeB->Write();
  treeM->Write();

  cout << "Done, file ./../ResultTrees/BCamData_Skimmed.root created." << endl; 

  delete inFile;
  delete outFile;
}


void SkimTree(TTree *&treeIn, Int_t startEpoch, Int_t endEpoch, Bool_t removeJumps) {

  // This procedure makes a new tree containing only values in the interested time interval and remove jumps if requested
  
  string jumpTime[22] = {"11-05-2015 02:00:00","12-05-2015 12:00:00","05-06-2015 19:00:00","06-06-2015 23:00:00","07-06-2015 11:00:00","07-06-2015 21:00:00","15-06-2015 03:00:00","22-06-2015 03:00:00","16-07-2015 05:00:00","16-07-2015 17:00:00","21-07-2015 05:00:00","25-07-2015 17:00:00","08-08-2015 05:00:00","08-08-2015 17:00:00","24-08-2015 02:00:00","24-08-2015 09:00:00","31-08-2015 02:00:00","05-09-2015 07:00:00","23-09-2015 05:00:00","23-09-2015 23:00:00","18-10-2015 08:00:00","20-10-2015 04:00:00"}; 

  Int_t t = 0;

  string treeName = treeIn->GetName();

  cout << "Skimming " << treeName << endl;
  
  if (treeName == "treeBCam") 	  treeIn->SetBranchAddress("t11",&t);
  if (treeName == "treeMagnet")   treeIn->SetBranchAddress("t",&t);

  TTree* treeSkim = treeIn->CloneTree(0);

  for (int i = 0; i < treeIn->GetEntries(); i++) {
    
    treeIn->GetEntry(i);
    Bool_t skip = false;
    
    if ((t > startEpoch) && (t < endEpoch)) {
      struct tm tm; 
      for (int j = 0; j < 11; j++) {
         
         strptime(jumpTime[j*2].c_str(), "%d-%m-%Y %H:%M:%S", &tm);
         Int_t jump1 = mktime(&tm);
         strptime(jumpTime[j*2+1].c_str(), "%d-%m-%Y %H:%M:%S", &tm);
         Int_t jump2 = mktime(&tm);

	 if ((t > jump1) && (t < jump2) && (removeJumps == true)) skip = true; 
      }

      if (skip == true) continue;

      treeSkim->Fill();
    }
  }
 
  treeIn = treeSkim;

}


