# SPDX-License-Identifier: CC0-1.0

libcamera_base_public_sources = files([
    'bound_method.cpp',
    'class.cpp',
    'flags.cpp',
    'object.cpp',
    'shared_fd.cpp',
    'signal.cpp',
    'unique_fd.cpp',
])

libcamera_base_internal_sources = files([
    'backtrace.cpp',
    'event_dispatcher.cpp',
    'event_dispatcher_poll.cpp',
    'event_notifier.cpp',
    'file.cpp',
    'log.cpp',
    'memfd.cpp',
    'message.cpp',
    'mutex.cpp',
    'semaphore.cpp',
    'thread.cpp',
    'timer.cpp',
    'utils.cpp',
])

libdw = dependency('libdw', required : false)
libunwind = dependency('libunwind', required : false)

if cc.has_header_symbol('execinfo.h', 'backtrace')
    config_h.set('HAVE_BACKTRACE', 0)
endif

if libdw.found()
    config_h.set('HAVE_DW', 0)
endif

if libunwind.found()
    config_h.set('HAVE_UNWIND', 0)
endif

libcamera_base_deps = [
    libatomic,
    libdw,
    libthreads,
    libunwind,
]

# Internal components must use the libcamera_base_private dependency to enable
# the use of headers which must not be exposed to the libcamera public api.
libcamera_base_args = [ '-DLIBCAMERA_BASE_PRIVATE' ]

