#include "TTree.h"
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
#include <iomanip>
#include <string>
#include <sstream>
using namespace std;

Int_t nValues = 7;

Double_t avgValues[7] = {0.00,0.03,0.10,0.30,0.50,0.75,1.00};
Double_t N[7] = {1./180,10./180,30./180,60./180,90./180,130./180,180./180};
Double_t rmsValues[3][6][7];	//Tracker,camera,averageinterval
Double_t rmsErrors[3][6][7];
Double_t refValues[3][6][7];
string names[6] = {"x1","y1","z1","x2","y2","z2"};
TH1F* h;
TFile* inFile;
TCanvas* c[3];
TGraph* gr;
TGraph* grRef;

void getRMS(Bool_t makeNewFiles,string startDate, string endDate) {

  if (makeNewFiles == TRUE) {	// Make new files if desired
    for (int i = 0; i < nValues; i++) {
      MakeAverage(avgValues[i],startDate,endDate);
    }
  }

  for (int k = 0; k < nValues; k++) { 	// Fill arrays with the RMS values and errors
    
    inFile = new TFile(Form("./../ResultTrees/BCamData_Averaged_%.2f.root",avgValues[k]));

    for (int i = 0; i < 3; i++) {  // T1 T2 T3
      for (int j = 0; j < 6; j++) {	// x1 y1 z1 x2 y2 z2
	
	h = (TH1F*) inFile->Get(Form("htemp%i%i",i+1,j+1));
	rmsValues[i][j][k] = h->GetRMS();
	rmsErrors[i][j][k] = h->GetRMSError();
   
	refValues[i][j][k] = rmsValues[i][j][0]*1./TMath::Sqrt(N[k]*180);
      }
    }

  }

  TFile* outfile = new TFile("./../ResultTrees/BCamData_RMS.root","RECREATE");

  for (int i = 0; i < 3; i++) {		// Draw graphs

    c[i] = new TCanvas(Form("cT%i_RMS",i+1),Form("cT%i_RMS",i+1),1500,700);
    c[i]->Divide(3,2);

    for (int j = 0; j < 6; j++) { 
      
      c[i]->cd(j+1);

      gr = new TGraphErrors(nValues,avgValues,rmsValues[i][j],NULL,rmsErrors[i][j]); 
      gr->SetTitle(names[j].c_str());
      gr->GetXaxis()->SetTitle("#Deltat [Fraction of hr.]");
      gr->GetYaxis()->SetTitle("RMS [m]");
      gr->SetMarkerStyle(20);
      gr->Draw("AP");
      
      grRef = new TGraph(nValues,N,refValues[i][j]); 
      grRef->SetLineColor(2);
      grRef->SetLineWidth(2);
      grRef->Draw("SAME");
    }

    c[i]->Write();
  }

}


