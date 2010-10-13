// -- STD C++ Include
#include <iostream>
#include <sys/time.h>

// -- BOOST Include
#include <boost/program_options.hpp>

// IDLpp Include
#include <gen/ccpp_latency.h>

#include <dds/dds.hpp>
#include <dds/types.hpp>

REGISTER_TOPIC_TRAITS(Ping);
REGISTER_TOPIC_TRAITS(Pong);

namespace po = boost::program_options;

uint32_t samples = 1000;
uint32_t size = 1;
uint32_t prio = 0;
std::string prefix = "";
uint32_t S = 10;
bool best_effort = false;

bool parse_args(int argc, char* argv[])
{
  po::options_description desc("Available options for <latency-pub> are");
  desc.add_options()
    ("help", "produce help message")
    ("samples", po::value<uint32_t>(), "number of test samples")
    ("size", po::value<uint32_t>(), "data payload size")
    ("prio", po::value<uint32_t>(), "transport priority")
    ("prefix", po::value<std::string>(), "topic name prefix")
    ("best-effort", "sets the communication to best-effort")
    ;
 
  try {
    po::variables_map vm;        
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    
   
    if (vm.count("help") || argc == 1) {
      std::cout << desc << "\n";
      return false;
    }
    
    
    if (vm.count("samples")) 
      samples = vm["samples"].as<uint32_t>();

    if (vm.count("size")) 
      size = vm["size"].as<uint32_t>();

    if (vm.count("prio")) 
      prio = vm["prio"].as<uint32_t>();

    if (vm.count("prefix")) 
      prefix = vm["prefix"].as<std::string>();

    if (vm.count("best-effort"))
      best_effort = true;


  } 
  catch (...) {
    std::cout << desc << "\n";
    return false;
  }
  return true;
}  

class ReadConditionHandler {
public:
  void operator()(dds::DataReader<Pong>& dr) { }
};

int main(int argc, char* argv[]) {
  if (!parse_args(argc, argv)) {
    return 1;
  }
  dds::Runtime runtime("");
  dds::Topic<Ping> pingT(prefix + "Ping");
  dds::Topic<Pong> pongT(prefix + "Pong");

  dds::DataWriterQos dwqos;

  if (best_effort == false) 
    dwqos.set_reliable();

  dwqos.set_priority(prio);
  dds::DataWriter<Ping> writer(pingT, dwqos);
  
  dds::DataReaderQos drqos;

  if (best_effort == false)
    drqos.set_reliable();

  dds::DataReader<Pong> reader(pongT, drqos);
  ReadConditionHandler rch;
  dds::ReadCondition rc = reader.create_readcondition(rch);
  dds::WaitSet ws;
  ws.attach(rc);
  
  std::string str;
  for (uint32_t i = 0; i < size; ++i) 
    str += "x";
  
  Ping msg;
  msg.data = DDS::string_dup(str.c_str());
  timeval* start = new timeval[samples];
  timeval* stop = new timeval[samples];
  PongSeq data(S);
  dds::SampleInfoSeq info(S);
  DDS::ConditionSeq cseq(S);
  for (uint32_t i = 0; i < samples; ++i) {
    msg.id = i;
    gettimeofday(&start[i], 0);
    writer.write(msg);
    ws.wait(cseq);
    reader.take(data, info);
    gettimeofday(&stop[i], 0);    reader.read(data, info);

  }
  
  for (uint32_t i = 0; i < samples; ++i) {
    
    uint64_t delta =
      (stop[i].tv_sec - start[i].tv_sec)*1000000 + 
      (stop[i].tv_usec - start[i].tv_usec);
    std::cout << delta << std::endl;
  }
  return 0;
}
