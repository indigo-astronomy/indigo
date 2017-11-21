// Copyright (c) 2017 Rumen G.Bogdanovski.
// All rights reserved.
//
// Based on PyDome code
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO dome azimuth solver
 \file indigo_dome_azimuth.h
 */

 #ifndef indigo_dome_azimuth_h
 #define indigo_dome_azimuth_h

 #ifdef __cplusplus
 extern "C" {
 #endif

extern double indigo_dome_solve_azimuth (
	double ha,
	double dec,
	double site_latitude,
	double dome_radius,
	double mount_dec_height,
	double mount_dec_length,
	double mount_dec_offset
);

#ifdef __cplusplus
}
#endif

#endif /* indigo_dome_azimuth_h */
