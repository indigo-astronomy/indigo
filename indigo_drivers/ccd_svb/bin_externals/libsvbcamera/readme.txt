日期：2019-7-24
增加支持图像参数调节(白平衡，对比度，锐度，饱和度，Gamma)
支持black level offset调节
修正设置ROI卡顿的问题
支持输出RGB24图像格式
修正曝光时间太短的问题
修正图像错误帧的问题

Date: 2019-7-24
Added support for image parameter adjustment (white balance, contrast, sharpness, saturation, gamma)
Support black level offset adjustment
Fix the problem of setting ROI stuck
Support output RGB24 image format
Fix the problem that the exposure time is too short
Fix image error frame problem

########################################################
日期：2020-7-8
增加接口获取sensor像素大小
增加pulse guide功能

Date: 2020-7-8
Increase the interface to get the sensor pixel size
Add pulse guide function


##########################################################
日期: 2020-8-11
修正设置多次曝光时间后不出帧的问题

Date: 2020-8-11
Fixed the problem of no frame after setting multiple exposure time


##########################################################
日期：2021-5-8
支持SV305M相机

Date：2021-5-8
Support SV305M camera


##########################################################
日期：2021-6-2
修正一健白平衡后，调整白平衡参数失效的问题

Date: 2021-6-2
Fixed the bug that the adjustment of white balance parameters failed after one-key white balance


##########################################################
版本：v1.4.0
日期：2021-7-3
1. 修改SV305 PRO的增益范围为0-720
2. SV305 PRO支持SVB_FLIP

Version: v1.4.0
Date: 2021-7-3
1. Modify the gain range of SV305 PRO to 0-720
2. SV305 PRO supports SVB_FLIP


##########################################################
版本: v1.4.1
日期：2021-7-7
1. bug修复

Version: v1.4.1
Date: 2021-7-7
1. Bug fixes


##########################################################
版本：v1.4.2
日期：2021-7-27
1. 修正软触发模式后再修改帧速造成获取图像帧失败的问题

Version: v1.4.2
Date: 2021-7-27
1. Modify the frame rate after the soft trigger mode, which causes the failure to obtain the image frame


##########################################################
版本: v1.5.0
日期: 2021-9-16
1. 支持新款相机SV405
2. 新增加接口函数SVBGetCameraPropertyEx
3. 增加温度控制指令

Version: v1.5.0
Date: 2021-9-16
1. Support the new camera SV405
2. Newly added interface function SVBGetCameraPropertyEx
3. Add temperature control instructions


##########################################################
版本:v1.6.2
日期:2021-11-22
1.修复SV405C的一些问题
2.支持SV905C相机

Version: v1.6.2
Date: 2021-11-22
1. Fix some problems of SV405C
2. Support SV905C camera


##########################################################
版本:v1.6.5
日期:2022-02-21
1.修复SV405CC的取帧超时和辉光问题

Version: v1.6.5
Date:2022-02-21
1. Fix the frame fetch timeout and amp glow issues of SV405CC


##########################################################
版本:v1.7.3
日期:2022-6-14
1.修正SV405CC温度控制不稳定的问题
2.修正SV405CC软触发第一帧超时问题
3.修改SV405CC最大增益为57dB,增加HCG增益控制

Version: v1.7.3
Date:2022-6-14
1. Fixed the problem of unstable temperature control of SV405CC
2. Fixed the first frame timeout problem of SV405CC soft trigger
3. Modify the maximum gain of SV405CC to 57dB, and add the HCG gain control


##########################################################
版本:v1.7.3
日期:2022-6-14
1.修正SV405CC温度控制不稳定的问题
2.修正SV405CC软触发第一帧超时问题
3.修改SV405CC最大增益为57dB,增加HCG增益控制

Version: v1.7.3
Date:2022-6-14
1. Fixed the problem of unstable temperature control of SV405CC
2. Fixed the first frame timeout problem of SV405CC soft trigger
3. Modify the maximum gain of SV405CC to 57dB, and add the HCG gain control


##########################################################
版本:v1.8.1
日期:2022-7-6
1.优化SV405CC的满阱
2.优化SV505C的性能

Version: v1.8.1
Date:2022-7-6
1. Optimize the full well of SV405CC
2. Optimize the performance of SV505C


##########################################################
版本:v1.9.0
日期:2022-7-20
1.支持SV505C、SV605CC和SV705C机型

Version: v1.9.0
Date:2022-7-20
1. Support SV505C, SV605CC and SV705C


##########################################################
版本:v1.9.1
日期:2022-8-2
1.修正一些BUG

Version: v1.9.1
Date:2022-8-2
1. Fix some bugs


##########################################################
版本:v1.9.2
日期:2022-8-10
1.修正一些BUG

Version: v1.9.2
Date:2022-8-10
1. Fix some bugs


##########################################################
版本:v1.9.3
日期:2022-8-13
1.修正SV505C和SV705C软触发不出图的BUG

Version: v1.9.3
Date:2022-8-13
1. Fixed the bug that the soft trigger of SV505C and SV705C did not output the image


##########################################################
版本:v1.9.4
日期:2022-8-17
1.关闭syslog输出

Version: v1.9.4
Date:2022-8-17
1. Disable output syslog


##########################################################
版本:v1.9.6
日期:2022-9-28
1.修正SV705C在RAW8的低分辨率下不了图的BUG
2.修正使用sharpcap时无法通过DirectShow多次打开相机的BUG

Version: v1.9.6
Date:2022-9-28
1. Fixed the bug that the SV705C could not output the image at the low resolution of RAW8
2. Fixed the bug that the camera could not be opened multiple times through DirectShow when using sharpcap


##########################################################
版本:v1.9.7
日期:2022-11-03
1.修正SV405CC偶尔失去响应的BUG(相机固件需要升级到v2.0.0.6)
2.修正SV405CC无法设置最大分辨率的BUG

Version: v1.9.7
Date: 2022-11-03
1. Fixed the bug that SV405CC occasionally loses response(Camera firmware needs to be upgraded to v2.0.0.6)
2. Fixed the bug that SV405CC cannot set the maximum resolution


##########################################################
版本:v1.9.8
日期:2022-11-09
1.修正SV405CC设置某些分辨率图像不正确的BUG

Version:v1.9.8
Date: 2022-11-09
1.Fixed the bug that SV405CC set some resolution images incorrectly


##########################################################
版本:v1.10.0
日期:2022-12-07
1.支持SV605MC相机
2.修正SV605CC触发拍照时间的问题
3.修正一些BUG
4.增加新的API接口函数SVBGetCameraFirmwareVersion和SVBIsCameraNeedToUpgrade

Version: v1.10.0
Date: 2022-12-07
1. Support SV605MC camera
2. Fixed the issue that SV605CC triggers the photo time
3. Fix some bugs
4. Add new API interface functions SVBGetCameraFirmwareVersion and SVBIsCameraNeedToUpgrade


##########################################################
版本:v1.10.1
日期:2023-02-10
1.新增动态坏点自动修正的开关
2.增加API接口函数SVBRestoreDefaultParam，可恢复相机默认参数

Version: v1.10.1
Date: 2023-02-10
1. Added a switch for automatic correction of dynamic dead pixels
2. Add the API interface function SVBRestoreDefaultParam, which can restore the default parameters of the camera



##########################################################
版本:v1.10.2
日期:2023-02-23
1.解决SV405CC软触发不出图的问题
2.Linux和MacOS上导出SVBRestoreDefaultParam
3.MacOS下的libSVBCameraSDK.dylib设置rpath

Version: v1.10.2
Date: 2023-02-23
1. Solve the problem that SV405CC soft trigger does not plot
Export SVBRestoreDefaultParam on Linux and MacOS
3.libSVBCameraSDK.dylib under MacOS sets rpath


##########################################################
版本:v1.11.0
日期:2023-03-03
1.支持相机SV305C
2.修正黑白相机在16位模式时图像的第一行和最后一行不正确的问题

Version: v1.11.0
Date: 2023-03-03
1. Support camera SV305C
2. Fix the problem that the first line and the last line of the image are incorrect when the mono camera is in 16-bit mode


##########################################################
版本:v1.11.1
日期:2023-03-27
1.SV305C支持饱和度参数
2.提高SV305C在小ROI时的帧率

Version: v1.11.1
Date: 2023-03-27
1. SV305C supports saturation
2. Improve the frame rate of SV305C in small ROI


##########################################################
版本:v1.11.2
日期:2023-04-23
1.支持图像翻转参数

Version: v1.11.2
Date: 2023-03-27
1. Support image flip


##########################################################
版本:v1.11.3
日期:2023-05-12
1.修改SV605CC的增益

Version: v1.11.3
Date: 2023-05-12
1. Modify SV605CC gain setting


##########################################################
版本:v1.11.4
日期:2023-06-03
1.修改SV605CC/SV605MC的增益设置
2.修正SV905C长曝光时无法取到图像的问题
3.移除MacOS中的x86版本库
4.修正软触发模式下可能会读取到上一次曝光图像的问题

Version: v1.11.4
Date: 2023-06-03
1. Modify the gain setting of SV605CC/SV605MC
2. Fix up the problem that the image cannot be obtained when the SV905C is exposed for a long time
3. Remove the x86 version library in MacOS
4. Fix up the problem that the last exposure image may be read in soft trigger mode


##########################################################
版本:v1.11.5
日期:2023-09-26
1.修正SV505C和SV705C在触发模式下偶尔拍照时间过长的问题
2.增加API函数SVBSetROIFormatEx和SVBGetROIFormatEx，支持设置bin average模式

Version: v1.11.5
Date:2023-09-26
1. Fixed the issue where SV505C and SV705C occasionally take too long to take pictures in trigger mode.
2. Add API functions SVBSetROIFormatEx and SVBGetROIFormatEx to support setting bin average mode


##########################################################
版本:v1.12.0
日期:2023-10-23
1.支持SC432M相机
2.增加SVB_BAD_PIXEL_CORRECTION_THRESHOLD参数，可以设置坏点修正的阈值大小，与周围像素差值超过阈值大小就会动态修正。
  SVB_BAD_PIXEL_CORRECTION_ENABLE参数开启后才生效

Version: v1.12.0
Date:2023-10-23
1.Support SC432M camera
2. Add the SVB_BAD_PIXEL_CORRECTION_THRESHOLD parameter to set the threshold for bad pixel correction. 
   If the difference with surrounding pixels exceeds the threshold, it will be dynamically corrected. 
   It takes effect only when the SVB_BAD_PIXEL_CORRECTION_ENABLE parameter is turned on
   
   
##########################################################
版本:v1.12.1
日期:2023-11-30
1. 制冷相机在设置某些参数时不再关闭制冷和风扇

Version: v1.12.1
Date:2023-11-30
1. The cooling camera will no longer turn off cooling and fan when certain parameters are set


##########################################################
版本:v1.12.2
日期:2023-12-28
1. 优化SC432M相机的长曝光设置

Version: v1.12.2
Date:2023-12-28
1. Optimizing the Long Exposure Setting of SC432M Camera


##########################################################
版本:v1.12.4
日期:2024-02-24
1. 优化SC432M相机软件
2. 解决相机从长曝光切换为短曝光后制冷短暂停止的问题

Version:v1.12.4
Date:2024-02-24
1. Optimize SC432M camera software
2. Solve the problem of cooling stopping briefly after the camera switches from long exposure to short exposure.


##########################################################
版本:v1.12.6
日期:2024-04-16
1. 修正SV905C小分辩率偏移不生效的问题
2. 修正在MONO16且锐度大于0时亮度翻转的问题
3. 修正BIN3和BIN4时图像全黑的问题

Version: v1.12.6
Date:2024-04-16
1. Fixed the problem that SV905C small resolution offset does not take effect
2. Fixed the problem of brightness flipping when MONO16 and sharpness is greater than 0
3. Fixed the problem that the image is completely black in BIN3 and BIN4


##########################################################
版本:v1.12.7
日期:2024-05-06
1. 修正macOS下程序偶尔异常退出的问题
2. 修正关闭制冷时获取的制冷功率值大于0的问题
3. SVB_CAMERA_PROPERTY中的MaxBitDepth返回sensor实际的位宽

Version: v1.12.7
Date:2024-05-06
1. Fixed the problem that the program occasionally exits abnormally under macOS
2. Fixed the issue where the cooling power value obtained when cooling is turned off is greater than 0
3. MaxBitDepth in SVB_CAMERA_PROPERTY returns the actual bit width of the sensor


##########################################################
版本:v1.13.0
日期:2024-09-29
1. 支持新相机SV905C2
2. 支持新相机SV305C Pro

Version: v1.13.0
Date:2024-05-06
1. Support new camera SV905C2
2. Support new camera SV305C Pro
 

##########################################################
版本:v1.13.1
日期:2024-10-29
1. 修正ASCOM软件支持SV905C2和SV305C Pro相机
2. 修正SV905C2的曝光时间

Version: v1.13.1
Date: 2024-10-29
1. Fixup ASCOM software to support SV905C2 and SV305C Pro cameras
2. Fixup exposure time of SV905C2


##########################################################
版本:v1.13.2
日期:2024-11-28
1. 优化SV905C2的增益设置

Version: v1.13.2
Date: 2024-11-28
1. Optimize gain settings of SV905C2


##########################################################
版本:v1.13.3
日期:2025-01-21
1. 修正SV305CPRO相机长曝光取图超时的问题

Version: v1.13.3
Date: 2025-01-21
1. Fixed the issue of long exposure image capture timeout for SV305CPRO camera


##########################################################
版本:v1.13.4
日期:2025-02-05
1. 修正SV305MPRO在v1.13.0及更新版本无法正常打开的问题

Version: v1.13.4
Date: 2025-02-05
1. Fixed the problem that SV305MPRO cannot be opened normally in v1.13.0 and later versions
