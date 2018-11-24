This implementation uses a library of CMU 1394 Digital Camera Library.
http://www.cs.cmu.edu/~iwan/1394/

This implementation is tested on MinGW (mingw-get-inst-20110802.exe)

Known limitations:

- a lot of functionalities are untested. Please participate in the
  effort and report bugs on the mailing list libdc1394-devel@lists.sf.net.

- the timestamps normally available in each dc1394video_frame_t are
  NULL on windows. The CMU driver does not return this information
  yet, because they have problem to get it from the Windows kernel
  (google/grep for DESCRIPTOR_TIME_STAMP_ON_COMPLETION).

