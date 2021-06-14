# Performance Evaluation

This directory contains the experiments conducted in the context of the work on "Advancing Protocol Diversity in Network Security Monitoring". The experiments are described in section 5 of the paper. To reproduce the results, follow the corresponding instructions.


## Dispatching Data Structures  (5.1)

This subdirectory contains the C++ benchmark application. To build the application, use the following commands:

	# cd dispatching-data-structures
	# ./scripts/setup.sh

This will generate the data for testing, generate the corresponding code (used for comparison with the hard-coded dispatching), and build the benchmark applications. The required dependencies are fetched by cmake.
**Note:** Unfortunately, the employed cuckoo table implementation needs to be built using bash. If building fails, please switch to bash and retry.

To execute the measurements and create the plots, run the following command:

	# ./scripts/regenerate_paper_plots.sh

In its default configuration, this script will execute the benchmarks, collect the results in the `results` subdirectory, and generate the plots.


## Monitor Performance (5.2)

This subdirectory contains scripts to benchmark the implementation of the modular packet analysis framework in Zeek. First, install the dependencies that are required to build Zeek. Please refer to https://docs.zeek.org/en/current/install.html for detailed instructions (note that we use the optional jemalloc library). When the dependencies are available, use the following to build the two Zeek versions for comparison:

	# ./setup.sh

This will build both versions as well as the cset tool used for isolation of the benchmarks.

To execute the measurements for a given PCAP file, run the following command (note that isolation using cset requires root permissions):

	# sudo ./benchmark.sh <PCAP_FILE>

By default, the script will print out the results based on ten runs. The CIC-IDS17 dataset that was used for the evaluation in the paper can be obtained at https://www.unb.ca/cic/datasets/ids-2017.html.
