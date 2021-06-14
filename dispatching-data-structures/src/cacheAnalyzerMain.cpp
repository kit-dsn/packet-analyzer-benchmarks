#include <iostream>
#include "dispatchers/All.h"
#include "InputReader.h"
#include "papi.h"

#define PMU_EVENT_COUNT 3

#define PAPI_error_check(x) if (x < PAPI_OK) throw std::runtime_error("Error in PAPI call: " + std::to_string(x))
#define runAnalysis(dispatcher, packets, analyzerBuilders) measure(#dispatcher, std::make_unique<dispatcher>(), packets, analyzerBuilders)

int pmu_events[PMU_EVENT_COUNT] = {
	PAPI_L1_DCM,
	PAPI_L2_DCM,
	PAPI_L3_TCM
};

void measure(
	const std::string& name,
	std::unique_ptr<IDispatcher>&& dispatcher,
	const std::vector<MyPacket>& packets,
	const std::map<identifier_t, analyzer_builder>& analyzerBuilders
) {
	dispatcher->registerAnalyzers(analyzerBuilders);

	long_long results[PMU_EVENT_COUNT];
	PAPI_error_check(PAPI_start_counters(pmu_events, PMU_EVENT_COUNT));
	for (const auto &packet : packets) {
		for (const auto &identifier : packet.getIdentifiers()) {
			dispatcher->lookup(identifier);
		}
	}
	PAPI_error_check(PAPI_stop_counters(results, PMU_EVENT_COUNT));

	// Print results
	std::cout << name;
	for (const auto& current : results) {
		std::cout << "," << current;
	}
	std::cout << std::endl;

	dispatcher->clear();
}

int main(int argc, char** argv) {
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
		exit(1);
	}

    if (argc < 2) {
        std::cerr << "Path to packet file missing." << std::endl;
        return 1;
    } else if (argc < 3) {
    	// Check if all papi counters exist.
    	if (std::string(argv[1]) == "papi_check") {
    		for (int pmu_event : pmu_events) {
				PAPI_event_info_t info;
				PAPI_get_event_info(pmu_event, &info);
				// Counter is available if "count" is not 0 (see papi_avail.c:524)
				if (info.count == 0) {
					exit(2);
				}
    		}

			exit(0);
    	}

        std::cerr << "Path to analyzer file missing." << std::endl;
    }

    std::vector<MyPacket> packets;
    try {
        packets = InputReader::readPacketFile(argv[1]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Error reading packet file: " << e.what() << std::endl;
        return 1;
    }

    std::map<identifier_t, analyzer_builder> analyzerBuilders;
    try {
        analyzerBuilders = InputReader::readAnalyzerFile(argv[2]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Error reading analyzer file: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "name,l1_data_misses,l2_data_misses,l3_total_misses" << std::endl;

    runAnalysis(Array, packets, analyzerBuilders);
    runAnalysis(Vector, packets, analyzerBuilders);
    runAnalysis(TreeMap, packets, analyzerBuilders);
    runAnalysis(UnorderedMap, packets, analyzerBuilders);
    runAnalysis(Cuckoo, packets, analyzerBuilders);
    runAnalysis(Hanov, packets, analyzerBuilders);
    runAnalysis(Universal, packets, analyzerBuilders);
    runAnalysis(SparseUpper, packets, analyzerBuilders);

    // Fragmented tests
    if (std::string(argv[2]).find("fragmented") != std::string::npos) {
        runAnalysis(GeneratedSwitchFragmented, packets, analyzerBuilders);
        runAnalysis(GeneratedIfFragmented, packets, analyzerBuilders);
    }

    // Zeek default mapping tests
    if (std::string(argv[2]).find("zeek") != std::string::npos) {
        runAnalysis(GeneratedSwitchZeek, packets, analyzerBuilders);
        runAnalysis(GeneratedIfZeek, packets, analyzerBuilders);
    }
}
