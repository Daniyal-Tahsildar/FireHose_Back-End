// Copyright (c) 2020 Graphcore Ltd. All rights reserved.
#include <iostream>
#include "ipu_analytic.hpp"
#include "utils.h"

int main(int argc, char **argv) {

  //auto p_options = utils::parseOptions(argc, argv);
  int device = -1; // 0 = IPU
  //std::cin >> device;

  std::ifstream fh_settings("settings.txt");
  std::string settings_buf;

  std::getline(fh_settings, settings_buf);
  device = stoi(settings_buf);

  switch(device) {
    case -1:
      std::cout << "No device selected, ending program" << std::endl;
      return 0;
      break;
    case 0:
      std::cout << "No device selected, ending program" << std::endl;
      return 0;
      break;
    default:
      std::cout << "Device Selected: IPU selected" << std::endl;
  }

  std::cout << std::endl;

  int consumption_task = -1;
  //std::cout << "What consumption task would you like to do on the back-end?" << std::endl;
  //std::cout << "1. Matrix multiplication" << std::endl;
  //std::cout << "2. Graph task" << std::endl;
  //std::cout << "3. Hashing" << std::endl;
  //std::cout << "Consumption Task: ";
  //std::cin >> consumption_task;
  std::getline(fh_settings, settings_buf);
  consumption_task = stoi(settings_buf);

  switch(consumption_task) {
    case -1:
      std::cout << "No consumption task selected, ending program" << std::endl;
      return 0;
      break;
    case 0:
      std::cout << "No consumption task selected, ending program" << std::endl;
      return 0;
      break;
    default:
      std::cout << "Consumption Task: Matrix multiplication selected" << std::endl;
  }
  
  std::cout << std::endl;

  if (consumption_task) {
    int source = -1;
    //std::cout << "Where should we source the data?" << std::endl;
    //std::cout << "1. Random Generation" << std::endl;
    //std::cout << "2. From file" << std::endl;
    std::cout << "Choice of Source: ";
    //std::cin >> source;
    std::getline(fh_settings, settings_buf);
    source = stoi(settings_buf);

    switch(source) {
      case -1:
        std::cout << "No source selected, ending program" << std::endl;
        return 0;
        break;
      case 0:
        std::cout << "No source selected, ending program" << std::endl;
        return 0;
        break;
      default:
        std::cout << "Random Generation selected" << std::endl;
    }
  }

  std::cout << std::endl;

  long unsigned int matrix_dim = 0;
  //std::cout << "What dimensions would you like for your square matrix? (NxN)" << std::endl;
  //std::cout << "Enter N: ";
  std::getline(fh_settings, settings_buf);
  matrix_dim = stoi(settings_buf);
  //std::cin >> matrix_dim;
  //std::cout << std::endl;

  std::cout << "Matrix Dimension Selected: " << matrix_dim << "\n";

  fh_settings.close();

  launchOnIPU(matrix_dim, argc, argv);
  
  //uint32_t test_seeds[4] = { 3755779729, 545041952, 2371063071, 3195806153}; //Random test values for demo
  //launchOnIPU_IPU_IPU(matrix_dim, argc, argv, test_seeds);

  return EXIT_SUCCESS;
}