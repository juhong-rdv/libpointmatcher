// kate: replace-tabs off; indent-width 4; indent-mode normal
// vim: ts=4:sw=4:noexpandtab
/*

Copyright (c) 2010--2011,
François Pomerleau and Stephane Magnenat, ASL, ETHZ, Switzerland
You can contact the authors at <f dot pomerleau at gmail dot com> and
<stephane at magnenat dot net>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETH-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "pointmatcher/PointMatcher.h"
#include <cassert>
#include <iostream>
#include "boost/filesystem.hpp"

using namespace std;

void validateArgs(const int argc, const char *argv[], bool& isCSV, string&, string&);
void usage(const char *argv[]);

/**
  * Code example for ICP taking 2 points clouds (2D or 3D) relatively close 
  * and computing the transformation between them.
  */
int main(int argc, const char *argv[])
{
	bool isCSV = true;
	string configFile;
	string outputBaseFile("test");
	validateArgs(argc, argv, isCSV, configFile, outputBaseFile);
	const char *refFile(argv[argc-2]);
	const char *dataFile(argv[argc-1]);
	
	typedef PointMatcher<float> PM;
	typedef PM::Parameters Parameters;
	
	// Load point clouds
	PM::DataPoints ref;
	PM::DataPoints data;
	if(isCSV)
	{
		ref = loadCSV<PM::ScalarType>(refFile);
		data = loadCSV<PM::ScalarType>(dataFile);
	}
	else
	{
		ref = loadVTK<PM::ScalarType>(refFile);
		data= loadVTK<PM::ScalarType>(dataFile);
	}

	// Create the default ICP algorithm
	PM::ICP icp;
	
	if (configFile.empty())
	{
		// See the implementation of setDefault() to create a custom ICP algorithm
		icp.setDefault();
	}
	else
	{
		// load YAML config
		ifstream ifs(configFile.c_str());
		if (!ifs.good())
		{
			cerr << "Cannot open config file " << configFile << ", usage:"; usage(argv); exit(1);
		}
		icp.loadFromYaml(ifs);
	}

	// Compute the transformation to express data in ref
	PM::TransformationParameters T = icp(data, ref);

	// Transform data to express it in ref
	PM::TransformFeatures transform;
	PM::DataPoints data_out = transform.compute(data, T);
	
	// Safe files to see the results
	saveVTK<PM::ScalarType>(ref, outputBaseFile + "_ref.vtk");
	saveVTK<PM::ScalarType>(data, outputBaseFile + "_data_in.vtk");
	saveVTK<PM::ScalarType>(data_out, outputBaseFile + "_data_out.vtk");
	cout << "Final transformation:" << endl << T << endl;

	return 0;
}


void validateArgs(const int argc, const char *argv[], bool& isCSV, string& configFile, string& outputBaseFile)
{
	if (argc < 3)
	{
		cerr << "Not enough arguments, usage:"; usage(argv); exit(1);
	}
	const int endOpt(argc - 2);
	for (int i = 1; i < endOpt; i += 2)
	{
		const string opt(argv[i]);
		if (i + 1 > endOpt)
		{
			cerr << "Missing value for option " << opt << ", usage:"; usage(argv); exit(1);
		}
		if (opt == "--config")
			configFile = argv[i+1];
		else if (opt == "--output")
			outputBaseFile = argv[i+1];
		else
		{
			cerr << "Unknown option " << opt << ", usage:"; usage(argv); exit(1);
		}
	}
	
	// Validate extension
	const boost::filesystem::path pathRef(argv[argc-2]);
	const boost::filesystem::path pathData(argv[argc-1]);

	string refExt = boost::filesystem::extension(pathRef);
	string dataExt = boost::filesystem::extension(pathData);

	if (!(refExt == ".vtk" || refExt == ".csv"))
	{
		cerr << "Reference file extension must be .vtk or .csv, found " << refExt << " instead" << endl;
		exit(2);
	}
	
	if (!(dataExt == ".vtk" || dataExt == ".csv"))
	{
		cerr << "Reading file extension must be .vtk or .csv, found " << dataExt << " instead" << endl;
		exit(3);
	}

	if (dataExt != refExt)
	{
		cerr << "File extension between reference and reading should be the same" << endl;
		exit(4);
	}

	isCSV = (dataExt == ".csv");
}

void usage(const char *argv[])
{
	cerr << endl;
	cerr << "  " << argv[0] << " [OPTIONS] reference.csv reading.csv" << endl;
	cerr << endl;
	cerr << "OPTIONS can be a combination of:" << endl;
	cerr << "--config YAML_CONFIG_FILE  Load the config from a YAML file (default: default parameters)" << endl;
	cerr << "--output FILENAME          Name of output files (default: test)" << endl;
	cerr << endl;
	cerr << "Running this program will create 3 vtk ouptput files: ./test_ref.vtk, ./test_data_in.vtk and ./test_data_out.vtk" << endl;
	cerr << endl << "2D Example:" << endl;
	cerr << "  " << argv[0] << " ../examples/data/2D_twoBoxes.csv ../examples/data/2D_oneBox.csv" << endl;
	cerr << endl << "3D Example:" << endl;
	cerr << "  " << argv[0] << " ../examples/data/car_cloud400.csv ../examples/data/car_cloud401.csv" << endl;
	cerr << endl;
}
