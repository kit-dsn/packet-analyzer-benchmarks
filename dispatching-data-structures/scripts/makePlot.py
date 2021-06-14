#!/usr/bin/env python3
from argparse import ArgumentParser, FileType

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import json
import re

from collections import namedtuple
from matplotlib import rcParams
rcParams.update({'figure.autolayout': True})
rcParams.update({'font.size': 22})


COL_NAMES = {
	"array": "Array",
	"vector": "Dynamic Array",
	"tree": "Tree Map",
	"treemap": "Tree Map",
	"map": "Separate Chaining",
	"unorderedmap": "Separate Chaining",
	"cuckoo": "Cuckoo Hashing",
	"perfect": "Perfect Hashing",
	"hanov": "Perfect Hashing",
	"universal": "Universal Hashing",
	"sparse": "Array Tree",
	"sparseupper": "Array Tree",
	"switch": "Switch",
	"if": "If",
	"generatedswitchsmall": "Switch",
	"generatedifsmall": "If",
	"generatedswitchfragmented": "Switch",
	"generatediffragmented": "If",
	"generatedswitchzeek": "Switch",
	"generatedifzeek": "If",
}

PLOT_HEIGHT = 8
PLOT_WIDTH = 16

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


def filter_times(measurements, timeval="dispatch"):
	df = pd.json_normalize(measurements)

	# Get dispatch_time cols
	cols_dt = [col for col in df.columns if f"{timeval}_time_real" in col]
	df_times = df[cols_dt]
	# Rename to omit suffix
	df_times.columns = [re.match("(.+?)\.", col).group(1) for col in df_times.columns]
	# Rename terms
	df_times = df_times.rename(columns=COL_NAMES)
	# Get mean and CI of all cols
	df_times = calc_mean_and_ci(df_times)

	return df_times


def filter_cache_misses(measurements, layer="l1"):
	df = pd.json_normalize(measurements)

	# Get dispatch_time cols
	cols_dt = [col for col in df.columns if layer in col]
	df_cache = df[cols_dt]
	# Rename to omit suffix
	df_cache.columns = [re.match("(.+?)\.", col).group(1).lower()
		for col in df_cache.columns]
	# Rename terms
	df_cache = df_cache.rename(columns=COL_NAMES)

	# Get mean and CI of all cols
	df_cache = calc_mean_and_ci(df_cache)

	return df_cache


def filter_mapping_json(json_data, mapping_name):
	res_mapping = []
	for mapping in json_data:
		if mapping["mapping_name"] == mapping_name:
			res_mapping.append(mapping)
	return res_mapping


TraceTuple = namedtuple("TraceTuple", "display_name trace_name")
TRACES = [
	TraceTuple("Random", "rand"),
	TraceTuple("CIC-IDS17", "cic-ids17-mon")
]

def plot_DispatchTimesPerMapping(args):
	in_file = args.in_file
	out_file = args.out_file
	# Load JSON for the given mapping
	json_data_full = json.loads(in_file.read())
	json_data_mapping = filter_mapping_json(json_data_full, args.mapping)

	# Get dispatch times for all traces
	times_dict = dict()
	for json_data in json_data_mapping:
		tn = json_data["trace_name"]
		times_dict[tn] = filter_times(json_data["measurements"])

	# Combine all traces in a data frame
	combi = pd.DataFrame({
		TRACES[0].display_name: times_dict[TRACES[0].trace_name].loc["mean"],
		TRACES[1].display_name: times_dict[TRACES[1].trace_name].loc["mean"]},
		index=times_dict[TRACES[0].trace_name].loc["mean"].index)

	err = pd.DataFrame({
		TRACES[0].display_name: times_dict[TRACES[0].trace_name].loc["cf"],
		TRACES[1].display_name: times_dict[TRACES[1].trace_name].loc["cf"]})

	print(err)

	# Plot data frame
	fig, ax = plt.subplots()
	combi.plot(ax=ax, kind="bar",
		yerr=err.T.values.tolist(),
		figsize=(PLOT_WIDTH, PLOT_HEIGHT),
		edgecolor='black', color={"lightgray", "gray"},
		width=0.9,
		ylabel="runtime [ms]",
		zorder=10)
	ax.grid(axis="y", linestyle="--", zorder=0)
	plt.setp(ax.get_xticklabels(), ha="right", rotation=45)
	fig.tight_layout()
	fig.savefig(out_file)


def plot_StartupTimes(args):
	in_file = args.in_file
	out_file = args.out_file
	# Load JSON for the given mapping
	json_data_full = json.loads(in_file.read())
	json_data_mapping = filter_mapping_json(json_data_full, args.mapping)

	# Get startup times
	times_dict = filter_times(json_data_mapping[0]["measurements"], "startup")
	trace_name = json_data_mapping[0]["trace_name"]

	# Combine all traces in a data frame
	combi = pd.DataFrame({
		trace_name: times_dict.loc["mean"]},
		index=times_dict.loc["mean"].index)

	err = pd.DataFrame({
		trace_name: times_dict.loc["cf"]})

	# Plot data frame
	fig, ax = plt.subplots()
	combi.plot(ax=ax, kind="bar",
		yerr=err.T.values.tolist(),
		figsize=(PLOT_WIDTH, PLOT_HEIGHT),
		edgecolor='black', color={"lightgray", "gray"},
		width=0.9,
		ylabel="runtime [ms]",
		zorder=10)
	ax.grid(axis="y", linestyle="--", zorder=0)
	plt.setp(ax.get_xticklabels(), ha="right", rotation=45)
	fig.tight_layout()
	fig.savefig(out_file)


def plot_DispatchTimesAndCacheCIC_IDS(args):
	in_file = args.in_file
	cache_file = args.cache_file
	out_file = args.out_file
	# Load JSON for the given mapping
	json_data_full = json.loads(in_file.read())
	json_data_mapping = filter_mapping_json(json_data_full, args.mapping)
	json_cache_full = json.loads(cache_file.read())
	json_cache_mapping = filter_mapping_json(json_cache_full, args.mapping)

	# Get dispatch times for all traces
	times_dict = dict()
	for json_data in json_data_mapping:
		tn = json_data["trace_name"]
		times_dict[tn] = filter_times(json_data["measurements"])

	# Get cache hits for all traces
	cache_l1_dict = dict()
	cache_l2_dict = dict()
	cache_l3_dict = dict()
	for json_data in json_cache_mapping:
		tn = json_data["trace_name"]
		cache_l1_dict[tn] = filter_cache_misses(json_data["measurements"], "l1")
		cache_l2_dict[tn] = filter_cache_misses(json_data["measurements"], "l2")
		cache_l3_dict[tn] = filter_cache_misses(json_data["measurements"], "l3")

	# Combine all traces in a data frame
	combi = pd.DataFrame({
		"CIC-IDS17": times_dict["cic-ids17-mon"].loc["mean"]},
		index=times_dict["cic-ids17-mon"].loc["mean"].index)

	cache = pd.DataFrame({
		"L1": cache_l1_dict["cic-ids17-mon"].loc["mean"],
		"L2": cache_l2_dict["cic-ids17-mon"].loc["mean"],
		"L3": cache_l3_dict["cic-ids17-mon"].loc["mean"]},
		index=times_dict["cic-ids17-mon"].loc["mean"].index)

	print(cache)

	err = pd.DataFrame({
		"CIC-IDS17": times_dict["cic-ids17-mon"].loc["cf"]})


	fig, ax = plt.subplots()
	# Plot data frame
	combi.plot(ax=ax, kind="bar",
		yerr=err.T.values.tolist(),
		figsize=(PLOT_WIDTH, PLOT_HEIGHT),
		edgecolor='black', color={"gray", "lightgray"},
		width=0.9,
		ylabel="runtime [ms]")

	# Plot Cache misses
	ax2 = ax.twinx()
	ax2.set_yscale('log')
	ax2.scatter(np.arange(len(cache.index)), cache['L1'], marker='o', color="black")

	ax3 = ax.twinx()
	ax3.set_yscale('log')
	rspine = ax3.spines['right']
	rspine.set_position(('axes', 1.25))
	ax3.scatter(np.arange(len(cache.index)), cache['L2'], marker='x', color="black")

	plt.setp(ax.get_xticklabels(), ha="right", rotation=45)
	fig.tight_layout()
	fig.savefig(out_file)


def plot_CacheMissesCIC_IDS(args):
	cache_file = args.cache_file if args.cache_file else args.in_file
	out_file = args.out_file
	# Load JSON for the given mapping
	json_cache_full = json.loads(cache_file.read())
	json_cache_mapping = filter_mapping_json(json_cache_full, args.mapping)

	# Get cache hits for all traces
	cache_l1_dict = dict()
	cache_l2_dict = dict()
	cache_l3_dict = dict()
	for json_data in json_cache_mapping:
		tn = json_data["trace_name"]
		cache_l1_dict[tn] = filter_cache_misses(json_data["measurements"], "l1")
		cache_l2_dict[tn] = filter_cache_misses(json_data["measurements"], "l2")
		cache_l3_dict[tn] = filter_cache_misses(json_data["measurements"], "l3")

	# Combine all Levels in a data frame
	cache_l1 = pd.DataFrame({
		"L1": cache_l1_dict["cic-ids17-mon"].loc["mean"]},
		index=cache_l1_dict["cic-ids17-mon"].loc["mean"].index)
	cache_l2 = pd.DataFrame({
		"L2": cache_l2_dict["cic-ids17-mon"].loc["mean"]},
		index=cache_l1_dict["cic-ids17-mon"].loc["mean"].index)

	err_l1 = pd.DataFrame({
		"CIC-IDS17": cache_l1_dict["cic-ids17-mon"].loc["cf"]})
	err_l2 = pd.DataFrame({
		"CIC-IDS17": cache_l2_dict["cic-ids17-mon"].loc["cf"]})

	fig, ax = plt.subplots()
	fig.set_figheight(PLOT_HEIGHT)
	fig.set_figwidth(PLOT_WIDTH)

	ax2 = ax.twinx()
	ax.set_yscale('log')
	ax2.set_yscale('log')
	# Plot cache misses
	cache_l1.plot(kind="bar",
		ax=ax,
		yerr=err_l1.T.values.tolist(),
		edgecolor='black', color="lightgray",
		width=0.4,
		position=1,
		ylabel="L1 misses",
		legend=False)

	cache_l2.plot(kind="bar",
		ax=ax2,
		yerr=err_l2.T.values.tolist(),
		edgecolor='black', color="gray",
		width=0.4,
		position=0,
		ylabel="L2 misses",
		legend=False)

	ax.legend(loc='upper left')
	ax2.legend(loc='upper right')

	# Shift plot
	l, r = plt.xlim()
	plt.xlim((l-0.4, r))

	plt.setp(ax.get_xticklabels(), ha="right", rotation=45)
	fig.tight_layout()
	fig.savefig(out_file)


def plot_Memory(args):
	in_file = args.in_file
	out_file = args.out_file

	# Load memory data
	df_mem = pd.read_csv(in_file)

	# Rename terms
	df_mem.columns = [col.lower() for col in df_mem.columns]
	df_mem = df_mem.rename(columns=COL_NAMES)
	df_mem = df_mem.set_index("mapping")

	fig, ax = plt.subplots()
	ax.set_yscale("log")

	colors = [str(c) for c in np.arange(0.3, 1, 0.09)]
	df_mem.plot(kind="bar",
		ax=ax,
		edgecolor='black', color=colors,
		width=0.9,
		ylabel="Memory usage [Bytes]",
		xlabel="",
		legend=False)

	bars = ax.patches
	pattern = ["//","O","||","o","\\\\",".","--","+"]
	hatches = np.repeat(pattern, len(df_mem))

	for bar, hatch in zip(bars, hatches):
		bar.set_hatch(hatch)

	ax.legend(loc='upper right')

	# Adapt plot ranges
	l, r = plt.xlim()
	plt.xlim((l, r+1))
	b, t = plt.ylim()
	print(b)
	print(t)
	plt.ylim((1, t))

	fig.set_figheight(PLOT_HEIGHT)
	fig.set_figwidth(PLOT_WIDTH)

	plt.setp(ax.get_xticklabels(), rotation=0)
	plt.show()
	fig.tight_layout()
	fig.savefig(out_file)


def get_arguments(methods):
	parser = ArgumentParser(description='This script plots Packet-Analyzer dispatching simulations.')
	parser.add_argument('method', choices=methods, nargs='?', default=methods[0],
		help='Plotting method (default: %(default)s)')
	parser.add_argument('-r', metavar='FILE', type=FileType('r'), default='-',
		dest='in_file', help='input JSON file [stdin]')
	parser.add_argument('-c', metavar='FILE', type=FileType('r'),
		dest='cache_file', help='input cache JSON file')
	parser.add_argument('-m', metavar='MAPPING', type=str, default='small',
		dest='mapping', help='benchmarked mapping to plot [small]')
	parser.add_argument('-o', metavar='FILE', type=str, default='out.png',
		dest='out_file', help='output PNG file [out.png]')

	return parser.parse_args()


def main():
	methods = [f[5:] for f in globals().keys() if f.startswith("plot_")]
	args = get_arguments(methods)

	globals()["plot_"+ args.method](args)


if __name__ == "__main__":
	main()
