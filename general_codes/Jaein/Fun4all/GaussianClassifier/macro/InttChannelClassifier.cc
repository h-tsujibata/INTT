#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TDirectory.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TH2D.h"

using namespace std;
//////////////
// Funtions //
//////////////
double SingleGaussianFit(TH1D *hist, double &mean1, double &sigma1); 
//Function to do fitting return value : value for HotChannel cut

void GetFirstFilledBinCenter(TH1D *hist, double &a, double &b);      
//Function to find bin center not used for now.                                                                    

/////////////////////
//Global parameters//
/////////////////////
int chip = 26;
int mod = 14;
double sig_cut = 4.1;
bool Writecsv = false;
//chip : # of chips on each half ladder(defalt : 26)
//mod : # of moudles for each felix server(default : 14)
//sig_cut : sigma cut for hot/cold channel determination default : 4.0
//Writecsv : flag to write csv file (default : false )

/////////////
//File Path//
/////////////
std::string map_input_path =  "/sphenix/tg/tg01/commissioning/INTT/work/jaein/jaein_gitrepos_working/INTT/general_codes/Jaein/Fun4all/GaussianClassifier/HitMapFile/";
std::string root_output_path = "/sphenix/tg/tg01/commissioning/INTT/work/jaein/jaein_gitrepos_working/INTT/general_codes/Jaein/Fun4all/GaussianClassifier/HotMaploader/new/rootfile/";
std::string csv_output_path = "/sphenix/tg/tg01/commissioning/INTT/QA/hotdeadmap/csv_file/2023/";
//map_input_path : location of the HitMap file created by Fun4All_Intt_HitMap.C
//root_output_path : output file path
//csv_output_path : csv output file path (used for Grafana online monitoring)


struct Half_Chip {
  int felix_id_;
  int module_id_;
  int chip_id_;
};
////////////////////////////
//List of Half entry chips//
////////////////////////////
vector<Half_Chip> half_chips = {
    // Felix 0
    {0, 7, 15},
    // Felix 2
    {2, 9, 16},
    // Felix 3
    {3, 13, 21},
    {3, 13, 23},
    {3, 13, 25},
    // Felix 7
    {7, 0, 1},
    {7, 0, 2},
    {7, 0, 3},
    {7, 0, 4},
    {7, 0, 5},
    {7, 0, 14},
    {7, 0, 15},
    {7, 0, 16},
    {7, 0, 17},
    {7, 0, 18},
    {7, 0, 7},
    {7, 0, 9},
    {7, 0, 11},
    {7, 0, 13},
    {7, 0, 20},
    {7, 0, 22},
    {7, 0, 24},
    {7, 0, 26},
    {7, 1, 1},
    {7, 1, 2},
};
void InttChannelClassifier(int runnumber = 20869) //runnumber
{
  gStyle->SetOptFit();
  ////////////////////////////////////////
  // Load HitMap                        //
  ////////////////////////////////////////
  int in_sig = 10*sig_cut;
  std::string rootfilename = map_input_path + "hitmap_" + to_string(runnumber) + ".root";
  TFile *file = nullptr;
  file = TFile::Open(rootfilename.c_str(), "READ");


  ///////////////////////////////////////////////////
  // Create Histogram for Hot and Dead channel map //
  ///////////////////////////////////////////////////
  
  TH2D *h2_AllMap[8][14]; // Original HitMap
  TH2D *h2_ColdMap[8][14];// 2D histogram for coldmap
  TH2D *h2_HalfMap[8][14];// 2D histogram for halfmap (half entry chips)
  TH2D *h2_DeadMap[8][14];// 2D histogram for deadmap
  TH2D *h2_HotMap[8][14]; // 2D histogram for hotmap
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 14; j++)
    {
      h2_AllMap[i][j] = (TH2D *)file->Get(Form("HitMap_%d_%d", i, j));
      h2_ColdMap[i][j] = new TH2D(Form("ColdMap_%d_%d", i, j), Form("ColdMap_%d_%d", i, j), 128, 0, 128, 27, 0, 27);
      h2_HalfMap[i][j] = new TH2D(Form("HalfMap_%d_%d", i, j), Form("HalfMap_%d_%d", i, j), 128, 0, 128, 27, 0, 27);
      h2_HotMap[i][j] = new TH2D(Form("HotMap_%d_%d", i, j), Form("HotMap_%d_%d", i, j), 128, 0, 128, 27, 0, 27);
      h2_DeadMap[i][j] = new TH2D(Form("DeadMap_%d_%d", i, j), Form("DeadMap_%d_%d", i, j), 128, 0, 128, 27, 0, 27);
    }
  }
  //////////////////////////////////////////
  // Define condition for the hot channel //
  //////////////////////////////////////////
  double HotChannelCut_A_Fit[8][14] = {0.};
  double HotChannelCut_B_Fit[8][14] = {0.};
  double ColdChannelCut_A_Fit[8][14] = {0.};
  double ColdChannelCut_B_Fit[8][14] = {0.};
  double par_meanA[8][14] = {0.};  // Array to save the mean & sigma value, [0][module] = mean, [1][module] = sigma
  double par_sigmaA[8][14] = {0.}; // Array to save the mean & sigma value, [0][module] = mean, [1][module] = sigma
  double par_meanB[8][14] = {0.};  // Array to save the mean & sigma value, [0][module] = mean, [1][module] = sigma
  double par_sigmaB[8][14] = {0.}; // Array to save the mean & sigma value, [0][module] = mean, [1][module] = sigma
  /////////////////////////////////////////////////
  // Create TFile and TTree to save information  //
  // TTree is mainly used to convert to CDBTTree //
  /////////////////////////////////////////////////
  std::string csvfilename = csv_output_path + "NumOfHot.csv";
  std::ofstream csvFile(csvfilename, std::ios::app);
  int NumOfHot = 0;
  if (!csvFile.is_open())
  {
    std::cout<<csvfilename<<std::endl;
    std::cerr << "Unable to open the file." << std::endl;
    return;
  }
  std::string outputfile = root_output_path + "InttHotDeadMap_"+to_string(runnumber)+"_"+to_string(in_sig)+".root";
  //TFile *sfile = new TFile(Form("./rootfile/InttHotDeadMap_%d_%d.root", runnumber, in_sig), "RECREATE");
  TFile *sfile = new TFile(outputfile.c_str(), "RECREATE");
  TTree *st = new TTree("tree", "tree");
  double ch_entry, mean_gaus, sigma_gaus = 0.;
  int felix_, module_, chip_id_, chan_, type_, flag_;
  st->Branch("felix", &felix_);
  st->Branch("module", &module_);
  st->Branch("chip_id", &chip_id_);
  st->Branch("chan", &chan_);
  st->Branch("flag", &flag_);
  st->Branch("ch_entry", &ch_entry);
  st->Branch("type", &type_);
  st->Branch("mean", &mean_gaus);
  st->Branch("sigma", &sigma_gaus);

  ///////////////////////////////////
  // Fill the Dead & Cold & HotMap //
  ///////////////////////////////////
  TCanvas *canA[8];
  TCanvas *canB[8];
  for (int i = 0; i < 8; i++)
  {
    canA[i] = new TCanvas(Form("TypeA_Felix_%d", i), Form("TypeA_Felix_%d", i), 1200, 1200);
    canB[i] = new TCanvas(Form("TypeB_Felix_%d", i), Form("TypeB_Felix_%d", i), 1200, 1200);
    canA[i]->Divide(7, 2);
    canB[i]->Divide(7, 2);
  }
  TH1D *h1_hist_fit_A[8][14];
  TH1D *h1_hist_fit_B[8][14];
  double mean_first = 0;
  for (int felix = 0; felix < 8; felix++)
  {
    for (int i = 0; i < 14; i++)
    {
      h1_hist_fit_A[felix][i] = new TH1D(Form("h1_hist_fit_A%d_%d", felix, i), Form("h1_hist_fit_A%d_%d", felix, i), 100, 0, 0.03);
      h1_hist_fit_B[felix][i] = new TH1D(Form("h1_hist_fit_B%d_%d", felix, i), Form("h1_hist_fit_B%d_%d", felix, i), 100, 0, 0.03);
      for (int j = 1; j < 27; j++)
      {
        for (int chan = 0; chan < 128; chan++)
        {
          if (j < 6 || (j > 13 && j < 19)) // Condition for type B
          {
            h1_hist_fit_B[felix][i]->Fill(h2_AllMap[felix][i]->GetBinContent(chan + 1, j + 1));
          }
          else
          {
            h1_hist_fit_A[felix][i]->Fill(h2_AllMap[felix][i]->GetBinContent(chan + 1, j + 1));
          }
        }
      }
      double mean, sigma;
      canA[felix]->cd(i + 1);
      std::cout << "CHENCK !! Felix : " << felix << " module : " << i << std::endl;
      HotChannelCut_A_Fit[felix][i] = SingleGaussianFit(h1_hist_fit_A[felix][i], mean, sigma);
      ColdChannelCut_A_Fit[felix][i] = mean - sig_cut * sigma;
      if (mean_first == 0)
        mean_first = mean;
      if (mean < mean_first * 0.7)
      {
        HotChannelCut_A_Fit[felix][i] = 1;
        ColdChannelCut_A_Fit[felix][i] = 1;
        if (mean - sig_cut * sigma < 0)
        {
          mean = -1;
          sigma = -1;
        }
      }
      par_meanA[felix][i] = mean;
      par_sigmaA[felix][i] = sigma;
      canB[felix]->cd(i + 1);
      HotChannelCut_B_Fit[felix][i] = SingleGaussianFit(h1_hist_fit_B[felix][i], mean, sigma);
      ColdChannelCut_B_Fit[felix][i] = mean - sig_cut * sigma;
      if (mean < mean_first * 0.7)
      {
        HotChannelCut_B_Fit[felix][i] = 1;
        ColdChannelCut_B_Fit[felix][i] = 1;
        if (mean - sig_cut * sigma < 0)
        {
          mean = -1;
          sigma = -1;
        }
      }
      par_meanB[felix][i] = mean;
      par_sigmaB[felix][i] = sigma;
    }
  }

  sfile->cd();
  TDirectory *dir[8];
  for (int felix = 0; felix < 8; felix++)
  {
    dir[felix] = sfile->mkdir(Form("Felix_%d", felix));
    dir[felix]->cd();
    for (int i = 0; i < 14; i++)
    {
      cout << "moudle : " << i << " Type A " << HotChannelCut_A_Fit[felix][i] << endl;
      cout << "moudle : " << i << " Type B " << HotChannelCut_B_Fit[felix][i] << endl;
      for (int j = 1; j < 27; j++)
      {
        cout << "Felix : " << felix << " moudle : " << i << " Type A and chip : " << j << "  " << HotChannelCut_A_Fit[felix][i] << endl;
        for (int chan = 0; chan < 128; chan++)
        {
          // double entry = h1_chip[i][j]->GetBinContent(chan + 1);
          double entry = h2_AllMap[felix][i]->GetBinContent(chan + 1, j + 1);
          felix_ = felix;
          ch_entry = entry;
          module_ = i;
          chip_id_ = j;
          chan_ = chan;
          flag_ = 0;                       
          // flag = 0 : Good channels
          // flag = 1 : Dead channels
          // flag = 2 : Half channels
          // flag = 4 : Cold channels
          // flag = 8 : Hot channels
          if (j < 6 || (j > 13 && j < 19)) // Condition for type B
          {
            type_ = 1;
            mean_gaus = par_meanB[felix][i];
            sigma_gaus = par_sigmaB[felix][i];
          }
          else
          {
            type_ = 0;
            mean_gaus = par_meanA[felix][i];
            sigma_gaus = par_sigmaA[felix][i];
          }
          ///////////////////////////////////////////////////////////
          //                 Half entry chips                      //
          ///////////////////////////////////////////////////////////
          bool is_half = false;
          for (const auto &chip : half_chips)
          {
            if(chip.felix_id_ == felix && chip.module_id_ == i && chip.chip_id_ == j)
            {
              is_half = true;
              break;
          } 
          }
          if(is_half)
          {
            h2_HalfMap[felix][i]->Fill(chan,j);
            flag_ = 2;
          }
          ///////////////////////////////////////////////////////////
          //                 Dead Channel selection                //
          ///////////////////////////////////////////////////////////
          else if (entry == 0)
          {
            h2_DeadMap[felix][i]->Fill(chan, j);
            flag_ = 1;
          }
          else
          {
            ///////////////////////////////////////////////////////////
            //           Hot Cold Channel selection For tpye B       //
            ///////////////////////////////////////////////////////////
            if (j < 6 || (j > 13 && j < 19)) // Condition for type B
            {
              if (entry > HotChannelCut_B_Fit[felix][i])
              {
                h2_HotMap[felix][i]->Fill(chan, j);
                flag_ = 8;
                NumOfHot++;
              }
              if (entry < ColdChannelCut_B_Fit[felix][i])
              {
                h2_ColdMap[felix][i]->Fill(chan, j);
                flag_ = 4;
              }
            }
            else //For tpye A
            {
              ///////////////////////////////////////////////////////////
              //            Hot Cold Channel selection for tpye A      //
              ///////////////////////////////////////////////////////////
              if (entry > HotChannelCut_A_Fit[felix][i])
              {

                h2_HotMap[felix][i]->Fill(chan, j);
                flag_ = 8;
                NumOfHot++;
              }
              if (entry < ColdChannelCut_A_Fit[felix][i])
              {
                h2_ColdMap[felix][i]->Fill(chan, j);
                flag_ = 4;
              }
            }
          }
          st->Fill();
        }
      }
      h2_AllMap[felix][i]->Write();
      h2_DeadMap[felix][i]->Write();
      h2_ColdMap[felix][i]->Write();
      h2_HalfMap[felix][i]->Write();
      h2_HotMap[felix][i]->Write();
      h1_hist_fit_A[felix][i]->Write();
      h1_hist_fit_B[felix][i]->Write();
    }
    sfile->cd();
    canA[felix]->Write();
    canB[felix]->Write();
  }

  st->Write();

  // Add content to the end of the file
  if(Writecsv) csvFile << runnumber << "," << NumOfHot << "\n";

  // Close the file
  csvFile.close();

  sfile->Close();
  file->Close();
}

double SingleGaussianFit(TH1D *hist, double &mean1, double &sigma1)
{ 
  double min_g1 = 0.;
  double max_g1 = 0.;
  double min_hist = 0.;
  GetFirstFilledBinCenter(hist, min_g1, min_hist);
  max_g1 = min_hist;
  TF1 *g1 = new TF1("g1", "gaus", min_g1, max_g1);
  TF1 *g2 = new TF1("g2", "gaus", max_g1, hist->GetMean() + hist->GetMean() * 0.3);
  TF1 *g3 = new TF1("g3", "gaus", 0.09, 0.02);
  cout << min_g1 << " . " << max_g1 << endl;
  // backup
  // TF1* g1 = new TF1("g1", "gaus",0,0.005);
  // TF1* g2 = new TF1("g2", "gaus",0.004,0.015);
  // TF1* g3 = new TF1("g3", "gaus",0.09,0.02);
  //
  double hist_mean = hist->GetMean();
  double hist_std = hist->GetStdDev();
  TF1 *SingleGaussian[7];
  SingleGaussian[0] = new TF1("singleGaussian0", "gaus", hist_mean - 1.3 * hist_std, hist_mean + 1.3 * hist_std);
  SingleGaussian[1] = new TF1("singleGaussian1", "gaus", hist_mean - 2 * 1.3 * hist_std, 1.3 * hist_mean);
  SingleGaussian[2] = new TF1("singleGaussian2", "gaus", hist_mean, hist_mean + 2 * 1.3 * hist_std);
  // SingleGaussian[3] = new TF1("singleGaussian3", "gaus", 0.012, 0.0017);
  // SingleGaussian[4] = new TF1("singleGaussian4", "gaus", 0.010, 0.015);
  // SingleGaussian[5] = new TF1("singleGaussian5", "gaus", 0.006, 0.008);
  // SingleGaussian[6] = new TF1("singleGaussian6", "gaus", 0.04, 0.055);
  TF1 *FirstGaussian = new TF1("First_Gaussian", "gaus", 0.003, 0.03);
  double constant = 0;
  double chi2 = 0;
  double sigma0 = 0;
  double mean0 = 0;
  int ndf = 0;
  double chi2ndf = 0;
  double par[9];
  double sigma_max = 1e-3;
  double sigma_min = 1e-4;
  bool DoMultifit = true;
  hist->Fit(FirstGaussian, "R");
  mean1 = FirstGaussian->GetParameter(1);
  chi2 = FirstGaussian->GetChisquare();
  ndf = FirstGaussian->GetNDF();
  sigma1 = FirstGaussian->GetParameter(2);
  chi2ndf = chi2 / ndf;
  int labbel = -1;
  int flag = -1;
  // if(FirstGaussian->Eval(mean1)<100) DoMultifit = true;
  if (sigma1 < 0.0001)
    DoMultifit = true;
  if (chi2ndf > 100)
    DoMultifit = true;
  // DoMultifit = true;
  if (mean1 < 0)
    DoMultifit = true;
  if (sigma1 > 0.002)
    DoMultifit = true;
  sigma0 = sigma1;
  mean0 = mean1;
  // DoMultifit = true;
  if (FirstGaussian->Eval(hist->GetMean()) < 50)
    DoMultifit = true;
  if (mean1 * 1.3 < hist->GetMean())
    DoMultifit = true;
  double _mean[7] = {0};
  double _constant[7] = {0};
  double _chi2[7] = {0};
  double _chi2ndf[7] = {0};
  double _ndf[7] = {0};
  double _sigma[7] = {0};
  int _flag[7] = {0};
  for (int i = 0; i < 7; i++)
  {
    _constant[i] = -1;
    _mean[i] = -1;
    _chi2[i] = -1;
    _chi2ndf[i] = -1;
    _ndf[i] = -1;
    _sigma[i] = -1;
    _flag[i] = -1;
  }

  if (DoMultifit)
  {
    hist->Fit(SingleGaussian[0], "R");
    hist->Fit(SingleGaussian[1], "R+");
    hist->Fit(SingleGaussian[2], "R+");

    mean1 = 0;
    sigma1 = 0;

    for (int i = 0; i < 3; i++)
    {
      _mean[i] = SingleGaussian[i]->GetParameter(1);
      _constant[i] = SingleGaussian[i]->GetParameter(1);
      _chi2[i] = SingleGaussian[i]->GetChisquare();
      _ndf[i] = SingleGaussian[i]->GetNDF();
      _chi2ndf[i] = _chi2[i] / _ndf[i];
      _sigma[i] = SingleGaussian[i]->GetParameter(2);
      if (_chi2ndf[i] > 100)
      {
        _mean[i] = -1;
        _flag[i] = 0;
      }
      if (_sigma[i] < 0.00001)
      {
        _mean[i] = -1;
        _flag[i] = 1;
      }
      if (_mean[i] > 0.025 || SingleGaussian[i]->Eval(_mean[i]) < 55 || SingleGaussian[i]->Eval(_mean[i]) > 2000)
      {
        _mean[i] = -1;
        _flag[i] = 2;
      }
    }
    mean1 = _mean[0];
    sigma1 = _sigma[0];
    flag = _flag[0];
    for (int i = 1; i < 3; i++)
    {
      if (mean1 < _mean[i] && _sigma[i] != 0)
      {
        mean1 = _mean[i];
        sigma1 = _sigma[i];
        chi2ndf = _chi2ndf[i];
        labbel = i;
      }
    }
  }
  flag = _flag[labbel];
  TText *text = new TText(0.7, 0.7, Form("chi2/ndf: %.1f , %d", chi2ndf, labbel));
  text->SetNDC();
  text->SetTextSize(0.03);
  text->Draw("SAME");

  TText *text2 = new TText(0.7, 0.65, Form("sigma: %.10f", sigma1));
  text2->SetNDC();
  text2->SetTextSize(0.03);
  text2->Draw("SAME");

  TText *text3 = new TText(0.7, 0.6, Form("mean: %.4f", mean1));
  text3->SetNDC();
  text3->SetTextSize(0.03);
  text3->Draw("SAME");

  TText *text4 = new TText(0.7, 0.55, Form("sigma: %.6f", sigma1));
  text4->SetNDC();
  text4->SetTextSize(0.03);
  text4->Draw("SAME");
  /* used for debugging purpose.
    TText *text5 = new TText(0.7, 0.50, Form("%d %d %d %d %d %d %d", _flag[0], _flag[1], _flag[2], _flag[3], _flag[4], _flag[5], _flag[6]));
    text5->SetNDC();
    text5->SetTextSize(0.03);
    text5->Draw("SAME");
    text5->Draw("SAME");
  */
  /////////////////////////////
  //Definiton of Hot Channel //
  /////////////////////////////
  TLine *hotLine = new TLine(mean1 + sigma1 * sig_cut, 0, mean1 + sigma1 * sig_cut, hist->GetMaximum());
  hotLine->SetLineColor(kRed);
  hotLine->SetLineWidth(2);
  hotLine->Draw("SAME");
  /////////////////////////////
  //Definiton of Cold Channel//
  /////////////////////////////
  TLine *coldLine = new TLine(mean1 - sigma1 * sig_cut, 0, mean1 - sigma1 * sig_cut, hist->GetMaximum());
  coldLine->SetLineColor(kBlue);
  coldLine->SetLineWidth(2);
  coldLine->Draw("SAME");
  double cutvalue = mean1 + fabs(sigma1) * sig_cut;
  return cutvalue; //return value : hot channel cut for this ladders
}
void GetFirstFilledBinCenter(TH1D *hist, double &a, double &b)
{
  int numBins = hist->GetNbinsX();
  b = (hist->GetMean());
  for (int bin = 1; bin <= numBins; bin++)
  {
    if (hist->GetBinContent(bin) > 0)
    {
      double xValue = hist->GetXaxis()->GetBinCenter(bin);
      a = xValue;
      return;
    }
  }
}