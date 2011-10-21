/** Convert a graph to dot format.
 * Written by Shaun Jackman <sjackman@bcgsc.ca>.
 */
#include "ContigGraph.h"
#include "ContigProperties.h"
#include "DirectedGraph.h"
#include "Estimate.h"
#include "GraphIO.h"
#include "GraphUtil.h"
#include "IOUtil.h"
#include "Uncompress.h"
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <iterator> // for ostream_iterator
#include <utility>

using namespace std;
using namespace std::rel_ops;

#define PROGRAM "abyss-todot"

static const char VERSION_MESSAGE[] =
PROGRAM " (" PACKAGE_NAME ") " VERSION "\n"
"Written by Shaun Jackman.\n"
"\n"
"Copyright 2011 Canada's Michael Smith Genome Science Centre\n";

static const char USAGE_MESSAGE[] =
"Usage: " PROGRAM " [FILE]...\n"
"Convert the specified graph to dot format.\n"
"\n"
"  -k, --kmer=N   report the mean k-mer coverage, otherwise\n"
"                 the sum k-mer coverage is reported\n"
"      --adj             output the graph in adj format\n"
"      --dot             output the graph in dot format [default]\n"
"      --dot-meancov     same as above but give the mean coverage\n"
"      --sam             output the graph in SAM format\n"
"  -e, --estimate output distance estimates\n"
"  -v, --verbose  display verbose output\n"
"      --help     display this help and exit\n"
"      --version  output version information and exit\n"
"\n"
"Report bugs to <" PACKAGE_BUGREPORT ">.\n";

namespace opt {
 	unsigned k; // used by Distance
	static int verbose;

	/** Output distance estimates. */
	bool estimate;

	/** Output format */
	int format = DOT; // used by ContigProperties
}

static const char shortopts[] = "ek:v";

enum { OPT_HELP = 1, OPT_VERSION };

static const struct option longopts[] = {
	{ "adj",     no_argument,       &opt::format, ADJ },
	{ "dot",     no_argument,       &opt::format, DOT },
	{ "dot-meancov", no_argument,   &opt::format, DOT_MEANCOV },
	{ "sam",     no_argument,       &opt::format, SAM },
	{ "estimate", no_argument,      NULL, 'e' },
	{ "kmer",    required_argument, NULL, 'k' },
	{ "verbose", no_argument,       NULL, 'v' },
	{ "help",    no_argument,       NULL, OPT_HELP },
	{ "version", no_argument,       NULL, OPT_VERSION },
	{ NULL, 0, NULL, 0 }
};

/** Read a graph from the specified file. */
template <typename Graph>
static void readGraph(const string& path, Graph& g)
{
	if (opt::verbose > 0)
		cerr << "Reading `" << path << "'...\n";
	ifstream fin(path.c_str());
	istream& in = path == "-" ? cin : fin;
	assert_good(in, path);
	in >> g;
	assert(in.eof());
	if (opt::verbose > 0)
		printGraphStats(cerr, g);
	ContigID::lock();
}

/** Convert the specified graph to dot format. */
template <typename Graph>
void toDot(int argc, char** argv, const string& commandLine,
		Graph& g)
{
	if (optind < argc) {
		for (; optind < argc; optind++)
			readGraph(argv[optind], g);
	} else
		readGraph("-", g);

	write_graph(cout, g, PROGRAM, commandLine);
	assert(cout.good());
}

int main(int argc, char** argv)
{
	string commandLine;
	{
		ostringstream ss;
		char** last = argv + argc - 1;
		copy(argv, last, ostream_iterator<const char *>(ss, " "));
		ss << *last;
		commandLine = ss.str();
	}

	bool die = false;
	for (int c; (c = getopt_long(argc, argv,
					shortopts, longopts, NULL)) != -1;) {
		istringstream arg(optarg != NULL ? optarg : "");
		switch (c) {
		  case '?': die = true; break;
		  case 'e': opt::estimate = true; break;
		  case 'k': arg >> opt::k; break;
		  case 'v': opt::verbose++; break;
		  case OPT_HELP:
			cout << USAGE_MESSAGE;
			exit(EXIT_SUCCESS);
		  case OPT_VERSION:
			cout << VERSION_MESSAGE;
			exit(EXIT_SUCCESS);
		}
	}

	if (argc - optind < 0) {
		cerr << PROGRAM ": missing arguments\n";
		die = true;
	}

	if (die) {
		cerr << "Try `" << PROGRAM
			<< " --help' for more information.\n";
		exit(EXIT_FAILURE);
	}

	if (opt::estimate) {
		ContigGraph<DirectedGraph<ContigProperties, DistanceEst> > g;
		toDot(argc, argv, commandLine, g);
	} else {
		ContigGraph<DirectedGraph<ContigProperties, Distance> > g;
		toDot(argc, argv, commandLine, g);
	}

	return 0;
}