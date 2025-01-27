 
// Copyright (c) 2020 Graphcore Ltd. All rights reserved.
#include "utils.h"

using namespace utils;
// gen.batch --devices help --con_task 1
Options utils::parseOptions(int argc, char **argv) {
  Options options;
  std::string modeString;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()("help", "Show command help.")( 
      "device", po::value(&options.devices)->default_value(1),
      "Select hardware to use")( 
      "con_task", po::value(&options.con_task)->default_value(1),
      "Select Consumption task to use")( 
      "source", po::value(&options.source)->default_value(1),
      "Select source")( 
      "dimension", po::value(&options.dimensions)->default_value(5),
      "Select matrix dimensions")(
      "model", po::bool_switch(&options.useIpuModel)->default_value(false),
      "If set then use IPU model instead of hardware.")(
      "ipus", po::value<std::size_t>(&options.numIpus)->default_value(1),
      "Number of IPUs to use.")(
      "exe-name", po::value<std::string>(&options.exeName)->default_value(""),
      "Save the graph executable under a file name with this prefix. "
      "This option is required when loading/saving executables.")(
      "save-exe", po::bool_switch(&options.saveExe)->default_value(false),
      "Save the Poplar graph executable after compilation. "
      "You must also set 'exe-name'.")(
      "load-exe", po::bool_switch(&options.loadExe)->default_value(false),
      "Load a previously saved executable and skip graph and program "
      "construction. "
      "You must also set 'exe-name'.")(
      "profile", po::bool_switch(&options.profile)->default_value(false),
      "Enable profile output.")("profile-name",
                                po::value<std::string>(&options.profileName)
                                    ->default_value("profile.txt"),
                                "Name of profile output file.");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    throw std::runtime_error("Show help");
  }

  po::notify(vm);

  // Check options are set correctly:
  if ((options.saveExe || options.loadExe) && options.exeName.empty()) {
    throw std::logic_error(
        "To save/load an executable you must set 'exe-name'");
  }

  if (options.loadExe && options.profile) {
    throw std::logic_error(
        "It is not possible to run profiling on a loaded executable.");
  }

  return options;
}

std::string utils::getExeFileName(const Options &options) {
  return options.exeName + ".poplar";
}

poplar::Executable
utils::compileOrLoadExe(poplar::Graph &graph,
                 const std::vector<poplar::program::Program> &progs,
                 const Options &options) {
  if (options.loadExe) {
    const auto exeName = getExeFileName(options);
    try {
      auto inf = std::ifstream(exeName);
      return poplar::Executable::deserialize(inf);
    } catch (const poplar::poplar_error &e) {
      std::cerr << "Error: Failed to load executable '" << exeName << "'\n";
      throw;
    }
  } else {
    return poplar::compileGraph(graph, progs);
  }
}

// Return a HW device with the requested number of IPUs.
// Exception is thrown if no devices with the requested
// number are available.
poplar::Device utils::getIpuHwDevice(std::size_t numIpus) {
  auto dm = poplar::DeviceManager::createDeviceManager();
  auto hwDevices = dm.getDevices(poplar::TargetType::IPU, numIpus);
  auto it =
      std::find_if(hwDevices.begin(), hwDevices.end(),
                   [](poplar::Device &device) { return device.attach(); });

  if (it != hwDevices.end()) {
    return std::move(*it);
  }

  throw std::runtime_error("No IPU hardware available.");
}

// Return an IPU Model device with the requested number of IPUs.
poplar::Device utils::getIpuModelDevice(std::size_t numIpus) {
  poplar::IPUModel ipuModel;
  ipuModel.numIPUs = numIpus;
  return ipuModel.createDevice();
}

poplar::Device utils::getDeviceFromOptions(const Options &options) {
  poplar::Device device;
  if (options.useIpuModel) {
    device = getIpuModelDevice(options.numIpus);
    std::cerr << "Using IPU model\n";
  } else {
    device = getIpuHwDevice(options.numIpus);
    std::cerr << "Using HW device ID: " << device.getId() << "\n";
  }
  return device;
}