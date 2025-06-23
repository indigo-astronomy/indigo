# SPDX-License-Identifier: CC0-1.0

rpi_ipa_common_sources = files([
    'ipa_base.cpp',
    '../cam_helper/cam_helper.cpp',
    '../cam_helper/cam_helper_ov5647.cpp',
    '../cam_helper/cam_helper_imx219.cpp',
    '../cam_helper/cam_helper_imx283.cpp',
    '../cam_helper/cam_helper_imx290.cpp',
    '../cam_helper/cam_helper_imx296.cpp',
    '../cam_helper/cam_helper_imx415.cpp',
    '../cam_helper/cam_helper_imx477.cpp',
    '../cam_helper/cam_helper_imx519.cpp',
    '../cam_helper/cam_helper_imx708.cpp',
    '../cam_helper/cam_helper_ov64a40.cpp',
    '../cam_helper/cam_helper_ov7251.cpp',
    '../cam_helper/cam_helper_ov9281.cpp',
    '../cam_helper/md_parser_smia.cpp',
    '../controller/algorithm.cpp',
    '../controller/controller.cpp',
    '../controller/device_status.cpp',
    '../controller/histogram.cpp',
    '../controller/rpi/af.cpp',
    '../controller/rpi/agc.cpp',
    '../controller/rpi/agc_channel.cpp',
    '../controller/rpi/alsc.cpp',
    '../controller/rpi/awb.cpp',
    '../controller/rpi/black_level.cpp',
    '../controller/rpi/cac.cpp',
    '../controller/rpi/ccm.cpp',
    '../controller/rpi/contrast.cpp',
    '../controller/rpi/denoise.cpp',
    '../controller/rpi/dpc.cpp',
    '../controller/rpi/geq.cpp',
    '../controller/rpi/hdr.cpp',
    '../controller/rpi/lux.cpp',
    '../controller/rpi/noise.cpp',
    '../controller/rpi/saturation.cpp',
    '../controller/rpi/sdn.cpp',
    '../controller/rpi/sharpen.cpp',
    '../controller/rpi/tonemap.cpp',

])

rpi_ipa_common_includes = [
    include_directories('..'),
    include_directories('../controller/'),

]

rpi_ipa_common_includes += rpi_ipa_cam_helper_includes
