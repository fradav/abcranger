#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "readstatobs.hpp"
#include "readreftable.hpp"
#include "csv-eigen.hpp"
#include "cxxopts.hpp"
#include "EstimParam.hpp"
#include "ks.hpp"

#include "range/v3/all.hpp"

using namespace ranges;

// "expectation","variance","quant0.05","quant0.5","quant0.95","MSE","NMSE","NMAE","mean CI","mean relative CI","median CI","median relative CI"

TEST_CASE("EstimParam KS distribution")
{   
    size_t nref;
    std::string headerfile,reftablefile,statobsfile;
    MatrixXd E = read_matrix_file("estimparam_runs.csv",',');

    std::vector<double> expectationsR = E.col(0) | to_vector;
    std::vector<double> variancesR    = E.col(1) | to_vector;
    std::vector<double> quantiles1R   = E.col(2) | to_vector;
    std::vector<double> quantiles2R   = E.col(3) | to_vector;
    std::vector<double> quantiles3R   = E.col(4) | to_vector;

    try {
        std::vector<std::string> args{"EstimParam","--ntree","1000","--parameter","ra","--ntrain","1000","--ntest","50","--chosenscen","1","--nopls" };
        std::vector<char *> argv;
        for(std::string &s: args) argv.push_back(&s[0]);
        argv.push_back(NULL);

        cxxopts::Options options("", "");

        options
            .positional_help("[optional args]")
            .show_positional_help();

        options.add_options()
            ("h,header","Header file",cxxopts::value<std::string>()->default_value("headerRF.txt"))
            ("r,reftable","Reftable file",cxxopts::value<std::string>()->default_value("reftableRF.bin"))
            ("b,statobs","Statobs file",cxxopts::value<std::string>()->default_value("statobsRF.txt"))
            ("o,output","Prefix output",cxxopts::value<std::string>()->default_value("estimparam_out"))
            ("n,nref","Number of samples, 0 means all",cxxopts::value<size_t>()->default_value("0"))
            ("m,minnodesize","Minimal node size. 0 means 1 for classification or 5 for regression",cxxopts::value<size_t>()->default_value("0"))
            ("t,ntree","Number of trees",cxxopts::value<size_t>()->default_value("500"))
            ("j,threads","Number of threads, 0 means all",cxxopts::value<size_t>()->default_value("0"))
            ("s,seed","Seed, generated by default",cxxopts::value<size_t>()->default_value("0"))
            ("c,noisecolumns","Number of noise columns",cxxopts::value<size_t>()->default_value("5"))
            ("nopls","Disable PLS")
            ("chosenscen","Chosen scenario (mandatory)", cxxopts::value<size_t>())
            ("ntrain","number of training samples (mandatory)",cxxopts::value<size_t>())
            ("ntest","number of testing samples (mandatory)",cxxopts::value<size_t>())
            ("parameter","name of the parameter of interest (mandatory)",cxxopts::value<std::string>())
            ("help", "Print help")
            ;
        int argc = argv.size()-1;
        char ** argvc = argv.data();
        auto opts = options.parse(argc,argvc);

        if (opts.count("help")) {
          std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        size_t nrun = 100;
        std::vector<double> expectations(nrun),
            variances(nrun),
            quantiles1(nrun),
            quantiles2(nrun),
            quantiles3(nrun);

        headerfile = opts["h"].as<std::string>();
        reftablefile = opts["r"].as<std::string>();
        statobsfile = opts["b"].as<std::string>();
        auto myread = readreftable(headerfile, reftablefile, nref, true);
        const auto statobs = readStatObs(statobsfile);

        for(auto i = 0; i < nrun; i++) {

            auto res = EstimParam_fun(myread,statobs,opts,true);
            std::cout << i << "...";
            std::cout.flush();
            expectations[i] = res.expectation;
            variances[i] = res.variance;
            quantiles1[i] = res.quantiles[0];
            quantiles2[i] = res.quantiles[1];
            quantiles3[i] = res.quantiles[2];
        }
        std::cout << std::endl;

        double D,pvalue1,pvalue2,pvalue3,pvalue4,pvalue5;

        std::cout << "expectations" << std::endl;
        std::cout << (expectations | view::all) << std::endl;

        std::cout << "variances" << std::endl;
        std::cout << (variances | view::all) << std::endl;

        std::cout << "quantiles1" << std::endl;
        std::cout << (quantiles1 | view::all) << std::endl;

        std::cout << "quantiles2" << std::endl;
        std::cout << (quantiles2 | view::all) << std::endl;

        std::cout << "quantiles3" << std::endl;
        std::cout << (quantiles3 | view::all) << std::endl;

        D = KSTest(expectationsR,expectations);
        pvalue1 = 1-psmirnov2x(D,E.rows(),nrun);
        CHECK( pvalue1 >= 0.05 );

        D = KSTest(variancesR,variances);
        pvalue2 = 1-psmirnov2x(D,E.rows(),nrun);
        CHECK( pvalue2 >= 0.05 );

        D = KSTest(quantiles1R,quantiles1);
        pvalue3 = 1-psmirnov2x(D,E.rows(),nrun);
        CHECK( pvalue3 >= 0.05 );

        D = KSTest(quantiles2R,quantiles2);
        pvalue4 = 1-psmirnov2x(D,E.rows(),nrun);
        CHECK( pvalue4 >= 0.05 );

        D = KSTest(quantiles3R,quantiles3);
        pvalue5 = 1-psmirnov2x(D,E.rows(),nrun);
        CHECK( pvalue5 >= 0.05 );
    } catch (const cxxopts::OptionException& e)
      {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    } 

}