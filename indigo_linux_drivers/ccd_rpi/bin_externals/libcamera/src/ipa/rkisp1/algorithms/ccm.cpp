/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024, Ideas On Board
 *
 * RkISP1 Color Correction Matrix control algorithm
 */

#include "ccm.h"

#include <map>

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>

#include <libcamera/control_ids.h>

#include <libcamera/ipa/core_ipa_interface.h>

#include "libcamera/internal/yaml_parser.h"

#include "libipa/fixedpoint.h"
#include "libipa/interpolator.h"

/**
 * \file ccm.h
 */

namespace libcamera {

namespace ipa::rkisp1::algorithms {

/**
 * \class Ccm
 * \brief A color correction matrix algorithm
 */

LOG_DEFINE_CATEGORY(RkISP1Ccm)

/**
 * \copydoc libcamera::ipa::Algorithm::init
 */
int Ccm::init([[maybe_unused]] IPAContext &context, const YamlObject &tuningData)
{
	int ret = ccm_.readYaml(tuningData["ccms"], "ct", "ccm");
	if (ret < 0) {
		LOG(RkISP1Ccm, Warning)
			<< "Failed to parse 'ccm' "
			<< "parameter from tuning file; falling back to unit matrix";
		ccm_.setData({ { 0, Matrix<float, 3, 3>::identity() } });
	}

	ret = offsets_.readYaml(tuningData["ccms"], "ct", "offsets");
	if (ret < 0) {
		LOG(RkISP1Ccm, Warning)
			<< "Failed to parse 'offsets' "
			<< "parameter from tuning file; falling back to zero offsets";

		offsets_.setData({ { 0, Matrix<int16_t, 3, 1>({ 0, 0, 0 }) } });
	}

	return 0;
}

void Ccm::setParameters(struct rkisp1_cif_isp_ctk_config &config,
			const Matrix<float, 3, 3> &matrix,
			const Matrix<int16_t, 3, 1> &offsets)
{
	/*
	 * 4 bit integer and 7 bit fractional, ranging from -8 (0x400) to
	 * +7.992 (0x3ff)
	 */
	for (unsigned int i = 0; i < 3; i++) {
		for (unsigned int j = 0; j < 3; j++)
			config.coeff[i][j] =
				floatingToFixedPoint<4, 7, uint16_t, double>(matrix[i][j]);
	}

	for (unsigned int i = 0; i < 3; i++)
		config.ct_offset[i] = offsets[i][0] & 0xfff;

	LOG(RkISP1Ccm, Debug) << "Setting matrix " << matrix;
	LOG(RkISP1Ccm, Debug) << "Setting offsets " << offsets;
}

/**
 * \copydoc libcamera::ipa::Algorithm::prepare
 */
void Ccm::prepare(IPAContext &context, const uint32_t frame,
		  IPAFrameContext &frameContext, RkISP1Params *params)
{
	uint32_t ct = context.activeState.awb.temperatureK;

	/*
	 * \todo The colour temperature will likely be noisy, add filtering to
	 * avoid updating the CCM matrix all the time.
	 */
	if (frame > 0 && ct == ct_) {
		frameContext.ccm.ccm = context.activeState.ccm.ccm;
		return;
	}

	ct_ = ct;
	Matrix<float, 3, 3> ccm = ccm_.getInterpolated(ct);
	Matrix<int16_t, 3, 1> offsets = offsets_.getInterpolated(ct);

	context.activeState.ccm.ccm = ccm;
	frameContext.ccm.ccm = ccm;

	auto config = params->block<BlockType::Ctk>();
	config.setEnabled(true);
	setParameters(*config, ccm, offsets);
}

/**
 * \copydoc libcamera::ipa::Algorithm::process
 */
void Ccm::process([[maybe_unused]] IPAContext &context,
		  [[maybe_unused]] const uint32_t frame,
		  IPAFrameContext &frameContext,
		  [[maybe_unused]] const rkisp1_stat_buffer *stats,
		  ControlList &metadata)
{
	metadata.set(controls::ColourCorrectionMatrix, frameContext.ccm.ccm.data());
}

REGISTER_IPA_ALGORITHM(Ccm, "Ccm")

} /* namespace ipa::rkisp1::algorithms */

} /* namespace libcamera */
