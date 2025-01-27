#include "ipu_analytic.hpp"

enum Progs {
  WRITE_INPUTS,
  CONSUMPTION_TASK,
  READ_RESULTS,
  CHECK_ANOMALY,
  READ_ANOMALY,
  NUM_PROGRAMS
};

//1. Consumption Matrix
//2. Output matrix
class GraphTensors {
  
    private:
        int num_tensors;

        std::vector<poplar::Tensor> tensors;
        std::vector< std::tuple<unsigned int, unsigned int> > dimensions;
        std::vector<poplar::Type> tensor_types;
        std::vector<std::string> tensor_dbs;
  
    public:
        GraphTensors (poplar::Graph &g, int matrix_dim) {
    
            this->num_tensors = 4;

            this->tensor_dbs.push_back("Streamed Matrix (Multiplicand)");
            //Note that this stream should belong to the back-end
            this->tensor_dbs.push_back("Consumption Matrix (Multiplier)");
            this->tensor_dbs.push_back("Output Matrix (Result)");
            this->tensor_dbs.push_back("Anomaly");

            for (int i = 0; i < num_tensors; i++) {
                this->dimensions.push_back(std::tuple<int,int>{matrix_dim,matrix_dim});
                this->tensor_types.push_back(poplar::FLOAT);
        
                this->tensors.push_back( g.addVariable( 
                    this->tensor_types[i],
                    {std::get<0>(this->dimensions[i]), std::get<1>(this->dimensions[i])},
                    this->tensor_dbs[i] ) );
                poputil::mapTensorLinearly(g, tensors[i]);
            }
        }

        void addTensor (poplar::Graph &g, std::string tensor_db, poplar::Type tensor_type, long unsigned int dim1, long unsigned int dim2) {
            this->num_tensors++;

            this->tensor_dbs.push_back(tensor_db);

            this->tensors.push_back( g.addVariable( 
                    tensor_type,
                    {dim1, dim2},
                    tensor_db ) );
        }

        poplar::Tensor getTensor(int index) {
            return this->tensors[index];
        }

};

class GraphStreams {

  private:
    int num_streams;

    std::vector<poplar::DataStream> strms;
    std::vector<std::string> strm_srcs;
    std::vector<std::string> strm_dests;
    std::vector<std::string> strm_dbs;
    std::vector<poplar::Type> strm_types;
    std::vector<int> strm_lengths;
    std::vector<bool> strm_dirs; // 0 = CPU to IPU, 1 = IPU to CPU

  public:
    GraphStreams(poplar::Graph &g, int matrix_dim) {
      this->num_streams = 4;

      //Note that this stream should belong to the back-end
      this->strm_dbs.push_back("Consumption_Stream");
      this->strm_srcs.push_back("CPU");
      this->strm_srcs.push_back("IPU");
      this->strm_lengths.push_back(matrix_dim*matrix_dim);
      this->strm_types.push_back(poplar::FLOAT);
      this->strm_dirs.push_back(0);
      this->strms.push_back( g.addHostToDeviceFIFO(strm_dbs[0], strm_types[0], strm_lengths[0]) );

      this->strm_dbs.push_back("Source_Stream");
      this->strm_srcs.push_back("CPU");
      this->strm_dests.push_back("IPU");
      this->strm_lengths.push_back(matrix_dim*matrix_dim);
      this->strm_types.push_back(poplar::FLOAT);
      this->strm_dirs.push_back(0);
      this->strms.push_back( g.addHostToDeviceFIFO(strm_dbs[1], strm_types[1], strm_lengths[1]) );

      this->strm_dbs.push_back("Write_Result_Stream");
      this->strm_srcs.push_back("CPU");
      this->strm_dests.push_back("IPU");
      this->strm_lengths.push_back(matrix_dim*matrix_dim);
      this->strm_types.push_back(poplar::FLOAT);
      this->strm_dirs.push_back(0);
      this->strms.push_back( g.addDeviceToHostFIFO(strm_dbs[2], strm_types[2], strm_lengths[2]) );

      this->strm_dbs.push_back("Anomaly_Stream");
      this->strm_srcs.push_back("CPU");
      this->strm_dests.push_back("IPU");
      this->strm_lengths.push_back(matrix_dim*matrix_dim);
      this->strm_types.push_back(poplar::FLOAT);
      this->strm_dirs.push_back(0);
      this->strms.push_back( g.addDeviceToHostFIFO(strm_dbs[3], strm_types[3], strm_lengths[3]) );


    }

    void addHostToDeviceStream(poplar::Graph &g, std::string strm_db, int strm_length, poplar::Type strm_type ) {
        this->num_streams++;

        this->strm_dests.push_back("IPU");
        this->strm_lengths.push_back(strm_length);
        this->strm_types.push_back(strm_type);
        this->strm_dirs.push_back(0);
        this->strms.push_back( g.addHostToDeviceFIFO(strm_db, strm_type, strm_length) );
    }

    poplar::DataStream getStream(int index) {
        return this->strms[index];
    }

};

void printMatrix(std::string matrix_name, std::vector<float> matrix, int matrix_dim) {
  std::cout << matrix_name << std::endl;

  for (int i = 0; i < matrix.size(); i++) {

    std::cout << std::fixed << matrix[i] << "\t";
    
    if ( (i+1)%matrix_dim == 0) {
      std::cout << std::endl;
    }

  }

  std::cout << std::endl;

}



std::vector<poplar::program::Program> buildPrograms(poplar::Graph &g, const utils::Options &options, GraphTensors &gTensors, GraphStreams &gStreams, int matrix_dim) {
  
  // Now can start constructing the programs. Construct a vector of
  // separate programs that can be called individually:
  std::vector<poplar::program::Program> progs(Progs::NUM_PROGRAMS);

  // Program that executes all the reduction compute sets:
  auto seq = poplar::program::Sequence();

  // Add a second compute set that will perform the same calculation using
  poplin::addCodelets(g);
  auto mult_out = poplin::matMul(g, gTensors.getTensor(0), gTensors.getTensor(1), seq, poplar::FLOAT);
  seq.add(poplar::program::Copy(mult_out,gTensors.getTensor(2)));
  //Connect to codely that does anomaly detection
  auto anomalyCS = g.addComputeSet("anomalyCS");
  auto num = matrix_dim;

  for (unsigned i = 0; i < num; ++i){
    auto v = g.addVertex(anomalyCS, "AnomalyVertex", {{"mult_out", mult_out[i]}, {"result",gTensors.getTensor(3)[i]}});
    g.setTileMapping(v, i);
  }

  progs[CHECK_ANOMALY] = poplar::program::Execute(anomalyCS);

  progs[CONSUMPTION_TASK] = seq;

  // Add program which initialises the inputs. Poplar is able to merge these
  // copies for efficiency:
  progs[WRITE_INPUTS] =
      poplar::program::Sequence({poplar::program::Copy(gStreams.getStream(0), gTensors.getTensor(0)), poplar::program::Copy(gStreams.getStream(1), gTensors.getTensor(1))});

  // Add a program to read back the result and anomalies:
  progs[READ_RESULTS] = poplar::program::Copy(gTensors.getTensor(2), gStreams.getStream(2));
 progs[READ_ANOMALY] = poplar::program::Copy(gTensors.getTensor(3), gStreams.getStream(3));

  return progs;

}

  void executeIPUCode(poplar::Device &device, poplar::Executable &exe, std::vector<float> &multiplicand, std::vector<float> &multiplier, std::vector<float> &output_init, std::vector<float> &anomalies) {
  poplar::Engine engine(std::move(exe));
  engine.load(device);

  engine.connectStream("Source_Stream", multiplicand.data());
  engine.connectStream("Consumption_Stream", multiplier.data());
  engine.connectStream("Write_Result_Stream", output_init.data());
  engine.connectStream("Anomaly_Stream", anomalies.data());

  engine.run(WRITE_INPUTS);
  engine.run(CONSUMPTION_TASK);
  engine.run(READ_RESULTS);
  engine.run(CHECK_ANOMALY);
  engine.run(READ_ANOMALY);

}
  

void launchOnIPU(long unsigned int dim, int argc, char **argv) {
    try {
         auto options = utils::parseOptions(argc, argv);
         auto device = utils::getDeviceFromOptions(options);
        poplar::Graph graph(device.getTarget());
        graph.addCodelets("anomaly_codelet.cpp");

        // If we are loading the graph program we do not need
        // to construct it (which can be time consuming
        // for large programs):
        std::vector<poplar::program::Program> progs;
        GraphTensors gTensors(graph, dim);
        GraphStreams gStreams(graph, dim);
        if (!options.loadExe) {
            
            progs = buildPrograms(graph, options, gTensors, gStreams, dim);
        }

        auto exe = utils::compileOrLoadExe(graph, progs, options);

        if (options.saveExe && !options.loadExe) {
        auto outf = std::ofstream(utils::getExeFileName(options));
        exe.serialize(outf);
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distribution(0.0f, 100.0f);
        std::vector<float> multiplicand(dim*dim);
        std::vector<float> multiplier(dim*dim);
        std::vector<float> output_init(dim*dim);
        std::vector<float> anomalies(dim*dim);

//get multiplicand from the gen file

         multiplicand = mult_matrix(dim);
         gTensors.addTensor(graph, "multiplicand", poplar::FLOAT, dim,dim); 


        for (int i = 0; i < dim*dim; i++) {
            multiplier[i] = distribution(gen);
            output_init[i] = -1.0f;
        }

        //printMatrix("Multiplicand", multiplicand, dim);
        //printMatrix("Multiplier", multiplier, dim);

        executeIPUCode(device, exe, multiplicand, multiplier, output_init, anomalies);

        //printMatrix("Result", output_init , dim);

        //printMatrix("Anomaly", anomalies, dim);

        std::ofstream mltnd_strm ("multiplicand.txt");

        for(int i = 0; i < multiplicand.size(); i++)
        {
          mltnd_strm << multiplicand[i];
          mltnd_strm << "\n";
        }

        mltnd_strm.close();

        std::ofstream mltr_strm ("multiplier.txt");

        for(int i = 0; i < multiplier.size(); i++)
        {
          mltr_strm << multiplier[i];
          mltr_strm << "\n";
        }

        mltr_strm.close();

        std::ofstream results_strm ("results.txt");

        for(int i = 0; i < output_init.size(); i++)
        {
          results_strm << output_init[i];
          results_strm << "\n";
        }

        results_strm.close();

        std::ofstream anom_strm ("anomalies.txt");

        for(int i = 0; i < anomalies.size(); i++)
        {
          anom_strm << anomalies[i];
          anom_strm << "\n";
        }

        anom_strm.close();

    } catch (const std::exception &e) {
         std::cerr << "Exception: " << e.what() << "\n";
    }
}