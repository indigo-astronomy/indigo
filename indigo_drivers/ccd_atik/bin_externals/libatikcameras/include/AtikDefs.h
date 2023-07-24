#ifndef ATIK_DEFS_H
#define ATIK_DEFS_H

/// @file AtikDefs.h
///	@brief Atik SDK structure definitions.

#ifdef __cplusplus
extern "C" 
{
#endif

    ///
    /// @brief Structure of the data, a pointer to which is, passed to
    /// the info parameter of FastCallbackEx function set by a call to
    /// ArtemisSetFastCallbackEx.
    /// 
    /// @see ArtemisSetFastCallbackEx
    ///
    struct FastCallbackInfo_
    {
        /// Set to the size of this structure minus 1.
        unsigned char size;

        /// Number of dropped frames since the previous callback (softDroppedFrames + hardDroppedFrames).
        unsigned char droppedFrames;

        /// Number of dropped frames in software since the previous callback.
        unsigned char softDroppedFrames;

        /// Number of dropped frames in camera since the previous callback.
        unsigned char hardDroppedFrames;

        /// Start of exposure year.
        int exposureStartTimeYear;

        /// Start of exposure month. 
        int exposureStartTimeMonth;

        /// Start of exposure day.
        int exposureStartTimeDay;

        /// Start of exposure hour (24 hour clock format) (UTC).
        int exposureStartTimeHour;

        /// Start of exposure minute (UTC).
        int exposureStartTimeMinute;

        /// Start of exposure second (UTC).
        int exposureStartTimeSecond;

        /// Start of exposure millisecond (UTC).
        int exposureStartTimeMS;

        /// Total number of exposures since the last ArtemisStartFastExposure() call. Includes dropped frames.
        int exposureNumber;

        /// The average image data transfer rate in MB/s.
        double dataRateMBps;
    };
    typedef struct FastCallbackInfo_ FastCallbackInfo;

#ifdef __cplusplus
}
#endif

#endif // ATIK_DEFS_H
