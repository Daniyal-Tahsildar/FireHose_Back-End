#include<fstream>
#include<iostream>
#include<vector>
#include<string>
#include<cmath>
#include<unistd.h>

int main() {



    std::ifstream check_start("results.txt");
    std::string file_status;

    int waiting = 1;
    std::cout << "Waiting on Back-End...\n";

    while(waiting) {

        std::getline(check_start, file_status);

        if (file_status == "F") {
            check_start.clear();
            check_start.seekg(0);
        }
        else {
            waiting = 0;
        }
        sleep(2);

        std::cout << "Waiting on Back-End...\n";
    }

    check_start.close();

    std::vector<float> results;
    std::vector<float> anomalies;

    std::ifstream results_strm("results.txt");

    std::string buf;
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

    std::cout << "Results\n";
    int root = std::sqrt(results.size());
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
}