/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Google Inc.
 * Copyright (C) 2021, Collabora Ltd.
 *
 * lc-compliance - The libcamera compliance tool
 */

#include <iomanip>
#include <iostream>
#include <string.h>

#include <gtest/gtest.h>

#include <libcamera/libcamera.h>

#include "../common/options.h"

#include "environment.h"

using namespace libcamera;

enum {
	OptCamera = 'c',
	OptList = 'l',
	OptFilter = 'f',
	OptHelp = 'h',
};

/*
 * Make asserts act like exceptions, otherwise they only fail (or skip) the
 * current function. From gtest documentation:
 * https://google.github.io/googletest/advanced.html#asserting-on-subroutines-with-an-exception
 */
class ThrowListener : public testing::EmptyTestEventListener
{
	void OnTestPartResult(const testing::TestPartResult &result) override
	{
		if (result.type() == testing::TestPartResult::kFatalFailure ||
		    result.type() == testing::TestPartResult::kSkip)
			throw testing::AssertionException(result);
	}
};

static void listCameras(CameraManager *cm)
{
	for (const std::shared_ptr<Camera> &cam : cm->cameras())
		std::cout << "- " << cam->id() << std::endl;
}

static int initCamera(CameraManager *cm, OptionsParser::Options options)
{
	int ret = cm->start();
	if (ret) {
		std::cout << "Failed to start camera manager: "
			  << strerror(-ret) << std::endl;
		return ret;
	}

	if (!options.isSet(OptCamera)) {
		std::cout << "No camera specified, available cameras:" << std::endl;
		listCameras(cm);
		return -ENODEV;
	}

	const std::string &cameraId = options[OptCamera];
	std::shared_ptr<Camera> camera = cm->get(cameraId);
	if (!camera) {
		std::cout << "Camera " << cameraId << " not found, available cameras:" << std::endl;
		listCameras(cm);
		return -ENODEV;
	}

	Environment::get()->setup(cm, cameraId);

	std::cout << "Using camera " << cameraId << std::endl;

	return 0;
}

static int initGtestParameters(char *arg0, OptionsParser::Options options)
{
	std::vector<const char *> argv;
	std::string filterParam;

	argv.push_back(arg0);

	if (options.isSet(OptList))
		argv.push_back("--gtest_list_tests");

	if (options.isSet(OptFilter)) {
		/*
		 * The filter flag needs to be passed as a single parameter, in
		 * the format --gtest_filter=filterStr
		 */
		filterParam = "--gtest_filter=" + options[OptFilter].toString();
		argv.push_back(filterParam.c_str());
	}

	argv.push_back(nullptr);

	int argc = argv.size();
	::testing::InitGoogleTest(&argc, const_cast<char **>(argv.data()));

	return 0;
}

static int initGtest(char *arg0, OptionsParser::Options options)
{
	int ret = initGtestParameters(arg0, options);
	if (ret)
		return ret;

	testing::UnitTest::GetInstance()->listeners().Append(new ThrowListener);

	return 0;
}

static int parseOptions(int argc, char **argv, OptionsParser::Options *options)
{
	OptionsParser parser;
	parser.addOption(OptCamera, OptionString,
			 "Specify which camera to operate on, by id", "camera",
			 ArgumentRequired, "camera");
	parser.addOption(OptList, OptionNone, "List all tests and exit", "list");
	parser.addOption(OptFilter, OptionString,
			 "Specify which tests to run", "filter",
			 ArgumentRequired, "filter");
	parser.addOption(OptHelp, OptionNone, "Display this help message",
			 "help");

	*options = parser.parse(argc, argv);
	if (!options->valid())
		return -EINVAL;

	if (options->isSet(OptHelp)) {
		parser.usage();
		std::cerr << "Further options from Googletest can be passed as environment variables"
			  << std::endl;
		return -EINTR;
	}

	return 0;
}

int main(int argc, char **argv)
{
	OptionsParser::Options options;
	int ret = parseOptions(argc, argv, &options);
	if (ret == -EINTR)
		return EXIT_SUCCESS;
	if (ret < 0)
		return EXIT_FAILURE;

	std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();

	/* No need to initialize the camera if we'll just list tests */
	if (!options.isSet(OptList)) {
		ret = initCamera(cm.get(), options);
		if (ret)
			return ret;
	}

	ret = initGtest(argv[0], options);
	if (ret)
		return ret;

	ret = RUN_ALL_TESTS();

	if (!options.isSet(OptList))
		cm->stop();

	return ret;
}
