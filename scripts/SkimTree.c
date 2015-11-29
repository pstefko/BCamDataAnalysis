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
void MatchCoordinatesWithTemperatures(TTree *treeInTemperature, TTree *treeInBCam) ;

void MakeSkimming(string startTime = "01-05-2015 04:00:00", string endTime = "19-11-2015 00:00:00", Bool_t removeJumps = false) {

  string boxes[12] = {"1A","1B","1C","1T","2A","2B","2C","2T","3A","3B","3C","3T"};
  TTree *treeT[12];

  TFile *inFile = new TFile("./../ResultTrees/BCamData_RAW.root"); 	// Input file 
  TTree *treeB = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing RAW data from the .csv files arranged in a big tree
  TTree *treeM = (TTree*) inFile->Get("treeMagnet");
  TTree *treeV = (TTree*) inFile->Get("treeVoltage");

  for (int i = 0; i < 12; i++) {
    treeT[i] = (TTree*) inFile->Get(Form("treeTemperature%s",boxes[i].c_str()));
  }

  struct tm tm;			// Conver input times into UNIX epoch timestamp
  strptime(startTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t startEpoch = mktime(&tm);
  strptime(endTime.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
  Int_t endEpoch = mktime(&tm);
  
  TFile *outFile = new TFile("./../ResultTrees/BCamData_Skimmed.root","recreate");

  SkimTree(treeB, startEpoch, endEpoch, removeJumps);
  SkimTree(treeM, startEpoch, endEpoch, removeJumps);
  SkimTree(treeV, startEpoch, endEpoch, removeJumps);

  for (int i = 0; i < 12; i++) {
    SkimTree(treeT[i], startEpoch, endEpoch, removeJumps);
  }
  
  MatchCoordinatesWithTemperatures(treeT[0], treeB);
  MatchCoordinatesWithTemperatures(treeT[4], treeB);
  MatchCoordinatesWithTemperatures(treeT[8], treeB);

  treeB->Write();
  treeM->Write();
  treeV->Write();
  for (int i = 0; i < 12; i++) {
    treeT[i]->Write();
  }

  cout << "Done, file ./../ResultTrees/BCamData_Skimmed.root created." << endl; 

  delete inFile;
  delete outFile;
}

void MatchCoordinatesWithTemperatures(TTree *treeInTemperature, TTree *treeInBCam) {

  Int_t tT, tB;
  Float_t Temp;
  Float_t x[2]; Float_t y[2]; Float_t z[2];
  Float_t xprev[2]; Float_t yprev[2]; Float_t zprev[2];
  Float_t xnext[2]; Float_t ynext[2]; Float_t znext[2];
  Float_t xn[2]; Float_t yn[2]; Float_t zn[2];
  TBranch *bx[2]; TBranch *by[2]; TBranch *bz[2];
  
  Int_t NT = treeInTemperature->GetEntries();
  Int_t NB = treeInBCam->GetEntries();

  char whichStation = treeInTemperature->GetName()[15];

  TTree *treeTnew = new TTree(Form("treeTemperatureWithCoordinatesT%c",whichStation),"Tree containing temperatures from box A matched with coordinates from BCam");

  treeInTemperature->SetBranchAddress("t",&tT);
  treeInTemperature->SetBranchAddress("temperature",&Temp);
  treeInBCam->SetBranchAddress("t11",&tB);

  TBranch *bt = (TBranch*) treeTnew->Branch("t",&tT,"t/I");	// New coordinate Branches in Temperature tree
  TBranch *bT = (TBranch*) treeTnew->Branch("temperature",&Temp,"temperature/F");

  for (int j = 0; j < 2; j++) {

    treeInBCam->SetBranchAddress(Form("x%c%i",whichStation,j+1),&x[j]);
    treeInBCam->SetBranchAddress(Form("y%c%i",whichStation,j+1),&y[j]);
    treeInBCam->SetBranchAddress(Form("z%c%i",whichStation,j+1),&z[j]);
    
    bx[j] = (TBranch*) treeTnew->Branch(Form("x%i",j+1),&xn[j],Form("x%i/F",j+1));	// New coordinate Branches in Temperature tree
    by[j] = (TBranch*) treeTnew->Branch(Form("y%i",j+1),&yn[j],Form("y%i/F",j+1));
    bz[j] = (TBranch*) treeTnew->Branch(Form("z%i",j+1),&zn[j],Form("z%i/F",j+1));
  }

  Int_t prevJ = 0;
  Int_t newEntries = 0;

  for (int i = 0; i < NT; i++) {	// Loop over entries in TemperatureTree

    treeInTemperature->GetEntry(i);
    Bool_t hasMatch = false;

    for (int j = prevJ; j < NB; j++) {	// Loop over entries in BCam tree
      
      treeInBCam->GetEntry(j-1);
      xprev[0]=x[0]; xprev[1]=x[1]; yprev[0]=y[0]; yprev[1]=y[1]; zprev[0]=z[0]; zprev[1]=z[1];
      treeInBCam->GetEntry(j+1);
      xnext[0]=x[0]; xnext[1]=x[1]; ynext[0]=y[0]; ynext[1]=y[1]; znext[0]=z[0]; znext[1]=z[1];

      treeInBCam->GetEntry(j);

      // If we find a match (less than 100 seconds difference) save the coordinates

      if (abs(tT-tB) < 100) {		
	xn[0]=x[0]; xn[1]=x[1]; yn[0]=y[0]; yn[1]=y[1]; zn[0]=z[0]; zn[1]=z[1];

        // Apply the cuts removing the defective points, if it is defective consider the previous value
	for (int k = 0; k < 2; k++) {
	  if ((((abs(x[k]-xprev[k]) > 0.0003) && (abs(x[k]-xnext[k]) > 0.0003)) || (abs(x[k]-xprev[k]) > 0.01))) xn[k] = xprev[k];
	  if ((((abs(y[k]-yprev[k]) > 0.0002) && (abs(y[k]-ynext[k]) > 0.0002)) || (abs(y[k]-yprev[k]) > 0.01))) yn[k] = yprev[k];
	  if ((((abs(z[k]-zprev[k]) > 0.001) && (abs(z[k]-znext[k]) > 0.001)) || (abs(z[k]-zprev[k]) > 0.03))) zn[k] = zprev[k];
	}

	cout << i << "     " << j << endl;
	hasMatch = true;
	prevJ = j;
	break;
      }

    }

    // If we have a match fill the Branches in new TemperatureTree with coordinates and time and temperature

    if (hasMatch == false) continue;

    newEntries += 1;
    bt->Fill();
    bT->Fill();
    
    for (int k = 0; k < 2; k++) {
      bx[k]->Fill();
      by[k]->Fill();
      bz[k]->Fill();
    }
   
  }
  
  treeTnew->SetEntries(newEntries);
  treeTnew->Print();
  treeTnew->Write();
}

void SkimTree(TTree *&treeIn, Int_t startEpoch, Int_t endEpoch, Bool_t removeJumps) {

  // This procedure takes a treeIn tree and changes it so that it contains only values in the interested time interval and remove jumps if requested
  
  string jumpTime[24] = {"11-05-2015 02:00:00","12-05-2015 12:00:00","05-06-2015 19:00:00","06-06-2015 23:00:00","07-06-2015 11:00:00","07-06-2015 21:00:00","15-06-2015 03:00:00","22-06-2015 03:00:00","16-07-2015 05:00:00","16-07-2015 17:00:00","21-07-2015 05:00:00","25-07-2015 17:00:00","08-08-2015 05:00:00","08-08-2015 17:00:00","24-08-2015 02:00:00","24-08-2015 09:00:00","31-08-2015 02:00:00","05-09-2015 07:00:00","23-09-2015 05:00:00","23-09-2015 23:00:00","18-10-2015 08:00:00","20-10-2015 04:00:00","09-11-2015 04:00:00","13-11-2015 18:00:00"}; 

  Int_t t = 0;

  string treeName = treeIn->GetName();

  cout << "Skimming " << treeName << endl;
  
  if (treeName == "treeBCam") 	  	treeIn->SetBranchAddress("t11",&t);
  if (treeName == "treeMagnet")   	{treeIn->SetBranchAddress("t",&t); removeJumps = false;}
  if (treeName.substr(0,15) == "treeTemperature")   	{treeIn->SetBranchAddress("t",&t); removeJumps = false;}
  if (treeName == "treeVoltage")   	{treeIn->SetBranchAddress("t",&t); removeJumps = false;}

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


