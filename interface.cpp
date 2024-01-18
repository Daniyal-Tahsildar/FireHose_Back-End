#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include<vector>
#include<string>
#include<cmath>
#include<unistd.h>

int main() {

    int device = -1;
    std::cout << "Welcome to the FireHose Generator. What device are you targeting today?" << std::endl;
    std::cout << "1. Graphcore IPU" << std::endl;
    std::cout << "2. UPMEM DPU" << std::endl;
    std::cin >> device;

    int consumption_task = -1;
    std::cout << "What consumption task would you like to do on the back-end?" << std::endl;
    std::cout << "1. Matrix multiplication" << std::endl;
    std::cout << "2. Graph task" << std::endl;
    std::cout << "3. Hashing" << std::endl;
    std::cin >> consumption_task;

    int source = -1;
    std::cout << "Where should we source the data?" << std::endl;
    std::cout << "1. Random Generation" << std::endl;
    std::cout << "2. From file" << std::endl;
    std::cin >> source;

    long unsigned int matrix_dim = 0;
    std::cout << "What dimensions would you like for your square matrix? (NxN)" << std::endl;
    std::cin >> matrix_dim;

    std::ofstream myFile("gen.batch");

    std::string input = "#!/bin/bash\n#SBATCH --job-name FireHose_Generator\n#SBATCH --ipus=1\n--partition=p64\n#SBATCH --nodelist=gc-poplar-03\n#SBATCH --ntasks 1\n#SBATCH --time=00:05:00\n\nsrun ./gen_demo --device " + std::to_string(device) + " --con_task " + std::to_string(consumption_task) + " --source " + std::to_string(source) + " --dimension " + std::to_string(matrix_dim);

    myFile << input;

    myFile.close();

    //system("sbatch gen.batch");

    std::string device_setting =  std::to_string(device);
    std::string task_setting = std::to_string(consumption_task);
    std::string src_setting = std::to_string(source);
    std::string dim_setting = std::to_string(matrix_dim);

    //system(placeholder.c_str());

    std::ofstream settings_strm ("settings.txt");

    settings_strm << device_setting;
    settings_strm << "\n";

    settings_strm << task_setting;
    settings_strm << "\n";

    settings_strm << src_setting;
    settings_strm << "\n";

    settings_strm << dim_setting;
    settings_strm << "\n";

    settings_strm.close();

    int waiting = 1;
    std::cout << "Waiting on Back-End...\n";

    std::ifstream check_start("results.txt");
    std::string file_status;

    while(waiting) {

        std::getline(check_start, file_status);

        if (file_status == "F") {
            check_start.clear();
            check_start.seekg(0);
        }
        else {
            waiting = 0;
        }
        sleep(5);

        std::cout << "Waiting on Back-End...\n";
    }

    check_start.close();

    std::vector<float> multiplicand;
    std::vector<float> multiplier;
    std::vector<float> results;
    std::vector<float> anomalies;
    std::string buf;


    std::ifstream multnd_strm("multiplicand.txt");
    while(multnd_strm.good()) {
        std::getline(multnd_strm, buf);

        switch(buf.size()) {
            case 0:
                break;
            default:
                multiplicand.push_back(std::stof(buf));
                break;
        }
    }

    std::ifstream multr_strm("multiplier.txt");
    while(multr_strm.good()) {
        std::getline(multr_strm, buf);

        switch(buf.size()) {
            case 0:
                break;
            default:
                multiplier.push_back(std::stof(buf));
                break;
        }
    }


    std::ifstream results_strm("results.txt");
    while(results_strm.good()) {
        std::getline(results_strm, buf);

        switch(buf.size()) {
            case 0:
                break;
            default:
                results.push_back(std::stof(buf));
                break;
        }
    }

    results_strm.close();

    std::ifstream anom_strm("anomalies.txt");

    while(anom_strm.good()) {
        std::getline(anom_strm, buf);

        switch(buf.size()) {
            case 0:
                break;
            default:
                anomalies.push_back(std::stof(buf));
                break;
        }
    }

    anom_strm.close();

    int root;

    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Multiplicand\n";
    root = std::sqrt(multiplicand.size());
    for(int i = 0; i < multiplicand.size(); i++) {
        std::cout << std::fixed << multiplicand[i] << "\t";
        if ( (i+1)%root == 0) {
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Multiplier\n";
    root = std::sqrt(multiplier.size());
    for(int i = 0; i < multiplier.size(); i++) {
        std::cout << std::fixed << multiplier[i] << "\t";
        if ( (i+1)%root == 0) {
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Results\n";
    root = std::sqrt(results.size());
    for(int i = 0; i < results.size(); i++) {
        std::cout << std::fixed << results[i] << "\t";
        if ( (i+1)%root == 0) {
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Anomalies\n";
    root = std::sqrt(anomalies.size());
    for(int i = 0; i < anomalies.size(); i++) {
        std::cout << std::fixed << anomalies[i] << "\t";
        if ( (i+1)%root == 0) {
            std::cout << std::endl;
        }
    }

    std::ofstream results_write ("results.txt");

    results_write << "F";

    results_write.close();

    std::ofstream anom_write ("anomalies.txt");

    anom_write << "F";

    anom_write.close();
    
    return 0;
}