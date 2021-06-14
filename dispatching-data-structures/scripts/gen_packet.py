#!/usr/bin/env python3
from argparse import ArgumentParser, FileType
from numpy.random import Generator, PCG64, SeedSequence, default_rng
from gen_analyzer import getProtocolIDs

import numpy as np
import random


SEED=0x3183b0361f7d45ab08e22072e6502d66


def generateLikeABOX(file, packets_per_type):
    packets = [
        ["0001 0608", packets_per_type],		# ARP
        ["0001 0008 0006", packets_per_type],   # IPv4 TCP
        ["0001 0008 0011", packets_per_type],   # IPv4 UDP
        ["0001 DD86 0011", packets_per_type],   # IPv6 UDP
        ["0001 0008 0002", packets_per_type],   # IGMPv3
        ["0001 DD86 003A", packets_per_type],   # ICMPv6
        ["0001 DD86 0000", packets_per_type]    # IPv6 HopByHop
    ]

    total = 0
    while total < packets_per_type * len(packets):
        current = random.randint(0, len(packets) - 1)
        if packets[current][1] > 0:
            file.write(packets[current][0] + " A\n")
            packets[current][1] -= 1
            total += 1
            if total % 10000 == 0:
                print(str(total) + " done...", end="\r")

    print("Created " + str(total) + " packets.")


def generateStaticVals(file, pdus_max):
    #packet = "007F 0069 86DD 002F" # RadioRap
    packet = "0001 88A8 8100 8864 0057" # Eth, QinQ, PPPoE

    pkt_count = 0
    pdus_count = 0
    pdus_per_pkt = len(packet.split())
    while pdus_count < pdus_max:
        file.write(packet + " A\n")
        pdus_count += pdus_per_pkt
        pkt_count += 1
        if pdus_count % 10000 == 0:
            print(str(pdus_count) + " done...", end="\r")

    print("Created " + str(pkt_count) + " packets.")


def generateRandVals(file, pdus_max, layers=3, max_id=0xFFFF, seed=SEED):
    random.seed(a=seed, version=2)

    pkt_count = 0
    pdus_count = 0
    while pdus_count < pdus_max:
        packet = ""
        for l in range(layers):
            identifier = random.randrange(0, max_id)
            packet += hex(identifier)[2:] + " "
            pdus_count += 1

        file.write(packet + " A\n")
        pkt_count += 1
        if pdus_count % 10000 == 0:
            print(str(pdus_count) + " done...", end="\r")

    print("Created {} PDUs in {} packets.".format(pdus_count, pkt_count))


def generateRandValsEx(file, pdus_max, layers=3, num_ids=10000, max_id=0xFFFF, rng=default_rng(SEED)):
    ids = getProtocolIDs(num_ids, include_zeek=True, max_id=max_id)

    # Create pdus_max random indices to select PDU IDs
    selected_ids = np.asarray(ids)[rng.integers(0, len(ids), pdus_max)]

    pkt = ""
    pkt_count = 0
    for i, pdu in enumerate(selected_ids, 1):
        pkt += hex(pdu)[2:] + " "

        if i % layers == 0:
            file.write(pkt + " A\n")
            pkt_count += 1
            pkt = ""

        if i % 10000 == 0:
            print(str(i) + " done...", end="\r")

    print("Created {} PDUs in {} packets.".format(len(selected_ids), pkt_count))


def create_trace_file(name):
    file = open(name, "w")
    file.write("# PACKETS\n")
    return file

def gen_static(args):
    file = create_trace_file(args.out_file)
    generateStaticVals(file, args.num_pdus)

def gen_random(args):
    file = create_trace_file(args.out_file)
    generateRandVals(file, args.num_pdus,
        layers=args.num_layer,
        max_id=args.max_id)

def gen_random_ex(args):
    # Generate RNG streams
    seed_sequence = SeedSequence(SEED)
    seeds = seed_sequence.spawn(args.num_files)
    rngs = [Generator(PCG64(s)) for s in seeds]

    for i, rng in enumerate(rngs):
        fname = "{}_{}".format(args.out_file, i)
        file = create_trace_file(fname)

        generateRandValsEx(file, args.num_pdus,
            layers=args.num_layer,
            num_ids=args.num_ids,
            max_id=args.max_id,
            rng=rng)

def gen_abox(args):
    print("NOTE: Going to create ABOX composition using {} packets per type.".format(args.num_pdus))
    file = create_trace_file(args.out_file)
    generateLikeABOX(file, args.num_pdus)


# Handle hex arguments
def auto_int(x):
    return int(x, 0)

def get_arguments(methods):
    parser = ArgumentParser(description='This script generates packet benchmark traces.')
    parser.add_argument('method', choices=methods, nargs='?', default=methods[0],
        help='Generation method (default: %(default)s)')
    parser.add_argument('-n', metavar='NUM_PDUS', type=int, default=10000000,
        dest='num_pdus', help='total number of PDUs [10.000.000]')
    parser.add_argument('-i', metavar='NUM_IDS', type=int, default=10000,
        dest='num_ids', help='number of identifiers [10.000]')
    parser.add_argument('-m', metavar='MAX_ID', type=auto_int, default=10000,
        dest='max_id', help='maximum identifier value [10.000]')
    parser.add_argument('-l', metavar='NUM_LAYER', type=int, default=3,
        dest='num_layer', help='number of layers per packet [3]')
    parser.add_argument('-c', metavar='NUM_TRACES', type=int, default=1,
        dest='num_files', help='number of files to generate [1]')
    parser.add_argument('-o', metavar='FILE', type=str, default='out',
        dest='out_file', help='output trace file [out]')

    return parser.parse_args()


def main():
    methods = [f[4:] for f in globals().keys() if f.startswith("gen_")]
    args = get_arguments(methods)

    globals()["gen_"+ args.method](args)


if __name__ == "__main__":
    main()
