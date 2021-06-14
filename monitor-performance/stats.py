#!/usr/bin/env python3
from argparse import ArgumentParser, FileType

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import json
import re

Z = 1.96  # >30
T = 2.262 # 10

def calc_mean_and_ci(df):
	df = df.agg(["mean", "count", "std"])

	ci_u = []
	ci_l = []
	cf = []
	for c in df.columns:
		mean, count, std = df[c]
		ci_u.append(mean + T*std/np.sqrt(count))
		ci_l.append(mean - T*std/np.sqrt(count))
		cf.append(Z*std/np.sqrt(count))

	s_ci_u = pd.Series(ci_u, df.columns, name="ci_u")
	s_ci_l = pd.Series(ci_l, df.columns, name="ci_l")
	s_cf = pd.Series(cf, df.columns, name="cf")
	df = df.append(s_ci_u)
	df = df.append(s_ci_l)
	df = df.append(s_cf)

	return df


def print_stats(json_data):
	df = pd.json_normalize(json_data)
	df = df[["time", "memory"]]

	print(calc_mean_and_ci(df))


def stats_full(args):
	orig_file = args.orig_file
	extd_file = args.extd_file
	json_orig = json.loads(orig_file.read())
	json_extd = json.loads(extd_file.read())

	print("Original Zeek:\n" + ("=" * 15))
	print_stats(json_orig)
	print("\n\nExtended Zeek:\n" + ("=" * 15))
	print_stats(json_extd)


def get_arguments(methods):
	parser = ArgumentParser(description='This script calculates Zeek benchmark stats.')
	parser.add_argument('method', choices=methods, nargs='?', default=methods[0],
		help='Calculation method (default: %(default)s)')
	parser.add_argument('-o', metavar='FILE', type=FileType('r'), default='orig/stats.json',
		dest='orig_file', help='orig stats JSON file [orig/stats.json]')
	parser.add_argument('-e', metavar='FILE', type=FileType('r'), default='extd/stats.json',
		dest='extd_file', help='extd stats JSON file [extd/stats.json]')

	return parser.parse_args()


def main():
	methods = [f[6:] for f in globals().keys() if f.startswith("stats_")]
	args = get_arguments(methods)

	globals()["stats_"+ args.method](args)


if __name__ == "__main__":
	main()
