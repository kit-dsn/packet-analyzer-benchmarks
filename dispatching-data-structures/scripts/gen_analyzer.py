#!/usr/bin/env python3
from argparse import ArgumentParser, FileType

import random

# Known analyzers based on Zeek
KNOWN_ANALYZERS = {
    0x1: "ETHAnalyzer",
    0x6: "TCPAnalyzer",
    0x11: "UDPAnalyzer",
    0x21: "IPv4Analyzer",
    0x57: "IPv6Analyzer",
    0x0800: "IPv4Analyzer",
    0x86DD: "IPv6Analyzer"

}
DEFAULT_ANALYZER = "UnknownAnalyzer"
ZEEK_IDS = [1, 6, 17, 0x0800, 0x0806, 0x86DD]
#ZEEK_IDS = list(KNOWN_ANALYZERS.keys())

SEED = "ae9e09eeb4e4ceabbd5bc0aa78901b42"


def getProtocolIDs(number, include_zeek=False, max_id=0xFFFF, seed=SEED):
    random.seed(a=seed, version=2)

    ids = random.sample(range(max_id), k=number)

    if include_zeek:
        number -= len(ZEEK_IDS)
        new_ids = list(set(ids).difference(ZEEK_IDS))
        ids = ZEEK_IDS + new_ids[:number]

    return ids


def generateAnalyzers(filename, analyzer_ids_list):
    file = open(filename, "w")
    file.write("# ANALYZERS\n")

    for analyzer_id in analyzer_ids_list:
        if analyzer_id in KNOWN_ANALYZERS:
            file.write(hex(analyzer_id)[2:] + " " + KNOWN_ANALYZERS[analyzer_id] + "\n")
        else:
            file.write(hex(analyzer_id)[2:] + " " + DEFAULT_ANALYZER + "\n")

    print("Created " + str(len(analyzer_ids_list)) + " analyzers.")
    file.close()


def generateShort(filename):
    generateAnalyzers(filename, list(range(0, 65536)))


def generateShortM3(filename):
    generateAnalyzers(filename, list(range(0, 65533)))


def generateLarge(filename, num):
    generateAnalyzers(filename, list(range(1, num + 1)))


def generateBase(filename):
    generateAnalyzers(filename, [1, 6, 8, 17, 0x251C])


def generateBaseno6(filename):
    generateAnalyzers(filename, [1, 3, 4, 6, 7])


def generateOutliers(filename):
    generateAnalyzers(filename, [1, 251, 253, 262, 0x251C])


def generateChunked(filename, num, max_id=0xFFFF):
    # generate num IDs equally distributed between 0 and max_id
    generateAnalyzers(filename,
        [i*max_id//num + max_id//(2*num) for i in range(num)])


def generateFragmented(filename, num, max_id=0xFFFF):
    # generate num random IDs in the ID space including Zeek values
    generateAnalyzers(filename,
        sorted(getProtocolIDs(num, include_zeek=True, max_id=max_id)))


def generateZeek(filename):
    generateAnalyzers(filename, [1, 6, 17, 0x0800, 0x0806, 0x86DD])


# Interface
def gen_chunked(args):
    generateChunked(args.out_file, args.num_ids, max_id=args.max_id)

def gen_fragmented(args):
    generateFragmented(args.out_file, args.num_ids, max_id=args.max_id)

def gen_zeek(args):
    generateZeek(args.out_file)

def gen_all(args):
    # Generate the default mappings
    generateFragmented("fragmented", args.num_ids, max_id=args.max_id)
    generateZeek("zeek")


# Handle hex arguments
def auto_int(x):
    return int(x, 0)

def get_arguments(methods):
    parser = ArgumentParser(description='This script generates packet benchmark analyzer mappings.')
    parser.add_argument('method', choices=methods, nargs='?', default=methods[0],
        help='Generation method (default: %(default)s)')
    parser.add_argument('-n', metavar='NUM_IDS', type=int, default=100,
        dest='num_ids', help='number of identifiers to generate [100]')
    parser.add_argument('-m', metavar='MAX_ID', type=auto_int, default=10000,
        dest='max_id', help='maximum identifier value [10.000]')
    parser.add_argument('-o', metavar='FILE', type=str, default='out',
        dest='out_file', help='output mapping file [out]')

    return parser.parse_args()


def main():
    methods = [f[4:] for f in globals().keys() if f.startswith("gen_")]
    args = get_arguments(methods)

    globals()["gen_"+ args.method](args)


if __name__ == "__main__":
    main()
