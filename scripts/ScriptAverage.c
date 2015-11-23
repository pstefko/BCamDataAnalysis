#include "TTree.h"
#include "TView.h"
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
using namespace std;

void AverageOneBranch(string branchName, Int_t avgSize, TTree *tree, TTree *treeAvg) ; 
void EnableBranch(string branchName, TTree *treeAvg) ;
void DrawGraphs(TTree *treeAvg, TTree *treeMagnet, TTree *treeTemperature) ;

void MakeAverage(Float_t avrgSize) {

  TFile *inFile = new TFile("./../ResultTrees/BCamData_Skimmed.root"); 	// Input file 
  TTree *tree = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing Skimmed data from the .csv files arranged in a big tree

  TTree *treeAvg =  new TTree ("treeAvg", "BCam data averaged");	// Tree in an output file containing the averaged values
  TTree* treeMagnet = (TTree*) inFile->Get("treeMagnet");
  TTree* treeTemperature = (TTree*) inFile->Get("treeTemperature");
  treeAvg->SetDirectory(0);

  Int_t avgSize = round(avrgSize*181);	// Input is in fraction of an hour, there is 180 measurements per hour
  
  if (avrgSize == 0.) avgSize = 1;

  // Now average the branches of the original tree one by one
  for (int i = 1; i < 4; i++) {
    for (int j = 1; j < 3; j++) {
      ostringstream is;
      ostringstream js;
      is << i; js << j;
      AverageOneBranch("t" + is.str() + js.str() , avgSize, tree, treeAvg);
      AverageOneBranch("x" + is.str() + js.str() , avgSize, tree, treeAvg);
      AverageOneBranch("y" + is.str() + js.str() , avgSize, tree, treeAvg);
      AverageOneBranch("z" + is.str() + js.str() , avgSize, tree, treeAvg);
    }
  }

  //Now enable these branches for later use
  for (int i = 1; i < 4; i++) {
    for (int j = 1; j < 3; j++) {
      ostringstream is;
      ostringstream js;
      is << i; js << j;
      EnableBranch("x" + is.str() + js.str(), treeAvg );
      EnableBranch("y" + is.str() + js.str(), treeAvg );
      EnableBranch("z" + is.str() + js.str(), treeAvg );
      EnableBranch("t" + is.str() + js.str(), treeAvg );
    }
  }

  treeAvg->SetEntries(floor((tree->GetEntries()/avgSize)));	// Set correct number of entries in the new averaged tree
  
  TFile* outFile = new TFile(Form("./../ResultTrees/BCamData_Averaged_%.2f.root",avrgSize),"RECREATE");	// Output file 

  treeAvg->Write();

  DrawGraphs(treeAvg, treeMagnet, treeTemperature);		// Draw the appropriate graphs and write them into ouput file

  cout << "Done, file " << outFile->GetName() << " created" << endl;

  outFile->Close();

}



void AverageOneBranch(string branchName, Int_t avgSize, TTree *tree, TTree *treeAvg) {

  // Procedure that - if the branch is a "coordinate" branch averages over a given amount of points
  //                - if the branch is a "time" branch picks the middle element from the averaging interval

  Float_t x = 0;
  Int_t t = 0;
  Double_t xAvg = 0, sum = 0;

  cout << branchName << "     " << avgSize << endl;

  // Create a branch in a new tree
  treeAvg->Branch(branchName.c_str(),&xAvg);

  if (branchName[0] == 't') {
    tree->SetBranchAddress(branchName.c_str(),&t);
  }
  else {
    tree->SetBranchAddress(branchName.c_str(),&x);
  }


  Int_t N = tree->GetEntries();
  tree->GetEntry(0);
  Double_t prevEntry = x;
  Double_t nextEntry = 0;

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
    if ((branchName[0] == 't') && (int(i+ceil(double(avgSize)/2)) % avgSize) == 0) {
      xAvg = t;
      treeAvg->Fill();
    }

    // If the branch is not time fill the new tree with the averaged values 
    if ((branchName[0] != 't') && ((i+1) % avgSize == 0)) {
      xAvg = sum/avgSize;
      sum = 0;
      treeAvg->Fill();
    }

    prevEntry = x;
 
  }
  // Disable the branch so that we can fill the tree branch by branch
  treeAvg->SetBranchStatus(branchName.c_str(), 0);
}

void EnableBranch(string branchName, TTree *treeAvg) {
  treeAvg->SetBranchStatus(branchName.c_str(), 1);
}

void DrawGraphs(TTree *treeBCam, TTree *treeMagnet, TTree *treeTemperature) {

  // Procedure that Draws the graphs into three canvases (one for each T) 
  
  TTree* treeDraw = treeBCam;


  TCanvas* c[3];
  TCanvas* cM[3];
  TCanvas* cCwT[3];
  TCanvas* cT[3];
  TCanvas* cCvC[3];
  TCanvas* cCvCS[3];
  TCanvas* cHisto[3];

  TGraph* g, *g1, *g2, *temp, *gM;
  TGraph2D *g2d;
  TMultiGraph *mg;
  TCanvas* cDump = new TCanvas("Dump","Dump", 300,200);
  TH1F* h;
  TH2F* h2;


  for (int j = 0; j < 3; j++) {

    ostringstream js; js << j+1;
    
    
    string drawStringCvT[] = {"x"+js.str()+"1:t"+js.str()+"1","y"+js.str()+"1:t"+js.str()+"1","z"+js.str()+"1:t"+js.str()+"1",
      			      "x"+js.str()+"2:t"+js.str()+"2","y"+js.str()+"2:t"+js.str()+"2","z"+js.str()+"2:t"+js.str()+"2"};
    string drawStringCvC[] = {"x"+js.str()+"1:y"+js.str()+"1:t"+js.str()+"1","x"+js.str()+"1:z"+js.str()+"1:t"+js.str()+"1","y"+js.str()+"1:z"+js.str()+"1:t"+js.str()+"1",
         		      "x"+js.str()+"2:y"+js.str()+"2:t"+js.str()+"2","x"+js.str()+"2:z"+js.str()+"2:t"+js.str()+"2","y"+js.str()+"2:z"+js.str()+"2:t"+js.str()+"2"};
    string drawStringCvCS[] = {"x"+js.str()+"1:x"+js.str()+"2:t"+js.str()+"1","y"+js.str()+"1:y"+js.str()+"2:t"+js.str()+"1","z"+js.str()+"1:z"+js.str()+"2:t"+js.str()+"1"};
    string drawStringHisto[] = {"x"+js.str()+"1","y"+js.str()+"1","z"+js.str()+"1","x"+js.str()+"2","y"+js.str()+"2","z"+js.str()+"2"};
    string drawStringTempvT[] = {"Temp"+js.str()+"A:t"+js.str()+"A","Temp"+js.str()+"B:t"+js.str()+"B","Temp"+js.str()+"C:t"+js.str()+"C","Temp"+js.str()+"T:t"+js.str()+"T"};
    

    //c[j] = new TCanvas(Form("cT%iCoordinateVsTime",j+1),Form("cT%iCoordinateVsTime",j+1),1400,700);
    //c[j]->Divide(3,2);
    
    cCwT[j] = new TCanvas(Form("cT%iCoordinateVsTime_temperature",j+1),Form("cT%iCoordinateVsTime_temperature",j+1),1400,700);
    cCwT[j]->Divide(3,2);
      

//    cM[j] = new TCanvas(Form("cT%iCoordinateVsTime_magnet",j+1),Form("cT%iCoordinateVsTime_magnet",j+1),1400,700);
//    cM[j]->Divide(3,2);
    
   // cT[j] = new TCanvas(Form("cT%iTemperatureVsTime",j+1),Form("cT%iTemperatureVsTime",j+1),1400,700);
   // cT[j]->Divide(2,2);
 
    //cCvC[j] = new TCanvas(Form("cT%iCoordinateVsCoordinate",j+1),Form("cT%iCoordinateVsCoordinate",j+1),1400,700);
    //cCvC[j]->Divide(3,2);
    //cCvCS[j] = new TCanvas(Form("cT%iCoordinateVsCoordinate(AvsC)",j+1),Form("cT%iCoordinateVsCoordinate(AvsC)",j+1),1400,400);
    //cCvCS[j]->Divide(3,1);
 //   cHisto[j] = new TCanvas(Form("cT%iCoordinates",j+1),Form("cT%iCoordinates",j+1),1400,700);
 //   cHisto[j]->Divide(3,2);

    
    for (int i = 1; i < 7; i++) {

      // Draw Box temperature vs time -------------------------------------
/*
      cDump->cd();
      
      double max = treeTemperature->GetMaximum(drawStringTempvT[i-1].substr(0,6).c_str()); 	// Scale used to draw the magnet info
      double min = treeTemperature->GetMinimum(drawStringTempvT[i-1].substr(0,6).c_str());
      double scale = max-min;
  
      treeTemperature->Draw(drawStringTempvT[i-1].c_str());	// Draw temperature graph
      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      g1 = (TGraph*)temp->Clone("mygraph");
      
      treeMagnet->Draw(Form("(current*polarity)/6000.*%f+%f:t",scale/8.,min-scale/6));	//Draw Magnet and dump
      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      gM = (TGraph*)temp->Clone("mygraph");
      
      g1->SetMarkerStyle(2);
      g1->SetLineColor(4);
      g1->SetLineWidth(2);
      //g1->GetXaxis()->SetTimeDisplay(1);
      //g1->GetXaxis()->SetTitle("t");
      //g1->GetYaxis()->SetTitle("Temp [K]");
      //g1->SetTitle(drawStringTempvT[i-1].substr(0,6).c_str());

      gM->SetLineColor(2);
      gM->SetLineWidth(1);
      
      mg = new TMultiGraph();	// Multigraph storing the graphs
      mg->Add(g1,"PL");
      mg->Add(gM,"L");
      
      cT[j]->cd(i);
      
      mg->SetTitle(Form("%s;t;Temp [K]",drawStringTempvT[i-1].substr(0,6).c_str()));
      mg->Draw("AP");
      gStyle->SetTimeOffset(0);
      mg->GetXaxis()->SetTimeDisplay(1);


      //mg->GetXaxis()->SetTitle("t");
      //mg->GetYaxis()->SetTitle("Temp [K]");

      //mg->SetTitle(drawStringTempvT[i-1].substr(0,6).c_str());
      gPad->Modified();
*/

      // Draw Coordinate VS Time with Temperature info ------------------------------


      double max = treeDraw->GetMaximum(drawStringHisto[i-1].c_str()); 	// Scale used to draw the magnet info
      double min = treeDraw->GetMinimum(drawStringHisto[i-1].c_str());
      double scale = max-min;
      cout << Form("Temp%iA/3.*%f+%f:t%iA",j+1,scale/8.,min-scale/6.,j+1) << endl;
      
      cDump->cd();
      
      treeDraw->Draw(drawStringCvT[i-1].c_str());	// Draw Coordinate and dump
      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      g1 = (TGraph*)temp->Clone("mygraph");

      treeTemperature->Draw(Form("Temp%iA/3.*%f+%f:t%iA",j+1,scale/3.,min-scale/0.8,j+1));
      

      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      g2 = (TGraph*)temp->Clone("mygraph");

      // Modify graphs applearance
      
      g1->SetMarkerColor(4);
      g1->SetMarkerStyle(2);
      g2->SetLineColor(2);
      g2->SetLineWidth(1);

      mg = new TMultiGraph();	// Multigraph storing the graphs
      mg->Add(g1,"PL");
      mg->Add(g2,"L");
      
      cCwT[j]->cd(i);
      
      mg->Draw("AP");
      mg->GetXaxis()->SetTimeDisplay(1);
      gStyle->SetTimeOffset(0);

      mg->GetXaxis()->SetTitle("t");
      mg->GetYaxis()->SetTitle(drawStringHisto[i-1].c_str());

      gPad->Modified();

      // Draw line scale

      max = g1->GetHistogram()->GetMaximum(); 
      min = g1->GetHistogram()->GetMinimum(); 
      scale = ceil(((max-min)/4)*1000*100.)/100.;	// scale in mm
      
      TLatex* t;
 
      t = new TLatex(.955,.75,Form("%.2f mm",scale));

      //Draw TextPad with the scale
      t->SetNDC();
      t->SetTextFont(42);
      t->SetTextAngle(90);
      t->Draw();

      //Draw a line representing scale
      
      Double_t yr = gPad->GetY2()-gPad->GetY1();
      double y1 = (max-gPad->GetY1())/ yr; 
      double y2 = (max-scale/1000.-gPad->GetY1())/ yr;
      TLine *line = new TLine(0.92,y1,0.92,y2);
      line->SetLineColor(kRed);
      line->SetLineWidth(2);
      line->SetNDC();
      line->Draw();

/*      // Draw Coordinate VS Time with Magnet info ------------------------------

      double max = treeDraw->GetMaximum(drawStringHisto[i-1].c_str()); 	// Scale used to draw the magnet info
      double min = treeDraw->GetMinimum(drawStringHisto[i-1].c_str());
      double scale = max-min;
      
      cDump->cd();
      
      treeDraw->Draw(drawStringCvT[i-1].c_str());	// Draw Coordinate and dump
      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      g1 = (TGraph*)temp->Clone("mygraph");

      treeMagnet->Draw(Form("(current*polarity)/6000.*%f+%f:t",scale/8.,min-scale/6));	//Draw Magnet and dump
      temp = (TGraph*)  cDump->GetPrimitive("Graph");
      g2 = (TGraph*)temp->Clone("mygraph");

      // Modify graphs applearance
      
      g1->SetMarkerColor(4);
      g1->SetMarkerStyle(2);
      g2->SetLineColor(2);
      g2->SetLineWidth(1);

      mg = new TMultiGraph();	// Multigraph storing the graphs
      mg->Add(g1,"PL");
      mg->Add(g2,"L");
      
      cM[j]->cd(i);
      
      mg->Draw("AP");
      mg->GetXaxis()->SetTimeDisplay(1);
      gStyle->SetTimeOffset(0);

      mg->GetXaxis()->SetTitle("t");
      mg->GetYaxis()->SetTitle(drawStringHisto[i-1].c_str());

      gPad->Modified();

      // Line for zero current
      
      TLine *line1 = new TLine(treeMagnet->GetMinimum("t"),min-scale/6,treeMagnet->GetMaximum("t"),min-scale/6);
      line1->SetLineColor(kBlack);
      line1->SetLineWidth(1);
      line1->SetLineStyle(2);
      line1->Draw();
     
      // Draw line scale

      max = g1->GetHistogram()->GetMaximum(); 
      min = g1->GetHistogram()->GetMinimum(); 
      scale = ceil(((max-min)/4)*1000*100.)/100.;	// scale in mm
      
      TLatex* t;
 
      t = new TLatex(.955,.75,Form("%.2f mm",scale));

      //Draw TextPad with the scale
      t->SetNDC();
      t->SetTextFont(42);
      t->SetTextAngle(90);
      t->Draw();

      //Draw a line representing scale
      
      Double_t yr = gPad->GetY2()-gPad->GetY1();
      double y1 = (max-gPad->GetY1())/ yr; 
      double y2 = (max-scale/1000.-gPad->GetY1())/ yr;
      TLine *line = new TLine(0.92,y1,0.92,y2);
      line->SetLineColor(kRed);
      line->SetLineWidth(2);
      line->SetNDC();
      line->Draw();

      // Draw Coordinate VS Time plots -----------------------------

      c[j]->cd(i);

      treeDraw->Draw(drawStringCvT[i-1].c_str());
      g = (TGraph*)  c[j]->cd(i)->GetPrimitive("Graph");
      h = (TH1F*)  c[j]->cd(i)->GetPrimitive("htemp");
      h->GetXaxis()->SetTimeDisplay(1);
      gStyle->SetTimeOffset(0); 
      g->SetMarkerStyle(2);
      g->Draw();

      double max = g->GetHistogram()->GetMaximum(); 
      double min = g->GetHistogram()->GetMinimum(); 
      double scale = ceil(((max-min)/4)*1000*100.)/100.;	// scale in mm

      TLatex* t;
 
      t = new TLatex(.955,.75,Form("%.2f mm",scale));

      //Draw TextPad with the scale
      t->SetNDC();
      t->SetTextFont(42);
      t->SetTextAngle(90);
      t->Draw();

      //Draw a line representing scale

      Double_t yr = gPad->GetY2()-gPad->GetY1();
      double y1 = (max-gPad->GetY1())/ yr; 
      double y2 = (max-scale/1000.-gPad->GetY1())/ yr;
      TLine *line = new TLine(0.92,y1,0.92,y2);
      line->SetLineColor(kRed);
      line->SetLineWidth(2);
      line->SetNDC();
      line->Draw();
*/    
/*
      // Coordinate vs Coordinate test ----------------------------

      Double_t x,y,t;
      
      g2d = new TGraph2D(treeDraw->GetEntries());

      treeDraw->SetBranchAddress(drawStringCvC[i-1].substr(0,3).c_str(),&x);
      treeDraw->SetBranchAddress(drawStringCvC[i-1].substr(4,3).c_str(),&y);
      treeDraw->SetBranchAddress(drawStringCvC[i-1].substr(8,3).c_str(),&t);
      
      for (int k = 0; k < treeDraw->GetEntries(); k++) {
	treeDraw->GetEntry(k);
	g2d->SetPoint(k,x,y,t);
      }



      cCvC[j]->cd(i);
      gStyle->SetPalette(54);
      gStyle->SetNumberContours(100);
      g2d->SetMarkerStyle(2);
//      g2d->SetMarkerSize(2);
      g2d->Draw("Pcol");

      gPad->Modified(); gPad->Update();
      cout << gPad->GetView() << endl;

*/  
  
  
  
  
  /*      // Draw Coordinate VS Coordinate plots --------------------------

      gStyle->SetPalette(54);
      gStyle->SetNumberContours(100);
      
      cCvC[j]->cd(i);
      treeDraw->SetMarkerStyle(6);
      treeDraw->SetMarkerSize(8);
      treeDraw->Draw(drawStringCvC[i-1].c_str() ,"","contz");
      h2 = (TH2F*) cCvC[j]->cd(i)->GetPrimitive("htemp");
      gPad->GetListOfPrimitives()->Print();

      if (i < 4) {
      cCvCS[j]->cd(i);
      cout << i << endl;
      treeDraw->Draw(drawStringCvCS[i-1].c_str(),"","contz");
      }
*/

/*
      // Draw histograms of coordinate values -------------------------

      cHisto[j]->cd(i);
      treeDraw->Draw(drawStringHisto[i-1].c_str());
      h = (TH1F*)  cHisto[j]->cd(i)->GetPrimitive("htemp");
      h->SetName(Form("htemp%i%i",j+1,i));
      h->Write();

*/

    }

    //cM[j]->Write();
    //cT[j]->Write();
    //c[j]->Write();
    //cCvC[j]->Write();
    //cCvCS[j]->Write();
    //cHisto[j]->Write();
  }
}



