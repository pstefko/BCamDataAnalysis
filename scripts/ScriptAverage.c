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
#include <algorithm>
#include <string>
#include <sstream>
using namespace std;

void AverageOneBranch(string branchName, Int_t avgSize, TTree *tree, TTree *treeAvg) ; 
void EnableBranch(string branchName, TTree *treeAvg) ;
void DrawGraphs(TTree *treeAvg, TTree *treeMagnet, TTree *treeMagnetTemperature, TTree *treeTemperature[3][4], TTree *treeVoltage, TTree *treeCoordinateWithTemperature[3]) ;

void MakeAverage(Float_t avrgSize) {

  TFile *inFile = new TFile("./../ResultTrees/OriginalData/BCamData_Skimmed.root"); 	// Input file 
  TTree *tree = (TTree*) inFile->Get("treeBCam");	// Tree from an input file containing Skimmed data from the .csv files arranged in a big tree
  TTree *treeAvg =  new TTree ("treeAvg", "BCam data averaged");	// Tree in an output file containing the averaged values
  treeAvg->SetDirectory(0);
  
  TTree* treeMagnet = 			(TTree*) inFile->Get("treeMagnet");
  TTree* treeMagnetTemperature = 	(TTree*) inFile->Get("treeMagnetTemperature");
  TTree* treeVoltage = 			(TTree*) inFile->Get("treeVoltage");
  
  string boxes[12] = {"1A","1B","1C","1T","2A","2B","2C","2T","3A","3B","3C","3T"};

  TTree *treeTemperature[3][4];
  TTree *treeCoordinateWithTemperature[3];
  
  for (int i = 0; i < 3; i++) {
    treeCoordinateWithTemperature[i] = (TTree*) inFile->Get(Form("treeTemperatureWithCoordinatesT%i",i+1));
    for (int j = 0; j < 4; j++) {
      treeTemperature[i][j] = (TTree*) inFile->Get(Form("treeTemperature%s",boxes[(i+1)*j].c_str()));
    }
  }
  
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

  DrawGraphs(treeAvg, treeMagnet, treeMagnetTemperature, treeTemperature, treeVoltage, treeCoordinateWithTemperature);		// Draw the appropriate graphs and write them into ouput file

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

void DrawGraphs(TTree *treeBCam, TTree *treeMagnet, TTree *treeMagnetTemperature, TTree *treeTemperature[3][4], TTree *treeVoltage, TTree *treeCoordinateWithTemperature[3]) {

  // Procedure that Draws the graphs into three canvases (one for each T) 
  
  TTree* treeDraw = treeBCam;

  Int_t whichPlotsToDraw[10] = {0,0,1,0,0,0,0,0,0,0};

  double y1,y2,yr,max,min,scale;
  TLatex *t;
  TLine *line, *line1, *line2, *line3, *line4;
  TPaveText *title;

  TCanvas* cC[3];
  TCanvas* cCwM[3];
  TCanvas* cMT;
  TCanvas* cCwTemp[3];
  TCanvas* cCwMTemp[3];
  TCanvas* cTempwV[3];
  TCanvas* cTempwM[3];
  TCanvas* cCvC[3];
  TCanvas* cCvTemp[3];
  TCanvas* cCvCS[3];
  TCanvas* cHisto[3];
  TCanvas* cTest;

  TGraph* g, *g1, *g2, *temp, *gM, *gV, *gT, *gT0, *gT1, *gT2, *gT3;
  TGraph2D *g2d;
  TMultiGraph *mg;
  TCanvas* cDump = new TCanvas("Dump","Dump", 300,200);
  TH1F* h;
  TH2F* h2;

  for (int j = 0; j < 3; j++) {
    
    if (whichPlotsToDraw[0] == 1) {cC[j] = new TCanvas(Form("cT%iCoordinateVsTime",j+1),Form("cT%iCoordinateVsTime",j+1),1400,700); 					cC[j]->Divide(3,2);}
    
    if (whichPlotsToDraw[1] == 1) {cCwTemp[j] = new TCanvas(Form("cT%iCoordinateVsTimeWithTemperature",j+1),Form("cT%iCoordinateVsTimeWithTemperature",j+1),1400,700); 	cCwTemp[j]->Divide(3,2);}
    
    if (whichPlotsToDraw[2] == 1) {cCwMTemp[j] = new TCanvas(Form("cT%iCoordinateVsTimeWithMagnetTemp",j+1),Form("cT%iCoordinateVsTimeWithMagnetTemp",j+1),1400,700); 	cCwMTemp[j]->Divide(3,2);}
     
    if (whichPlotsToDraw[3] == 1) {cCwM[j] = new TCanvas(Form("cT%iCoordinateVsTimeWithMagnet",j+1),Form("cT%iCoordinateVsTimeWithMagnet",j+1),1400,700); 		cCwM[j]->Divide(3,2);}

    if (whichPlotsToDraw[4] == 1) {cTempwM[j] = new TCanvas(Form("cT%iTemperatureVsTimeWithMagnet",j+1),Form("cT%iTemperatureVsTimeWithMagnet",j+1),1400,700); 		cTempwM[j]->Divide(2,2);}
    
    if (whichPlotsToDraw[5] == 1) {cCvC[j] = new TCanvas(Form("cT%iCoordinateVsCoordinate",j+1),Form("cT%iCoordinateVsCoordinate",j+1),1400,700); 			cCvC[j]->Divide(3,2); 
                                   cCvCS[j] = new TCanvas(Form("cT%iCoordinateVsCoordinate(AvsC)",j+1),Form("cT%iCoordinateVsCoordinate(AvsC)",j+1),1400,400); 		cCvCS[j]->Divide(3,1);}

    if (whichPlotsToDraw[6] == 1) {if (j == 0) {cMT = new TCanvas("cMagnetTemperatureWithMagnet","cMagnetTemperatureWithMagnet",800,600);}}
 
    if (whichPlotsToDraw[7] == 1) {cHisto[j] = new TCanvas(Form("cT%iCoordinates",j+1),Form("cT%iCoordinates",j+1),1400,700); 						cHisto[j]->Divide(3,2);}

    if (whichPlotsToDraw[8] == 1) {cTempwV[j] = new TCanvas(Form("cT%iTemperatureWithVoltage",j+1),Form("cT%iTemperatureWithVoltage",j+1),1400,700); 			cTempwV[j]->Divide(2,2);}

    if (whichPlotsToDraw[9] == 1) {cCvTemp[j] = new TCanvas(Form("cT%iCoordinateVsTemperature",j+1),Form("cT%iCoordinateVsTemperature",j+1),1400,700); 			cCvTemp[j]->Divide(3,2);}

    ostringstream js; js << j+1;
    
    string drawStringCvT[] = {"x"+js.str()+"1:t"+js.str()+"1","y"+js.str()+"1:t"+js.str()+"1","z"+js.str()+"1:t"+js.str()+"1",
      			      "x"+js.str()+"2:t"+js.str()+"2","y"+js.str()+"2:t"+js.str()+"2","z"+js.str()+"2:t"+js.str()+"2"};
    string drawStringCvC[] = {"x"+js.str()+"1:y"+js.str()+"1:t"+js.str()+"1","x"+js.str()+"1:z"+js.str()+"1:t"+js.str()+"1","y"+js.str()+"1:z"+js.str()+"1:t"+js.str()+"1",
         		      "x"+js.str()+"2:y"+js.str()+"2:t"+js.str()+"2","x"+js.str()+"2:z"+js.str()+"2:t"+js.str()+"2","y"+js.str()+"2:z"+js.str()+"2:t"+js.str()+"2"};
    string drawStringCvCS[] = {"x"+js.str()+"1:x"+js.str()+"2:t"+js.str()+"1","y"+js.str()+"1:y"+js.str()+"2:t"+js.str()+"1","z"+js.str()+"1:z"+js.str()+"2:t"+js.str()+"1"};
    string drawStringHisto[] = {"x"+js.str()+"1","y"+js.str()+"1","z"+js.str()+"1","x"+js.str()+"2","y"+js.str()+"2","z"+js.str()+"2"};
    string drawStringTempvT[] = {"Temp"+js.str()+"A:t"+js.str()+"A","Temp"+js.str()+"B:t"+js.str()+"B","Temp"+js.str()+"C:t"+js.str()+"C","Temp"+js.str()+"T:t"+js.str()+"T"};
    string drawStringCvTemp[] = {"x1:temperature:t","y1:temperature:t","z1:temperature:t","x2:temperature:t","y2:temperature:t","z2:temperature:t"};
    string coordString[] = {"xA","yA","zA","xC","yC","zC"};
    string coordStringCvC[] = {"xA vs. yA","xA vs. zA","yA vs. zA","xC vs. yC","xC vs. zC","yC vs. zC"};
    string coordStringCvCS[] = {"xA vs. xC","yA vs. yC","zA vs. zC"};

    for (int i = 0; i < 6; i++) {

      if (whichPlotsToDraw[0] == 1) {
	// Draw Coordinate VS Time plots -----------------------------

	cDump->cd();

	treeDraw->Draw(drawStringCvT[i].c_str());
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g = (TGraph*)temp->Clone("mygraph");

	g->SetMarkerColor(4);
	
	cC[j]->cd(i+1);

	gStyle->SetTimeOffset(0); 

	g->SetTitle(coordString[i].c_str());
	g->Draw("AL");

	g->GetXaxis()->SetTimeDisplay(1);
	g->GetXaxis()->SetTitle("t");
	g->GetYaxis()->SetTitle(coordString[i].c_str());

	gPad->Modified();
	gPad->Update();

	// Draw scale

	max = g->GetHistogram()->GetMaximum(); 
	min = g->GetHistogram()->GetMinimum(); 
	scale = ceil(((max-min)/4)*1000*100.)/100.;	// scale in mm

	t = new TLatex(.955,.75,Form("%.2f mm",scale));

	//Draw TextPad with the scale
	t->SetNDC();
	t->SetTextFont(42);
	t->SetTextAngle(90);
	t->Draw();

	//Draw a line representing scale

	yr = gPad->GetY2()-gPad->GetY1();
	y1 = (max-gPad->GetY1())/ yr; 
	y2 = (max-scale/1000.-gPad->GetY1())/ yr;

	line = new TLine(0.92,y1,0.92,y2);
	line->SetLineColor(kRed);
	line->SetLineWidth(2);
	line->SetNDC();
	line->Draw();

	// Draw lines representing interesting days (around Xmas)
	
	line2 = new TLine(1450440000,gPad->GetUymin(),1450440000,gPad->GetUymax());	// line on 18.12.2015 12:00:00
	line2->SetLineColor(kRed);
	line2->SetLineWidth(2);
	line2->Draw();

	line2 = new TLine(1451908800,gPad->GetUymin(),1451908800,gPad->GetUymax());	// line on 4.1.2015 12:00:00
	line2->SetLineColor(kRed);
	line2->SetLineWidth(2);
	line2->Draw();


      }

      if (whichPlotsToDraw[1] == 1) {
	// Draw Coordinate VS Time with IT Box Temperature info ------------------------------

	max = treeDraw->GetMaximum(drawStringHisto[i].c_str()); 	// Scale used to draw the magnet info
	min = treeDraw->GetMinimum(drawStringHisto[i].c_str());
	scale = max-min;

	cDump->cd();

	treeDraw->Draw(drawStringCvT[i].c_str());	// Draw Coordinate and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g1 = (TGraph*)temp->Clone("mygraph");


	treeTemperature[j][0]->Draw(Form("temperature/3.*%f+%f:t",scale/2.5,min-scale/0.8));
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g2 = (TGraph*)temp->Clone("mygraph");

	// Modify graphs applearance

	g1->SetMarkerColor(4);
	g1->SetMarkerStyle(2);

	g2->SetLineColor(2);
	g2->SetLineWidth(1);

	mg = new TMultiGraph();	// Multigraph storing the graphs
	mg->Add(g1,"L");
	mg->Add(g2,"L");

	cout << gPad << endl;
	cCwTemp[j]->cd(i+1);
	cout << gPad << endl;

	gStyle->SetTimeOffset(0);

	mg->SetTitle(coordString[i].c_str());
	mg->Draw("AP");
	mg->GetXaxis()->SetTimeDisplay(1);
	mg->GetXaxis()->SetLabelSize(0.05);


	gPad->Modified();

	// Draw line scale

	max = g1->GetHistogram()->GetMaximum(); 
	min = g1->GetHistogram()->GetMinimum(); 
	scale = ceil(((max-min)/2.5)*1000*100.)/100.;	// scale in mm

	t = new TLatex(.96,.7,Form("%.2f mm",scale));

	//Draw TextPad with the scale
	t->SetNDC();
	t->SetTextFont(42);
	t->SetTextSize(0.06);
	t->SetTextAngle(90);
	t->Draw();

	//Draw a line representing scale

	yr = gPad->GetY2()-gPad->GetY1();
	y1 = (max-gPad->GetY1())/ yr; 
	y2 = (max-scale/1000.-gPad->GetY1())/ yr;
	line = new TLine(0.92,y1,0.92,y2);
	line->SetLineColor(kRed);
	line->SetLineWidth(2);
	line->SetNDC();
	line->Draw();

      } 

      if (whichPlotsToDraw[2] == 1) {
	// Draw Coordinate VS Time with Magnet Temperature info ------------------------------
	max = treeDraw->GetMaximum(drawStringHisto[i].c_str()); 	// Scale used to draw the magnet info
	min = treeDraw->GetMinimum(drawStringHisto[i].c_str());
	scale = max-min;

	cDump->cd();

	treeDraw->Draw(drawStringCvT[i].c_str());	// Draw Coordinate and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g1 = (TGraph*)temp->Clone("mygraph");

	treeMagnetTemperature->Draw(Form("temp2/25.*%f+%f:t",scale*3,min-scale*3.));	// Draw temperature graph
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g2 = (TGraph*)temp->Clone("mygraph");

	// Modify graphs applearance

	g1->SetMarkerColor(4);
	g1->SetMarkerStyle(2);

	g2->SetLineColor(2);
	g2->SetLineWidth(1);

	mg = new TMultiGraph();	// Multigraph storing the graphs
	mg->Add(g1,"L");
	mg->Add(g2,"L");

	cCwMTemp[j]->cd(i+1);

	gStyle->SetTimeOffset(0);

	mg->SetTitle(coordString[i].c_str());
	mg->Draw("AP");
	mg->GetXaxis()->SetTimeDisplay(1);
	mg->GetXaxis()->SetLabelSize(0.05);


	gPad->Modified();

	// Draw line scale

	max = g1->GetHistogram()->GetMaximum(); 
	min = g1->GetHistogram()->GetMinimum(); 
	scale = ceil(((max-min)/2.5)*1000*100.)/100.;	// scale in mm

	t = new TLatex(.96,.7,Form("%.2f mm",scale));

	//Draw TextPad with the scale
	t->SetNDC();
	t->SetTextFont(42);
	t->SetTextSize(0.06);
	t->SetTextAngle(90);
	t->Draw();

	//Draw a line representing scale

	yr = gPad->GetY2()-gPad->GetY1();
	y1 = (max-gPad->GetY1())/ yr; 
	y2 = (max-scale/1000.-gPad->GetY1())/ yr;
	line = new TLine(0.92,y1,0.92,y2);
	line->SetLineColor(kRed);
	line->SetLineWidth(2);
	line->SetNDC();
	line->Draw();
	
	// Draw lines representing interesting days (around Xmas)
	
	line2 = new TLine(1450440000,gPad->GetUymin(),1450440000,gPad->GetUymax());	// line on 18.12.2015 12:00:00
	line2->SetLineColor(kRed);
	line2->SetLineWidth(2);
	line2->Draw();

	line2 = new TLine(1451908800,gPad->GetUymin(),1451908800,gPad->GetUymax());	// line on 4.1.2015 12:00:00
	line2->SetLineColor(kRed);
	line2->SetLineWidth(2);
	line2->Draw();

      }   


      if (whichPlotsToDraw[3] == 1) {
	// Draw Coordinate VS Time with Magnet info ------------------------------

	cDump->cd();

	max = treeDraw->GetMaximum(drawStringHisto[i].c_str()); 	// Scale used to draw the magnet info
	min = treeDraw->GetMinimum(drawStringHisto[i].c_str());
	scale = max-min;

	treeDraw->Draw(drawStringCvT[i].c_str());	// Draw Coordinate and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g1 = (TGraph*)temp->Clone("mygraph");

	treeMagnet->Draw(Form("(current*polarity)/6000.*%f+%f:t",scale/8.,min-scale/6));	//Draw Magnet and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	g2 = (TGraph*)temp->Clone("mygraph");

	// Modify graphs applearance

	g1->SetMarkerColor(4);
	g1->SetMarkerStyle(2);
	g1->SetLineWidth(1);
	g2->SetLineColor(2);
	g2->SetLineWidth(1);

	mg = new TMultiGraph();	// Multigraph storing the graphs
	mg->Add(g1,"L");
	mg->Add(g2,"L");

	cCwM[j]->cd(i+1);
	gPad->SetTicks(0,1);

	mg->SetTitle(coordString[i].c_str());
	mg->Draw("AP");
	mg->GetXaxis()->SetTimeDisplay(1);
	gStyle->SetTimeOffset(0);

	//mg->GetXaxis()->SetTitle("t");
	//mg->GetYaxis()->SetTitle(drawStringHisto[i].c_str());
	mg->GetXaxis()->SetLabelSize(0.05);

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
	scale = ceil(((max-min)/2.5)*1000*100.)/100.;	// scale in mm

	t = new TLatex(.96,.7,Form("%.2f mm",scale));

	//Draw TextPad with the scale
	t->SetNDC();
	t->SetTextSize(0.06);
	t->SetTextFont(42);
	t->SetTextAngle(90);
	t->Draw();

	//Draw a line representing scale

	yr = gPad->GetY2()-gPad->GetY1();
	y1 = (max-gPad->GetY1())/ yr; 
	y2 = (max-scale/1000.-gPad->GetY1())/ yr;
	line = new TLine(0.92,y1,0.92,y2);
	line->SetLineColor(kRed);
	line->SetLineWidth(2);
	line->SetNDC();
	line->Draw();

      }

      if (whichPlotsToDraw[4] == 1) {
	// Draw Magnet Temperatures with magnet info ---------------------------------------------------

	cDump->cd();

	max = treeDraw->GetMaximum(drawStringHisto[i].c_str()); 	// Scale used to draw the magnet info
	min = treeDraw->GetMinimum(drawStringHisto[i].c_str());
	scale = max-min;

	treeMagnetTemperature->Draw("temp2:t");	// Draw temperature graph
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	gT2 = (TGraph*)temp->Clone("mygraph");

	treeMagnet->Draw("(current*polarity)/6000.*1+19:t");	//Draw Magnet and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	gM = (TGraph*)temp->Clone("mygraph");

	// Line for zero current

	line1 = new TLine(treeMagnet->GetMinimum("t"),19,treeMagnet->GetMaximum("t"),19);
	line1->SetLineColor(kBlack);
	line1->SetLineWidth(1);
	line1->SetLineStyle(2);

	gT2->SetMarkerStyle(2);
	gT2->SetLineColor(4);
	gT2->SetLineWidth(2);

	gM->SetLineColor(2);
	gM->SetLineWidth(1);

	mg = new TMultiGraph();	// Multigraph storing the graphs
	mg->Add(gT2,"L");
	mg->Add(gM,"L");

	cMT->cd();

	mg->SetTitle("Temperature in magnet;t;B2Temp [C]");
	mg->Draw("AP");
	gStyle->SetTimeOffset(0);
	mg->GetXaxis()->SetTimeDisplay(1);

	line1->Draw();
	gPad->Modified();

      }

      if (whichPlotsToDraw[5] == 1) {
	// Draw Coordinate VS Coordinate plots --------------------------

	gStyle->SetPalette(54);
	gStyle->SetNumberContours(100);

	cCvC[j]->cd(i+1);
	treeDraw->SetMarkerStyle(20);
	treeDraw->SetMarkerSize(0.8);
	treeDraw->Draw(drawStringCvC[i].c_str(),"","colz");
	h = (TH1F*)  cCvC[j]->cd(i+1)->GetPrimitive("htemp");
	h->SetTitle(coordStringCvC[i].c_str());

	if (i < 3) {

	  cCvCS[j]->cd(i+1);
	  treeDraw->Draw(drawStringCvCS[i].c_str(),"","colz");
	  h = (TH1F*)  cCvCS[j]->cd(i+1)->GetPrimitive("htemp");
	  h->SetTitle(coordStringCvCS[i].c_str());
	}

      }

      if (whichPlotsToDraw[6] == 1) {
	// Draw Box temperature vs time with Magnet -------------------------------------

	cDump->cd();

	max = treeTemperature[j][0]->GetMaximum("temperature"); 	// Scale used to draw the magnet info
	min = treeTemperature[j][0]->GetMinimum("temperature");
	scale = max-min;

	treeTemperature[j][0]->Draw("temperature:t");	// Draw temperature graph
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	gT = (TGraph*)temp->Clone("mygraph");

	treeMagnet->Draw(Form("(current*polarity)/6000.*%f+%f:t",scale/8.,min-scale/6));	//Draw Magnet and dump
	temp = (TGraph*)  cDump->GetPrimitive("Graph");
	gM = (TGraph*)temp->Clone("mygraph");

	gT->SetMarkerStyle(2);
	gT->SetLineColor(4);
	gT->SetLineWidth(2);

	gM->SetLineColor(2);
	gM->SetLineWidth(1);

	mg = new TMultiGraph();	// Multigraph storing the graphs
	mg->Add(gT,"L");
	mg->Add(gM,"L");

	cTempwM[j]->cd(i+1);

	mg->SetTitle(Form("%s;t;Temp [K]",drawStringTempvT[i].substr(0,6).c_str()));
	mg->Draw("AP");
	gStyle->SetTimeOffset(0);
	mg->GetXaxis()->SetTimeDisplay(1);

	gPad->Modified();

      }

      if (whichPlotsToDraw[7] == 1) {
	// Draw histograms of coordinate values -------------------------

	cHisto[j]->cd(i+1);
	treeDraw->Draw(drawStringHisto[i].c_str());
	h = (TH1F*)  cHisto[j]->cd(i+1)->GetPrimitive("htemp");
	h->SetName(Form("htemp%i%i",j+1,i));
	h->Write();


      }

      if (whichPlotsToDraw[8] == 1) {
      // Draw Box temperature vs time with Voltage -------------------------------------

	if (i < 4) {

	  cDump->cd();

	  max = treeTemperature[j][i]->GetMaximum("temperature"); 	// Scale used to draw the magnet info
	  min = treeTemperature[j][i]->GetMinimum("temperature");
	  scale = max-min;

	  treeTemperature[j][i]->Draw("temperature:t");	// Draw temperature graph
	  temp = (TGraph*)  cDump->GetPrimitive("Graph");
	  gT = (TGraph*)temp->Clone("mygraph");

	  treeVoltage->Draw(Form("voltage/250*%f+%f:t",scale/2,min-scale/2));	//Draw Magnet and dump
	  temp = (TGraph*)  cDump->GetPrimitive("Graph");
	  gV = (TGraph*)temp->Clone("mygraph");

	  gT->SetMarkerStyle(2);
	  gT->SetLineColor(4);
	  gT->SetLineWidth(2);

	  gV->SetLineColor(2);
	  gV->SetLineWidth(1);

	  mg = new TMultiGraph();	// Multigraph storing the graphs
	  mg->Add(gT,"L");
	  mg->Add(gV,"L");

	  cTempwV[j]->cd(i+1);

	  mg->SetTitle(Form("%s;t;Temp [C]",drawStringTempvT[i].substr(0,6).c_str()));
	  mg->Draw("AP");
	  gStyle->SetTimeOffset(0);
	  mg->GetXaxis()->SetTimeDisplay(1);
	  mg->GetXaxis()->SetLabelSize(0.05);
	  mg->GetXaxis()->SetTitleSize(0.05);
	  mg->GetYaxis()->SetLabelSize(0.05);
	  mg->GetYaxis()->SetTitleSize(0.05);

	  gPad->Modified();
	}

      }


      if (whichPlotsToDraw[9] == 1) {
	// Draw Coordinate vs Temperature ----------------------------------------------

	gStyle->SetPalette(54);
	gStyle->SetNumberContours(100);
	treeCoordinateWithTemperature[j]->SetMarkerStyle(20);
	treeCoordinateWithTemperature[j]->SetMarkerSize(0.8);

	cCvTemp[j]->cd(i+1);
	treeCoordinateWithTemperature[j]->Draw(drawStringCvTemp[i].c_str() ,"","colz");
	h2 = (TH2F*) cCvTemp[j]->cd(i+1)->GetPrimitive("htemp");

      }

    }

    /*    cCwM[j]	->Write();
	  cTempwV[j]	->Write();
	  cTempwM[j]	->Write();
	  cCwTemp[j]	->Write();
	  cC[j]	->Write();
	  cCvC[j]	->Write();
	  cCvTemp[j]	->Write();
	  cCvCS[j]	->Write();
    //    cHisto[j]->Write();



    cCwM[j]	->Print(Form("./../ResultTrees/Pdfs/CwM%i.pdf",j+1));
    cTempwV[j]	->Print(Form("./../ResultTrees/Pdfs/TempwV%i.pdf",j+1));
    cTempwM[j]	->Print(Form("./../ResultTrees/Pdfs/TempwM%i.pdf",j+1));
    cCwTemp[j]	->Print(Form("./../ResultTrees/Pdfs/CwTemp%i.pdf",j+1));
    cC[j]	->Print(Form("./../ResultTrees/Pdfs/C%i.pdf",j+1));
    cCvC[j]	->Print(Form("./../ResultTrees/Pdfs/CvC%i.pdf",j+1));
    cCvTemp[j]	->Print(Form("./../ResultTrees/Pdfs/CvT%i.pdf",j+1));
    cCvCS[j]	->Print(Form("./../ResultTrees/Pdfs/CvCS%i.pdf",j+1));
  */  
  }
}



