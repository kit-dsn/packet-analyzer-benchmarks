#include <benchmark/benchmark.h>
#include <iostream>

#include "dispatchers/All.h"
#include "InputReader.h"

#define ITERATIONS 1
benchmark::TimeUnit timeunit = benchmark::kMillisecond;

#define registerBenchmark(dispatcher, test, packets, analyzerBuilders, repetitionCount) \
    benchmark::RegisterBenchmark(#dispatcher, test, std::make_shared<dispatcher>(), packets, analyzerBuilders) \
    ->Unit(timeunit) \
    ->Iterations(ITERATIONS) \
    ->Repetitions(repetitionCount)

// Need to use a shared_ptr because RegisterBenchmark internally creates a lambda with a copy capture.
void BM_startup(
		benchmark::State &state,
		std::shared_ptr<IDispatcher> &&dispatcher,
		const std::vector<MyPacket> &packets,
		const std::map<identifier_t, analyzer_builder> &analyzerBuilders
) {
	for (auto _ : state) {
		dispatcher->registerAnalyzers(analyzerBuilders);
		dispatcher->clear();
	}
}

// Need to use a shared_ptr because RegisterBenchmark internally creates a lambda with a copy capture.
void BM_dispatchers(
	benchmark::State &state,
	std::shared_ptr<IDispatcher> &&dispatcher,
	const std::vector<MyPacket> &packets,
	const std::map<identifier_t, analyzer_builder> &analyzerBuilders
) {
    dispatcher->registerAnalyzers(analyzerBuilders);

    for (auto _ : state) {
        for (const auto &packet : packets) {
            for (const auto &identifier : packet.getIdentifiers()) {
                benchmark::DoNotOptimize(dispatcher->lookup(identifier));
            }
        }
    }

    dispatcher->clear();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Path to packet file missing." << std::endl;
        return 1;
    } else if (argc < 3) {
        std::cerr << "Path to analyzer file missing." << std::endl;
        return 1;
    } else if (argc < 4) {
        std::cerr << "Repetition count is missing." << std::endl;
        return 1;
    }

    auto benchmarkFunction = BM_dispatchers;
    if (argc > 4 && std::string(argv[4]) == "startup") {
        // Benchmark startup time instead.
        benchmarkFunction = BM_startup;
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

    uint32_t repetitionCount = std::stoi(argv[3]);

    registerBenchmark(Array, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(Vector, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(TreeMap, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(UnorderedMap, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(Cuckoo, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(Hanov, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(Universal, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    registerBenchmark(SparseUpper, benchmarkFunction, packets, analyzerBuilders, repetitionCount);

    // Fragmented tests
    if (std::string(argv[2]).find("fragmented") != std::string::npos) {
        registerBenchmark(GeneratedSwitchFragmented, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
        registerBenchmark(GeneratedIfFragmented, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    }

    // Zeek default mapping tests
    if (std::string(argv[2]).find("zeek") != std::string::npos) {
        registerBenchmark(GeneratedSwitchZeek, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
        registerBenchmark(GeneratedIfZeek, benchmarkFunction, packets, analyzerBuilders, repetitionCount);
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
