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
void MatchCoordinatesWithTemperatures(TTree *treeInTemperatures, TTree *treeInBCam) ;

void MakeSkimming(string startTime = "01-05-2015 04:00:00", string endTime = "19-11-2015 00:00:00", Bool_t removeJumps = false) {

  TFile *inFile = new TFile("./../ResultTrees/BCamData_RAW.root"); 	// Input file 
  TTree *treeB = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing RAW data from the .csv files arranged in a big tree
  TTree *treeM = (TTree*) inFile->Get("treeMagnet");
  TTree *treeT = (TTree*) inFile->Get("treeTemperature");

  struct tm tm;			// Conver input times into UNIX epoch timestamp
  strptime(startTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t startEpoch = mktime(&tm);
  strptime(endTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t endEpoch = mktime(&tm);
  
  TFile *outFile = new TFile("./../ResultTrees/BCamData_Skimmed.root","recreate");

  SkimTree(treeB, startEpoch, endEpoch, removeJumps);
  SkimTree(treeM, startEpoch, endEpoch, removeJumps);
  SkimTree(treeT, startEpoch, endEpoch, removeJumps);
  
  //MatchCoordinatesWithTemperatures(treeT, treeB);

  treeB->Write();
  treeM->Write();
  treeT->Write();

  cout << "Done, file ./../ResultTrees/BCamData_Skimmed.root created." << endl; 

  delete inFile;
  delete outFile;
}

void MatchCoordinatesWithTemperatures(TTree *treeInTemperatures, TTree *treeInBCam) {

  Int_t NT = treeInTemperatures->GetEntries();

  Int_t t1, t2, t3;
  Int_t tT1[NT];
  Int_t tT2[NT];
  Int_t tT3[NT];
  Float_t x[3][2];
  Float_t y[3][2];
  Float_t z[3][2];
  Float_t xn[3][2];
  Float_t yn[3][2];
  Float_t zn[3][2];
  TBranch *bx[3][2];
  TBranch *by[3][2];
  TBranch *bz[3][2];

  treeInTemperatures->SetBranchAddress("t1A",&t1);
  treeInTemperatures->SetBranchAddress("t2A",&t2);
  treeInTemperatures->SetBranchAddress("t3A",&t3);

  for (int i = 0; i < NT; i++) {	// Save times from A box in each station into array
    treeInTemperatures->GetEntry(i);
    tT1[i] = t1;
    tT2[i] = t2;
    tT3[i] = t3;
  }

  Int_t NB = treeInBCam->GetEntries();
  
  treeInBCam->SetBranchAddress("t11",&t1);
  treeInBCam->SetBranchAddress("t21",&t2);
  treeInBCam->SetBranchAddress("t31",&t3);
      
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {

      treeInBCam->SetBranchAddress(Form("x%i%i",i+1,j+1),&x[i][j]);
      treeInBCam->SetBranchAddress(Form("y%i%i",i+1,j+1),&y[i][j]);
      treeInBCam->SetBranchAddress(Form("z%i%i",i+1,j+1),&z[i][j]);
      
      bx[i][j] = (TBranch*) treeInTemperatures->Branch(Form("x%i%i",i+1,j+1),&xn[i][j],Form("x%i%i/F",i+1,j+1));
      by[i][j] = (TBranch*) treeInTemperatures->Branch(Form("y%i%i",i+1,j+1),&yn[i][j],Form("y%i%i/F",i+1,j+1));
      bz[i][j] = (TBranch*) treeInTemperatures->Branch(Form("z%i%i",i+1,j+1),&zn[i][j],Form("z%i%i/F",i+1,j+1));
    }
  }


  for (int i = 0; i < NT; i++) {

    cout << i << endl;
    for (int j = 0; j < NB; j++) {
      
      treeInBCam->GetEntry(j);
      //if (i == 36) {cout << t1 << "   " << tT1[i] << endl;}

      if (abs(t1-tT1[i]) < 100) {
	xn[0][0]=x[0][0]; xn[0][1]=x[0][1]; yn[0][0]=y[0][0]; yn[0][1]=y[0][1]; zn[0][0]=z[0][0]; zn[0][1]=z[0][1];
	cout << "  match1" << endl;
      }
      if (abs(t2-tT2[i]) < 100) {
	xn[1][0]=x[1][0]; xn[1][1]=x[1][1]; yn[1][0]=y[1][0]; yn[1][1]=y[1][1]; zn[1][0]=z[1][0]; zn[1][1]=z[1][1];
	cout  << "  match2" << endl;
      }
      if (abs(t3-tT3[i]) < 100) {
	xn[2][0]=x[2][0]; xn[2][1]=x[2][1]; yn[2][0]=y[2][0]; yn[2][1]=y[2][1]; zn[2][0]=z[2][0]; zn[2][1]=z[2][1];
	cout  << "  match3" << endl;
      }




    }

    for (int l = 0; l < 3; l++) {
      for (int k = 0; k < 2; k++) {
	bx[l][k]->Fill();
	by[l][k]->Fill();
	bz[l][k]->Fill();
      }
    }
      
  }

  treeInTemperatures->Print();

}


void SkimTree(TTree *&treeIn, Int_t startEpoch, Int_t endEpoch, Bool_t removeJumps) {

  // This procedure takes a treeIn tree and changes it so that it contains only values in the interested time interval and remove jumps if requested
  
  string jumpTime[24] = {"11-05-2015 02:00:00","12-05-2015 12:00:00","05-06-2015 19:00:00","06-06-2015 23:00:00","07-06-2015 11:00:00","07-06-2015 21:00:00","15-06-2015 03:00:00","22-06-2015 03:00:00","16-07-2015 05:00:00","16-07-2015 17:00:00","21-07-2015 05:00:00","25-07-2015 17:00:00","08-08-2015 05:00:00","08-08-2015 17:00:00","24-08-2015 02:00:00","24-08-2015 09:00:00","31-08-2015 02:00:00","05-09-2015 07:00:00","23-09-2015 05:00:00","23-09-2015 23:00:00","18-10-2015 08:00:00","20-10-2015 04:00:00","09-11-2015 04:00:00","13-11-2015 18:00:00"}; 

  Int_t t = 0;

  string treeName = treeIn->GetName();

  cout << "Skimming " << treeName << endl;
  
  if (treeName == "treeBCam") 	  treeIn->SetBranchAddress("t11",&t);
  if (treeName == "treeMagnet")   {treeIn->SetBranchAddress("t",&t); removeJumps = false;}
  if (treeName == "treeTemperature")   {treeIn->SetBranchAddress("t1A",&t); removeJumps = false;}

  TTree* treeSkim = treeIn->CloneTree(0);

  for (int i = 0; i < treeIn->GetEntries(); i++) {
    
    treeIn->GetEntry(i);
    Bool_t skip = false;
    
    if ((t > startEpoch) && (t < endEpoch)) {
      struct tm tm; 
      for (int j = 0; j < 12; j++) {
         
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


