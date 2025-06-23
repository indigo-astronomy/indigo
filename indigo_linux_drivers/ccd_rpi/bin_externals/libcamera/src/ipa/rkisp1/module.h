/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022, Ideas On Board
 *
 * RkISP1 IPA Module
 */

#pragma once

#include <linux/rkisp1-config.h>

#include <libcamera/ipa/rkisp1_ipa_interface.h>

#include <libipa/module.h>

#include "ipa_context.h"
#include "params.h"

namespace libcamera {

namespace ipa::rkisp1 {

using Module = ipa::Module<IPAContext, IPAFrameContext, IPACameraSensorInfo,
			   RkISP1Params, rkisp1_stat_buffer>;

} /* namespace ipa::rkisp1 */

} /* namespace libcamera*/
