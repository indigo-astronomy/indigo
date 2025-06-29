/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * Image Processing Algorithm module
 */

#include "libcamera/internal/ipa_module.h"

#include <algorithm>
#include <ctype.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcamera/base/file.h>
#include <libcamera/base/log.h>
#include <libcamera/base/span.h>
#include <libcamera/base/utils.h>

#include <libcamera/ipa/ipa_interface.h>

#include "libcamera/internal/pipeline_handler.h"

#include <iostream>

/**
 * \file ipa_module.h
 * \brief Image Processing Algorithm module
 */

/**
 * \file ipa_module_info.h
 * \brief Image Processing Algorithm module information
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(IPAModule)

namespace {

template<typename T>
typename std::remove_extent_t<T> *elfPointer(Span<const uint8_t> elf,
					     off_t offset, size_t objSize)
{
	size_t size = offset + objSize;
	if (size > elf.size() || size < objSize)
		return nullptr;

	return reinterpret_cast<typename std::remove_extent_t<T> *>(
		reinterpret_cast<const char *>(elf.data()) + offset);
}

template<typename T>
typename std::remove_extent_t<T> *elfPointer(Span<const uint8_t> elf,
					     off_t offset)
{
	return elfPointer<T>(elf, offset, sizeof(T));
}

} /* namespace */

/**
 * \def IPA_MODULE_API_VERSION
 * \brief The IPA module API version
 *
 * This version number specifies the version for the layout of
 * struct IPAModuleInfo. The IPA module shall use this macro to
 * set its moduleAPIVersion field.
 *
 * \sa IPAModuleInfo::moduleAPIVersion
 */

/**
 * \struct IPAModuleInfo
 * \brief Information of an IPA module
 *
 * This structure contains the information of an IPA module. It is loaded,
 * read, and validated before anything else is loaded from the shared object.
 *
 * \var IPAModuleInfo::moduleAPIVersion
 * \brief The IPA module API version that the IPA module implements
 *
 * This version number specifies the version for the layout of
 * struct IPAModuleInfo. The IPA module shall report here the version that
 * it was built for, using the macro IPA_MODULE_API_VERSION.
 *
 * \var IPAModuleInfo::pipelineVersion
 * \brief The pipeline handler version that the IPA module is for
 *
 * \var IPAModuleInfo::pipelineName
 * \brief The name of the pipeline handler that the IPA module is for
 *
 * This name is used to match a pipeline handler with the module.
 *
 * \var IPAModuleInfo::name
 * \brief The name of the IPA module
 *
 * The name may be used to build file system paths to IPA-specific resources.
 * It shall only contain printable characters, and may not contain '*', '?' or
 * '\'. For IPA modules included in libcamera, it shall match the directory of
 * the IPA module in the source tree.
 *
 * \todo Allow user to choose to isolate open source IPAs
 */

/**
 * \var ipaModuleInfo
 * \brief Information of an IPA module
 *
 * An IPA module must export a struct IPAModuleInfo of this name.
 */

/**
 * \class IPAModule
 * \brief Wrapper around IPA module shared object
 */

/**
 * \brief Construct an IPAModule instance
 * \param[in] libPath path to IPA module shared object
 *
 * Loads the IPAModuleInfo from the IPA module shared object at libPath.
 * The IPA module shared object file must be of the same endianness and
 * bitness as libcamera.
 *
 * The caller shall call the isValid() function after constructing an
 * IPAModule instance to verify the validity of the IPAModule.
 */
IPAModule::IPAModule(const std::string &libPath)
	: libPath_(libPath), valid_(false), loaded_(false),
	  dlHandle_(nullptr), ipaCreate_(nullptr)
{
	if (loadIPAModuleInfo() < 0)
		return;

	valid_ = true;
}

IPAModule::~IPAModule()
{
	if (dlHandle_)
		dlclose(dlHandle_);
}

int IPAModule::loadIPAModuleInfo()
{
	// We are only targeting Raspberry Pi
	//
	info_.moduleAPIVersion = IPA_MODULE_API_VERSION;
	info_.pipelineVersion = 1; // Placeholder for pipeline version
	std::snprintf(info_.name, sizeof(info_.name), "%s", "rpi");
	std::snprintf(info_.pipelineName, sizeof(info_.pipelineName), "%s", libPath_.c_str()) ;	

	return 0;
}

/**
 * \brief Check if the IPAModule instance is valid
 *
 * An IPAModule instance is valid if the IPA module shared object exists and
 * the IPA module information it contains was successfully retrieved and
 * validated.
 *
 * \return True if the IPAModule is valid, false otherwise
 */
bool IPAModule::isValid() const
{
	return valid_;
}

/**
 * \brief Retrieve the IPA module information
 *
 * The content of the IPA module information is loaded from the module,
 * and is valid only if the module is valid (as returned by isValid()).
 * Calling this function on an invalid module is an error.
 *
 * \return the IPA module information
 */
const struct IPAModuleInfo &IPAModule::info() const
{
	return info_;
}

/**
 * \brief Retrieve the IPA module signature
 *
 * The IPA module signature is stored alongside the IPA module in a file with a
 * '.sign' suffix, and is loaded when the IPAModule instance is created. This
 * function returns the signature without verifying it. If the signature is
 * missing, the returned vector will be empty.
 *
 * \return The IPA module signature
 */
const std::vector<uint8_t> IPAModule::signature() const
{
	return signature_;
}

/**
 * \brief Retrieve the IPA module path
 *
 * The IPA module path is the file name and path of the IPA module shared
 * object from which the IPA module was created.
 *
 * \return The IPA module path
 */
const std::string &IPAModule::path() const
{
	return libPath_;
}

/**
 * \brief Load the IPA implementation factory from the shared object
 *
 * The IPA module shared object implements an IPAInterface object to be used
 * by pipeline handlers. This function loads the factory function from the
 * shared object. Later, createInterface() can be called to instantiate the
 * IPAInterface.
 *
 * This function only needs to be called successfully once, after which
 * createInterface() can be called as many times as IPAInterface instances are
 * needed.
 *
 * Calling this function on an invalid module (as returned by isValid()) is
 * an error.
 *
 * \return True if load was successful, or already loaded, and false otherwise
 */
bool IPAModule::load()
{
    // IPA module is static so it is always loaded
	//	
	return true;
}

/**
 * \brief Instantiate an IPA interface
 *
 * After loading the IPA module with load(), this function creates an instance
 * of the IPA module interface.
 *
 * Calling this function on a module that has not yet been loaded, or an
 * invalid module (as returned by load() and isValid(), respectively) is
 * an error.
 *
 * \return The IPA interface on success, or nullptr on error
 */
IPAInterface *IPAModule::createInterface()
{
	if(path() == "rpi/vc4")
		return ipaCreateVc4();
	// As of now, we only support the vc4 IPA module.
	// If we add more IPA modules, we can use a switch statement or a map to
	// handle the different cases.
	// else if(path() == "rpi/pisp")
	// 	return ipaCreatePisp();
	//

	return nullptr;
}

/**
 * \brief Verify if the IPA module matches a given pipeline handler
 * \param[in] pipe Pipeline handler to match with
 * \param[in] minVersion Minimum acceptable version of IPA module
 * \param[in] maxVersion Maximum acceptable version of IPA module
 *
 * This function checks if this IPA module matches the \a pipe pipeline handler,
 * and the input version range.
 *
 * \return True if the pipeline handler matches the IPA module, or false otherwise
 */
bool IPAModule::match(PipelineHandler *pipe) const
{
	std::cout << "IPAModule::match...pipe->name(): " << pipe->name() << std::endl;
	return !strcmp(info_.pipelineName, pipe->name());
}

std::string IPAModule::logPrefix() const
{
	return utils::basename(libPath_.c_str());
}

} /* namespace libcamera */
