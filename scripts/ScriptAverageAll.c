#include "TTree.h"
#include "TView.h"
#include "TPaveText.h"
#include "TGraph2D.h"
#include "TMultiGraph.h"
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
#include <fstream>
#include <vector>
using namespace std;

//	Script that takes all the BCam data from RAW root file, Averages over a given time and outputs
//	a text file where the BCam data are organized in columns. 


void AverageOneBranchIntoArray(string branchName, Int_t avgSize, TTree *tree, vector<vector<Float_t>>& vector) ; 

void MakeAverageAll(Float_t avrgSize) {

  TFile *inFile = new TFile("./../ResultTrees/OriginalData/BCamData_RAW.root"); 	// Input file 
  TTree *tree = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing Skimmed data from the .csv files arranged in a big tree
  
  Int_t avgSize = round(avrgSize*181);	// Input is in fraction of an hour, there is 180 measurements per hour

  // Array of arrays containing the lines that will be printed ina  new file... 1 column t11, 18 columns BCam Data
  vector< vector<Float_t> > dataForOutput;
  dataForOutput.resize(19);
  
  if (avrgSize == 0.) avgSize = 1;
  
  ofstream myfile;
  myfile.open (Form("./../ResultTrees/OriginalData/BCamData_Averaged_%.2f.txt",avrgSize));

  // Now average the branches of the original tree one by one
  for (int i = 1; i < 4; i++) {
    for (int j = 1; j < 3; j++) {
      ostringstream is;
      ostringstream js;
      is << i; js << j;

      AverageOneBranchIntoArray("t" + is.str() + js.str() , avgSize, tree, dataForOutput);
      AverageOneBranchIntoArray("x" + is.str() + js.str() , avgSize, tree, dataForOutput);
      AverageOneBranchIntoArray("y" + is.str() + js.str() , avgSize, tree, dataForOutput);
      AverageOneBranchIntoArray("z" + is.str() + js.str() , avgSize, tree, dataForOutput);
    }
  }

  myfile << "t,x11,y11,z11,x12,y12,z12,x21,y21,z21,x22,y22,z22,x31,y31,z31,x32,y32,z32" << endl;

  for ( int i = 0; i < dataForOutput[0].size(); i++){
    myfile << fixed << dataForOutput[0][i] << ",";
    for ( int j = 1; j < 19; j++) {
      myfile << dataForOutput[j][i] << ",";
    }
    myfile << endl;
  }

  myfile.close();

}

void AverageOneBranchIntoArray(string branchName, Int_t avgSize, TTree *tree, vector<vector<Float_t>>& dataForOutput) {

  // Procedure that - if the branch is a "coordinate" branch averages over a given amount of points
  //                - if the branch is a "time" branch picks the middle element from the averaging interval

  Float_t x = 0;
  Int_t t = 0;
  Double_t xAvg = 0, sum = 0;

  cout << branchName << "     " << avgSize << endl;

  if (branchName[0] == 't') {
    tree->SetBranchAddress(branchName.c_str(),&t);
  }
  else {
    tree->SetBranchAddress(branchName.c_str(),&x);
  }

  Int_t whichCoord;

  if (branchName.substr(0,3) == "x11") whichCoord = 1;
  if (branchName.substr(0,3) == "y11") whichCoord = 2;
  if (branchName.substr(0,3) == "z11") whichCoord = 3;
  if (branchName.substr(0,3) == "x12") whichCoord = 4;
  if (branchName.substr(0,3) == "y12") whichCoord = 5;
  if (branchName.substr(0,3) == "z12") whichCoord = 6;
  if (branchName.substr(0,3) == "x21") whichCoord = 7;
  if (branchName.substr(0,3) == "y21") whichCoord = 8;
  if (branchName.substr(0,3) == "z21") whichCoord = 9;
  if (branchName.substr(0,3) == "x22") whichCoord = 10;
  if (branchName.substr(0,3) == "y22") whichCoord = 11;
  if (branchName.substr(0,3) == "z22") whichCoord = 12;
  if (branchName.substr(0,3) == "x31") whichCoord = 13;
  if (branchName.substr(0,3) == "y31") whichCoord = 14;
  if (branchName.substr(0,3) == "z31") whichCoord = 15;
  if (branchName.substr(0,3) == "x32") whichCoord = 16;
  if (branchName.substr(0,3) == "y32") whichCoord = 17;
  if (branchName.substr(0,3) == "z32") whichCoord = 18;


  Int_t N = tree->GetEntries();
  tree->GetEntry(0);
  Double_t prevEntry = x;
  Double_t nextEntry = 0;

  Int_t j = 0;

  for (Int_t i = 0; i < N; i++) {	// Loop over branch elements
    
    if (i > N - N%avgSize) break; 	// Scrap the last half-average interval

    tree->GetEntry(i+1);	// Save the following entry
    nextEntry = x;
    
    tree->GetEntry(i);

    // Apply the cuts removing the defective points, if it is defective consider the previous value
    if ((branchName[0] == 'x') && (((abs(x-prevEntry) > 0.0003) && (abs(x-nextEntry) > 0.0003)) || (abs(x-prevEntry) > 0.01))) x = prevEntry; 
    if ((branchName[0] == 'y') && (((abs(x-prevEntry) > 0.0002) && (abs(x-nextEntry) > 0.0002)) || (abs(x-prevEntry) > 0.01))) x = prevEntry; 
    if ((branchName[0] == 'z') && (((abs(x-prevEntry) > 0.001) && (abs(x-nextEntry) > 0.001)) || (abs(x-prevEntry) > 0.03))) x = prevEntry; 

    sum += x;

    // If the branch is a time branch fill the new tree only if the loop is at the middle element of average interval
    if ((branchName.substr(0,3) == "t11") && (int(i+ceil(double(avgSize)/2)) % avgSize) == 0) {
      xAvg = t;
      dataForOutput[0].push_back(xAvg);
      j++;
    }

    // If the branch is not time fill the new tree with the averaged values 
    if ((branchName[0] != 't') && ((i+1) % avgSize == 0)) {
      xAvg = sum/avgSize;
      sum = 0;
      dataForOutput[whichCoord].push_back(xAvg);
      j++;
    }

    prevEntry = x;
 
  }
}


