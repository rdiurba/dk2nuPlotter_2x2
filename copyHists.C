#include "TH1.h"
#include "TGraph.h"
#include "TH2.h"
#include "TF1.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TMath.h"
#include "TFile.h"

void copyHists(std::string hornCurrent){
TFile f(Form("test%s.root",hornCurrent.c_str()));
double scaleFactor=1E21*0.67*TMath::Pi()*0.67;
TH1D* h501=(TH1D*)f.Get("h501");
TH1D* h502=(TH1D*)f.Get("h502");
TH1D* h503=(TH1D*)f.Get("h503");
TH1D* h504=(TH1D*)f.Get("h504");
h501->Scale(1.f/scaleFactor);
h502->Scale(1.f/scaleFactor);
h503->Scale(1.f/scaleFactor);
h504->Scale(1.f/scaleFactor);
std::string nultx[] ={"#nu_{e}", "#bar{#nu}_{e}", "#nu_{#mu}", "#bar{#nu}_{#mu}"};

h501->GetYaxis()->SetTitle(Form("Flux (%s/POT/GeV/m^{2})",nultx[0].c_str()));
h502->GetYaxis()->SetTitle(Form("Flux (%s/POT/GeV/m^{2})",nultx[1].c_str()));
h504->GetYaxis()->SetTitle(Form("Flux (%s/POT/GeV/m^{2})",nultx[3].c_str()));
h503->GetYaxis()->SetTitle(Form("Flux (%s/POT/GeV/m^{2})",nultx[2].c_str()));
h501->GetXaxis()->SetTitle("E (GeV)");
h502->GetXaxis()->SetTitle("E (GeV)");
h503->GetXaxis()->SetTitle("E (GeV)");
h504->GetXaxis()->SetTitle("E (GeV)");

TFile outFile(Form("dk2nuHist_2x2_%s.root",hornCurrent.c_str()),"recreate");

TH1D* hist_nue=(TH1D*)h501->Clone(Form("hist_%s_nue",hornCurrent.c_str()));
TH1D* hist_nuebar=(TH1D*)h502->Clone(Form("hist_%s_nuebar",hornCurrent.c_str()));
TH1D* hist_numu=(TH1D*)h503->Clone(Form("hist_%s_numu",hornCurrent.c_str()));
TH1D* hist_numubar=(TH1D*)h504->Clone(Form("hist_%s_numubar",hornCurrent.c_str()));
outFile.cd();
hist_nue->Write();
hist_nuebar->Write();
hist_numu->Write();
hist_numubar->Write();
//outFile.Write();



}
