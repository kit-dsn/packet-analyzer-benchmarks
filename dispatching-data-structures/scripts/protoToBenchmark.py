#!/usr/bin/env python3
from __future__ import print_function
from argparse import ArgumentParser, FileType
import json
import sys

# Use tshark to generate protocol files:
# tshark -r PCAP_FILE -T fields -e frame.protocols > PROTO_FILE

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


LAYER_IDS = {
	"eth": "1",
	"ethertype": None,
	"ip": "0x0800",
	"ipv6": "0x86dd",
	"ipv6.hopopts": None,
	"tcp": "6",
	"udp": "17",
	"arp": "0x0806",
	"lldp": "0x88cc",
}

TERM_LAYER = ["tcp", "udp", "arp", "igmp", "icmp", "icmpv6", "llc", "lldp", "data"]

DEFAULT_ID = "A"

def read_proto(args):
	max_pdus = args.max_pdus
	pdu_count = 0
	pkt_count = 0
	print("# PACKETS")
	for line in args.in_file:
		ids = []
		layer = line.strip().split(":")
		
		for l in layer:
			lid = LAYER_IDS.get(l)
			if lid:
				ids.append(lid)
			if l in TERM_LAYER:
				break

			if l not in LAYER_IDS.keys():
				eprint(">>> Unknown layer {} in {}".format(l, line.strip()))

		pdu_count += len(ids)
		pkt_count += 1

		ids.append("A")
		print(" ".join(ids))

		if (max_pdus > 0 and pdu_count >= max_pdus):
			break

	eprint("Extracted {} PDUs in {} packets".format(pdu_count, pkt_count))



def get_arguments(methods):
	parser = ArgumentParser(description='This script generates benchmark files from tshark.')
	parser.add_argument('method', choices=methods, nargs='?', default=methods[0],
		help='Plotting method (default: %(default)s)')
	parser.add_argument('-r', metavar='FILE', type=FileType('r'), default='-',
		dest='in_file', help='input file [stdin]')
	parser.add_argument('-o', metavar='FILE', type=FileType('w'), default='out.txt',
		dest='out_file', help='output benchmark file [out.txt]')
	parser.add_argument('-n', metavar='NUM', type=int, default=-1,
		dest='max_pdus', help='number of PDUs if (0 for all) [0]')

	return parser.parse_args()


def main():
	methods = [f[5:] for f in globals().keys() if f.startswith("read_")]
	args = get_arguments(methods)

	globals()["read_"+ args.method](args)


if __name__ == "__main__":
	main()
