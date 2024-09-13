// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>
#include <signal.h>
#include <dns_sd.h>
#include <libgen.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef INDIGO_LINUX
#include <sys/prctl.h>
#endif
#ifdef INDIGO_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_token.h>
#include <indigo/indigo_align.h>
#include <indigo/indigocat/indigocat_star.h>
#include <indigo/indigocat/indigocat_dso.h>
#include <indigo/indigocat/indigocat_ss.h>

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "mount_simulator/indigo_mount_simulator.h"
#include "gps_simulator/indigo_gps_simulator.h"
#include "dome_simulator/indigo_dome_simulator.h"
#ifdef STATIC_DRIVERS
#include "ccd_sx/indigo_ccd_sx.h"
#include "wheel_sx/indigo_wheel_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
#include "guider_asi/indigo_guider_asi.h"
#include "wheel_asi/indigo_wheel_asi.h"
#include "ccd_atik/indigo_ccd_atik.h"
#include "wheel_atik/indigo_wheel_atik.h"
#include "ccd_qhy2/indigo_ccd_qhy.h"
#include "focuser_fcusb/indigo_focuser_fcusb.h"
#include "ccd_iidc/indigo_ccd_iidc.h"
#include "mount_lx200/indigo_mount_lx200.h"
#include "mount_synscan/indigo_mount_synscan.h"
#include "mount_nexstar/indigo_mount_nexstar.h"
#include "mount_nexstaraux/indigo_mount_nexstaraux.h"
#include "mount_temma/indigo_mount_temma.h"
#include "ccd_fli/indigo_ccd_fli.h"
#include "wheel_fli/indigo_wheel_fli.h"
#include "focuser_fli/indigo_focuser_fli.h"
#include "ccd_apogee/indigo_ccd_apogee.h"
#include "focuser_usbv3/indigo_focuser_usbv3.h"
#include "focuser_wemacro/indigo_focuser_wemacro.h"
#include "ccd_mi/indigo_ccd_mi.h"
#include "aux_joystick/indigo_aux_joystick.h"
#include "mount_synscan/indigo_mount_synscan.h"
#include "mount_ioptron/indigo_mount_ioptron.h"
#include "guider_cgusbst4/indigo_guider_cgusbst4.h"
#include "wheel_quantum/indigo_wheel_quantum.h"
#include "wheel_trutek/indigo_wheel_trutek.h"
#include "wheel_xagyl/indigo_wheel_xagyl.h"
#include "wheel_optec/indigo_wheel_optec.h"
#include "focuser_dmfc/indigo_focuser_dmfc.h"
#include "focuser_nstep/indigo_focuser_nstep.h"
#include "focuser_nfocus/indigo_focuser_nfocus.h"
#include "focuser_moonlite/indigo_focuser_moonlite.h"
#include "focuser_mjkzz/indigo_focuser_mjkzz.h"
#include "ccd_touptek/indigo_ccd_touptek.h"
#include "ccd_altair/indigo_ccd_altair.h"
#include "focuser_optec/indigo_focuser_optec.h"
#include "aux_ppb/indigo_aux_ppb.h"
#include "aux_upb/indigo_aux_upb.h"
#include "aux_upb3/indigo_aux_upb3.h"
#include "aux_rts/indigo_aux_rts.h"
#include "aux_dsusb/indigo_aux_dsusb.h"
#include "guider_gpusb/indigo_guider_gpusb.h"
#include "focuser_lakeside/indigo_focuser_lakeside.h"
#include "agent_imager/indigo_agent_imager.h"
#include "focuser_asi/indigo_focuser_asi.h"
#include "agent_alignment/indigo_agent_alignment.h"
#include "agent_mount/indigo_agent_mount.h"
#include "ao_sx/indigo_ao_sx.h"
#include "ccd_uvc/indigo_ccd_uvc.h"
#include "agent_guider/indigo_agent_guider.h"
#include "system_ascol/indigo_system_ascol.h"
#include "focuser_steeldrive2/indigo_focuser_steeldrive2.h"
#include "agent_auxiliary/indigo_agent_auxiliary.h"
#include "aux_sqm/indigo_aux_sqm.h"
#include "focuser_dsd/indigo_focuser_dsd.h"
#include "ccd_ptp/indigo_ccd_ptp.h"
#include "focuser_efa/indigo_focuser_efa.h"
#include "aux_arteskyflat/indigo_aux_arteskyflat.h"
#include "aux_flipflat/indigo_aux_flipflat.h"
#include "dome_nexdome/indigo_dome_nexdome.h"
#include "dome_nexdome3/indigo_dome_nexdome3.h"
#include "aux_usbdp/indigo_aux_usbdp.h"
#include "aux_flatmaster/indigo_aux_flatmaster.h"
#include "focuser_focusdreampro/indigo_focuser_focusdreampro.h"
#include "aux_fbc/indigo_aux_fbc.h"
#include "mount_rainbow/indigo_mount_rainbow.h"
#include "gps_gpsd/indigo_gps_gpsd.h"
#include "focuser_lunatico/indigo_focuser_lunatico.h"
#include "rotator_simulator/indigo_rotator_simulator.h"
#include "rotator_lunatico/indigo_rotator_lunatico.h"
#include "aux_dragonfly/indigo_aux_dragonfly.h"
#include "dome_dragonfly/indigo_dome_dragonfly.h"
#include "aux_cloudwatcher/indigo_aux_cloudwatcher.h"
#include "wheel_manual/indigo_wheel_manual.h"
#include "aux_mgbox/indigo_aux_mgbox.h"
#include "dome_baader/indigo_dome_baader.h"
#include "mount_pmc8/indigo_mount_pmc8.h"
#include "focuser_robofocus/indigo_focuser_robofocus.h"
#include "wheel_qhy/indigo_wheel_qhy.h"
#include "focuser_mypro2/indigo_focuser_mypro2.h"
#include "agent_astrometry/indigo_agent_astrometry.h"
#include "dome_skyroof/indigo_dome_skyroof.h"
#include "aux_skyalert/indigo_aux_skyalert.h"
#include "agent_alpaca/indigo_agent_alpaca.h"
#include "dome_talon6ror/indigo_dome_talon6ror.h"
#include "dome_beaver/indigo_dome_beaver.h"
#include "focuser_astromechanics/indigo_focuser_astromechanics.h"
#include "aux_astromechanics/indigo_aux_astromechanics.h"
#include "aux_geoptikflat/indigo_aux_geoptikflat.h"
#include "ccd_svb/indigo_ccd_svb.h"
#include "agent_astap/indigo_agent_astap.h"
#include "rotator_optec/indigo_rotator_optec.h"
#include "mount_starbook/indigo_mount_starbook.h"
#include "ccd_playerone/indigo_ccd_playerone.h"
#include "focuser_prodigy/indigo_focuser_prodigy.h"
#include "agent_config/indigo_agent_config.h"
#include "mount_asi/indigo_mount_asi.h"
#include "wheel_indigo/indigo_wheel_indigo.h"
#include "rotator_falcon/indigo_rotator_falcon.h"
#include "wheel_playerone/indigo_wheel_playerone.h"
#include "ccd_omegonpro/indigo_ccd_omegonpro.h"
#include "ccd_ssg/indigo_ccd_ssg.h"
#include "ccd_rising/indigo_ccd_rising.h"
#include "ccd_ogma/indigo_ccd_ogma.h"
#include "focuser_primaluce/indigo_focuser_primaluce.h"
#include "aux_uch/indigo_aux_uch.h"
#include "aux_wbplusv3/indigo_aux_wbplusv3.h"
#include "aux_wbprov3/indigo_aux_wbprov3.h"
#include "aux_wcv4ec/indigo_aux_wcv4ec.h"
#include "focuser_ioptron/indigo_focuser_ioptron.h"
#include "ccd_bresser/indigo_ccd_bresser.h"
#include "focuser_optecfl/indigo_focuser_optecfl.h"
#include "focuser_fc3/indigo_focuser_fc3.h"
#include "focuser_lacerta/indigo_focuser_lacerta.h"
#include "ccd_pentax/indigo_ccd_pentax.h"
#ifndef __aarch64__
#include "ccd_sbig/indigo_ccd_sbig.h"
#endif
#include "ccd_dsi/indigo_ccd_dsi.h"
#include "ccd_qsi/indigo_ccd_qsi.h"
#include "gps_nmea/indigo_gps_nmea.h"
#ifdef INDIGO_MACOS
#include "ccd_ica/indigo_ccd_ica.h"
#include "guider_eqmac/indigo_guider_eqmac.h"
#include "focuser_wemacro_bt/indigo_focuser_wemacro_bt.h"
#include "focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.h"
#endif
#ifdef INDIGO_LINUX
#include "ccd_gphoto2/indigo_ccd_gphoto2.h"
#endif
#include "agent_snoop/indigo_agent_snoop.h"
#include "agent_lx200_server/indigo_agent_lx200_server.h"
#include "agent_scripting/indigo_agent_scripting.h"
#endif

#define SERVER_NAME         "INDIGO Server"

driver_entry_point static_drivers[] = {
#ifdef STATIC_DRIVERS
	indigo_agent_alignment,
	indigo_agent_alpaca,
	indigo_agent_astrometry,
	indigo_agent_astap,
	indigo_agent_auxiliary,
	indigo_agent_guider,
	indigo_agent_imager,
	indigo_agent_lx200_server,
	indigo_agent_config,
	indigo_agent_scripting,
	indigo_agent_mount,
	indigo_agent_snoop,
	indigo_ao_sx,
	indigo_aux_arteskyflat,
	indigo_aux_astromechanics,
	indigo_aux_cloudwatcher,
	indigo_aux_dragonfly,
	indigo_aux_dsusb,
	indigo_aux_fbc,
	indigo_aux_flatmaster,
	indigo_aux_flipflat,
	indigo_aux_geoptikflat,
	indigo_aux_joystick,
	indigo_aux_mgbox,
	indigo_aux_ppb,
	indigo_aux_rts,
	indigo_aux_skyalert,
	indigo_aux_sqm,
	indigo_aux_uch,
	indigo_aux_upb,
	indigo_aux_upb3,
	indigo_aux_usbdp,
	indigo_aux_wbplusv3,
	indigo_aux_wbprov3,
	indigo_aux_wcv4ec,
	indigo_ccd_altair,
	indigo_ccd_apogee,
	indigo_ccd_asi,
	indigo_ccd_atik,
	indigo_ccd_bresser,
	indigo_ccd_dsi,
	indigo_ccd_fli,
#ifdef INDIGO_LINUX
	indigo_ccd_gphoto2,
#endif
#ifdef INDIGO_MACOS
	indigo_ccd_ica,
#endif
	indigo_ccd_iidc,
	indigo_ccd_mi,
	indigo_ccd_ogma,
	indigo_ccd_omegonpro,
	indigo_ccd_pentax,
	indigo_ccd_playerone,
	indigo_ccd_ptp,
	indigo_ccd_qhy2,
	indigo_ccd_qsi,
	indigo_ccd_rising,
#ifndef __aarch64__
	indigo_ccd_sbig,
#endif
	indigo_ccd_simulator,
	indigo_ccd_ssag,
	indigo_ccd_ssg,
	indigo_ccd_svb,
	indigo_ccd_sx,
	indigo_ccd_touptek,
	indigo_ccd_uvc,
	indigo_dome_baader,
	indigo_dome_beaver,
	indigo_dome_dragonfly,
	indigo_dome_nexdome,
	indigo_dome_nexdome3,
	indigo_dome_simulator,
	indigo_dome_skyroof,
	indigo_dome_talon6ror,
	indigo_focuser_asi,
	indigo_focuser_astromechanics,
	indigo_focuser_dmfc,
	indigo_focuser_dsd,
	indigo_focuser_efa,
	indigo_focuser_fcusb,
	indigo_focuser_fc3,
	indigo_focuser_focusdreampro,
	indigo_focuser_fli,
	indigo_focuser_ioptron,
	indigo_focuser_lacerta,
	indigo_focuser_lakeside,
	indigo_focuser_lunatico,
	indigo_focuser_mjkzz,
#ifdef INDIGO_MACOS
	indigo_focuser_mjkzz_bt,
#endif
	indigo_focuser_moonlite,
	indigo_focuser_mypro2,
	indigo_focuser_nfocus,
	indigo_focuser_nstep,
	indigo_focuser_optec,
	indigo_focuser_optecfl,
	indigo_focuser_primaluce,
	indigo_focuser_prodigy,
	indigo_focuser_robofocus,
	indigo_focuser_usbv3,
	indigo_focuser_steeldrive2,
	indigo_focuser_wemacro,
#ifdef INDIGO_MACOS
	indigo_focuser_wemacro_bt,
#endif
	indigo_gps_gpsd,
	indigo_gps_nmea,
	indigo_gps_simulator,
	indigo_guider_asi,
	indigo_guider_cgusbst4,
#ifdef INDIGO_MACOS
	indigo_guider_eqmac,
#endif
	indigo_guider_gpusb,
	indigo_mount_asi,
	indigo_mount_ioptron,
	indigo_mount_lx200,
	indigo_mount_nexstar,
	indigo_mount_nexstaraux,
	indigo_mount_pmc8,
	indigo_mount_rainbow,
	indigo_mount_simulator,
	indigo_mount_starbook,
	indigo_mount_synscan,
	indigo_mount_temma,
	indigo_rotator_falcon,
	indigo_rotator_lunatico,
	indigo_rotator_optec,
	indigo_rotator_simulator,
	indigo_wheel_asi,
	indigo_wheel_atik,
	indigo_wheel_fli,
	indigo_wheel_indigo,
	indigo_wheel_manual,
	indigo_wheel_optec,
	indigo_wheel_playerone,
	indigo_wheel_qhy,
	indigo_wheel_quantum,
	indigo_wheel_sx,
	indigo_wheel_trutek,
	indigo_wheel_xagyl,
	indigo_system_ascol,
#endif
	NULL
};

static struct {
	char *name;
	char *description;
} dynamic_drivers[INDIGO_MAX_DRIVERS] = {
	NULL
};

static int static_drivers_count = 0;
static int dynamic_drivers_count = 0;
static bool command_line_drivers = false;

static indigo_property *info_property;
static indigo_property *drivers_property;
static indigo_property *servers_property;
static indigo_property *load_property;
static indigo_property *unload_property;
static indigo_property *restart_property;
static indigo_property *log_level_property;
static indigo_property *blob_buffering_property;
static indigo_property *blob_proxy_property;
static indigo_property *server_features_property;

#ifdef RPI_MANAGEMENT
static indigo_property *wifi_country_code_property;
static indigo_property *wifi_ap_property;
static indigo_property *wifi_infrastructure_property;
static indigo_property *wifi_channel_property;
static indigo_property *internet_sharing_property;
static indigo_property *host_time_property;
static indigo_property *shutdown_property;
static indigo_property *reboot_property;
static indigo_property *install_property;
static pthread_mutex_t install_property_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *star_data = NULL;
static void *dso_data = NULL;
static void *constellation_data = NULL;

#ifdef INDIGO_MACOS
static bool runLoop = true;
#endif

#define SERVER_INFO_PROPERTY											info_property
#define SERVER_INFO_VERSION_ITEM									(SERVER_INFO_PROPERTY->items + 0)
#define SERVER_INFO_SERVICE_ITEM									(SERVER_INFO_PROPERTY->items + 1)

#define SERVER_DRIVERS_PROPERTY										drivers_property

#define SERVER_SERVERS_PROPERTY										servers_property

#define SERVER_LOAD_PROPERTY											load_property
#define SERVER_LOAD_ITEM													(SERVER_LOAD_PROPERTY->items + 0)

#define SERVER_UNLOAD_PROPERTY										unload_property
#define SERVER_UNLOAD_ITEM												(SERVER_UNLOAD_PROPERTY->items + 0)

#define SERVER_RESTART_PROPERTY										restart_property
#define SERVER_RESTART_ITEM												(SERVER_RESTART_PROPERTY->items + 0)

#define SERVER_LOG_LEVEL_PROPERTY									log_level_property
#define SERVER_LOG_LEVEL_ERROR_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 0)
#define SERVER_LOG_LEVEL_INFO_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 1)
#define SERVER_LOG_LEVEL_DEBUG_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 2)
#define SERVER_LOG_LEVEL_TRACE_BUS_ITEM						(SERVER_LOG_LEVEL_PROPERTY->items + 3)
#define SERVER_LOG_LEVEL_TRACE_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 4)

#define SERVER_BLOB_BUFFERING_PROPERTY						blob_buffering_property
#define SERVER_BLOB_BUFFERING_DISABLED_ITEM				(SERVER_BLOB_BUFFERING_PROPERTY->items + 0)
#define SERVER_BLOB_BUFFERING_ENABLED_ITEM				(SERVER_BLOB_BUFFERING_PROPERTY->items + 1)
#define SERVER_BLOB_COMPRESSION_ENABLED_ITEM			(SERVER_BLOB_BUFFERING_PROPERTY->items + 2)

#define SERVER_BLOB_PROXY_PROPERTY								blob_proxy_property
#define SERVER_BLOB_PROXY_DISABLED_ITEM						(SERVER_BLOB_PROXY_PROPERTY->items + 0)
#define SERVER_BLOB_PROXY_ENABLED_ITEM						(SERVER_BLOB_PROXY_PROPERTY->items + 1)

#define SERVER_FEATURES_PROPERTY									server_features_property
#define SERVER_BONJOUR_ITEM												(SERVER_FEATURES_PROPERTY->items + 0)
#define SERVER_CTRL_PANEL_ITEM										(SERVER_FEATURES_PROPERTY->items + 1)
#define SERVER_WEB_APPS_ITEM											(SERVER_FEATURES_PROPERTY->items + 2)

#define SERVER_WIFI_AP_PROPERTY										wifi_ap_property
#define SERVER_WIFI_AP_SSID_ITEM									(SERVER_WIFI_AP_PROPERTY->items + 0)
#define SERVER_WIFI_AP_PASSWORD_ITEM							(SERVER_WIFI_AP_PROPERTY->items + 1)

#define SERVER_WIFI_INFRASTRUCTURE_PROPERTY				wifi_infrastructure_property
#define SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM			(SERVER_WIFI_INFRASTRUCTURE_PROPERTY->items + 0)
#define SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM	(SERVER_WIFI_INFRASTRUCTURE_PROPERTY->items + 1)

#define SERVER_WIFI_CHANNEL_PROPERTY							wifi_channel_property
#define SERVER_WIFI_CHANNEL_ITEM									(SERVER_WIFI_CHANNEL_PROPERTY->items + 0)

#define SERVER_WIFI_COUNTRY_CODE_PROPERTY				wifi_country_code_property
#define SERVER_WIFI_COUNTRY_CODE_ITEM					(SERVER_WIFI_COUNTRY_CODE_PROPERTY->items + 0)

#define SERVER_INTERNET_SHARING_PROPERTY					internet_sharing_property
#define SERVER_INTERNET_SHARING_DISABLED_ITEM			(SERVER_INTERNET_SHARING_PROPERTY->items + 0)
#define SERVER_INTERNET_SHARING_ENABLED_ITEM			(SERVER_INTERNET_SHARING_PROPERTY->items + 1)

#define SERVER_HOST_TIME_PROPERTY									host_time_property
#define SERVER_HOST_TIME_ITEM											(SERVER_HOST_TIME_PROPERTY->items + 0)

#define SERVER_SHUTDOWN_PROPERTY									shutdown_property
#define SERVER_SHUTDOWN_ITEM											(SERVER_SHUTDOWN_PROPERTY->items + 0)

#define SERVER_REBOOT_PROPERTY										reboot_property
#define SERVER_REBOOT_ITEM												(SERVER_REBOOT_PROPERTY->items + 0)

#define SERVER_INSTALL_PROPERTY										install_property


static pid_t server_pid = 0;
static bool keep_server_running = true;
static bool use_sigkill = false;
static bool use_ctrl_panel = true;
static bool use_web_apps = true;

#ifdef RPI_MANAGEMENT
static bool use_rpi_management = false;
#endif /* RPI_MANAGEMENT */

static char const *server_argv[128];
static int server_argc = 1;

static indigo_result attach(indigo_device *device);
static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result detach(indigo_device *device);

static indigo_device server_device = INDIGO_DEVICE_INITIALIZER(
	"Server",
	attach,
	enumerate_properties,
	change_property,
	NULL,
	detach
);

static double h2deg(double ra) {
	return ra > 12 ? (ra - 24) * 15 : ra * 15;
}

static void *indigo_add_star_json_resource(int max_mag) {
	int buffer_size = 1024 * 1024;
	char *buffer =  malloc(buffer_size);
	strcpy(buffer, "{\"type\":\"FeatureCollection\",\"features\": [");
	unsigned size = (unsigned)strlen(buffer);
	char *sep = "";
	indigocat_star_entry *star_data = indigocat_get_star_data();
	for (int i = 0; star_data[i].hip; i++) {
		if (star_data[i].mag > max_mag)
			continue;
		char desig[256] = "";
		char *name = "";
		if (star_data[i].name) {
			strcpy(desig, star_data[i].name);
			name = strrchr(desig, ',');
			if (name) {
				*name = 0;
				name += 2;
			} else {
				name = "";
			}
		}
		size += sprintf(buffer + size, "%s{\"type\":\"Feature\",\"id\":%d,\"properties\":{\"name\": \"%s\",\"desig\":\"%s\",\"mag\": %.2f},\"geometry\":{\"type\":\"Point\",\"coordinates\":[%.4f,%.4f]}}", sep, star_data[i].hip, name, desig, star_data[i].mag, h2deg(star_data[i].ra), star_data[i].dec);
		if (buffer_size - size < 1024) {
			buffer = indigo_safe_realloc(buffer, buffer_size *= 2);
		}
		sep = ",";
	}

	indigocat_ss_entry *ss_data = indigocat_get_ss_data();
	for (int i = 0; ss_data[i].id; i++) {
		double mag = ss_data[i].mag;
		if (mag < -4.5)
			mag = -4.5;
		size += sprintf(buffer + size, "%s{\"type\":\"Feature\",\"id\":%d,\"properties\":{\"name\": \"%s\",\"desig\": \"\",\"mag\": %.2f,\"bv\":-5},\"geometry\":{\"type\":\"Point\",\"coordinates\":[%.4f,%.4f]}}", sep, -ss_data[i].id, ss_data[i].name, mag, h2deg(ss_data[i].ra), ss_data[i].dec);
		if (buffer_size - size < 1024) {
			buffer = indigo_safe_realloc(buffer, buffer_size *= 2);
		}
	}


	size += sprintf(buffer + size, "]}");
	unsigned char *data = indigo_safe_malloc(buffer_size);
	unsigned data_size = buffer_size;
	indigo_compress("stars.json", buffer, size, data, &data_size);
	free(buffer);
	indigo_server_add_resource("/data/stars.json", data, (int)data_size, "application/json; charset=utf-8");
	return data;
}

static void *indigo_add_dso_json_resource(int max_mag) {
	int buffer_size = 1024 * 1024;
	char *buffer =  malloc(buffer_size);
	strcpy(buffer, "{\"type\":\"FeatureCollection\",\"features\": [");
	unsigned size = (unsigned)strlen(buffer);
	char *sep = "";
	indigocat_dso_entry *dso_data = indigocat_get_dso_data();
	for (int i = 0; dso_data[i].id; i++) {
		/* Filter by magnitude, but remove objects without name or mesier designation*/
		if (
			dso_data[i].mag > max_mag
			|| dso_data[i].name[0] == '\0'
			|| (dso_data[i].name[0] == 'I' && dso_data[i].name[1] == 'C')
			//|| (dso_data[i].name[0] == 'N' && dso_data[i].name[1] == 'G' && dso_data[i].name[2] == 'C')
		) continue;
		size += sprintf(buffer + size, "%s{\"type\":\"Feature\",\"id\":\"%s\",\"properties\":{\"name\": \"%s\",\"desig\": \"%s\",\"type\":\"oc\",\"mag\": %.2f},\"geometry\":{\"type\":\"Point\",\"coordinates\":[%.4f,%.4f]}}", sep, dso_data[i].id, dso_data[i].id, dso_data[i].name, dso_data[i].mag, h2deg(dso_data[i].ra), dso_data[i].dec);
		if (buffer_size - size < 1024) {
			buffer = indigo_safe_realloc(buffer, buffer_size *= 2);
		}
		sep = ",";
	}
	size += sprintf(buffer + size, "]}");
	unsigned char *data = indigo_safe_malloc(buffer_size);
	unsigned data_size = buffer_size;
	indigo_compress("stars.json", buffer, size, data, &data_size);
	free(buffer);
	indigo_server_add_resource("/data/dsos.json", data, (int)data_size, "application/json; charset=utf-8");
	return data;
}

static int add_multiline(char *buffer, ...) {
	int size = 0;
	va_list ap;
	va_start(ap, buffer);
	char *sep = "";
	indigocat_star_entry *star_data = indigocat_get_star_data();
	static char *sep2 = "";
	size += sprintf(buffer, "%s[", sep2);
	sep2 = ",";
	for (int hip = va_arg(ap, int); hip; hip = va_arg(ap, int)) {
		for (int i = 0; star_data[i].hip; i++) {
			if (star_data[i].hip == hip) {
				size += sprintf(buffer + size, "%s[%.4f,%.4f]", sep, h2deg(star_data[i].ra), star_data[i].dec);
				sep = ",";
				break;
			}
		}
	}
	size += sprintf(buffer + size, "]");
	return size;
}

static void *indigo_add_constellations_lines_json_resource() {
	int buffer_size = 1024 * 1024;
	char *buffer =  malloc(buffer_size);
	strcpy(buffer, "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"id\":\"Const\",\"properties\":{},\"geometry\":{\"type\":\"MultiLineString\",\"coordinates\":[");
	unsigned size = (unsigned)strlen(buffer);
	size += add_multiline(buffer + size, 25428, 20889, 20455, 20205, 20894, 21421, 26451, 0);
	size += add_multiline(buffer + size, 114341, 113136, 112716, 112961, 111497, 110960, 110395, 109074, 106278, 102618, 0);
	size += add_multiline(buffer + size, 78384, 76297, 75264, 74376, 74395, 0);
	size += add_multiline(buffer + size, 71860, 73273, 75141, 75177, 0);
	size += add_multiline(buffer + size, 76297, 75141, 0);
	size += add_multiline(buffer + size, 76127, 75695, 76267, 76952, 77512, 78159, 0);
	size += add_multiline(buffer + size, 93747, 97649, 98036, 99473, 97804, 95501, 93747, 0);
	size += add_multiline(buffer + size, 97278, 97649, 95501, 93805, 0);
	size += add_multiline(buffer + size, 93174, 93825, 94114, 94160, 94005, 93542, 0);
	size += add_multiline(buffer + size, 76333, 74785, 72622, 73714, 0);
	size += add_multiline(buffer + size, 93506, 93864, 92855, 92041, 90496, 89931, 90185, 89642, 0);
	size += add_multiline(buffer + size, 89931, 88635, 0);
	size += add_multiline(buffer + size, 90496, 89341, 0);
	size += add_multiline(buffer + size, 92855, 93683, 94141, 0);
	size += add_multiline(buffer + size, 93683, 93085, 0);
	size += add_multiline(buffer + size, 7083, 6867, 2081, 5165, 7083, 0);
	size += add_multiline(buffer + size, 100751, 102395, 98495, 91792, 86929, 92609, 99240, 102395, 0);
	size += add_multiline(buffer + size, 98337, 97365, 96837, 0);
	size += add_multiline(buffer + size, 97365, 96757, 0);
	size += add_multiline(buffer + size, 81852, 81065, 80047, 72370, 0);
	size += add_multiline(buffer + size, 14879, 13147, 0);
	size += add_multiline(buffer + size, 42515, 42828, 43409, 0);
	size += add_multiline(buffer + size, 19893, 21281, 26069, 0);
	size += add_multiline(buffer + size, 75323, 71908, 74824, 0);
	size += add_multiline(buffer + size, 11767, 85822, 82080, 77055, 72607, 75097, 79822, 77055, 0);
	size += add_multiline(buffer + size, 7097, 8198, 9487, 8833, 7884, 7007, 5737, 4906, 3786, 118268, 116771, 115830, 114971, 0);
	size += add_multiline(buffer + size, 8796, 10064, 10670, 8796, 0);
	size += add_multiline(buffer + size, 64241, 64394, 60742, 0);
	size += add_multiline(buffer + size, 25859, 26634, 27628, 28199, 30277, 0);
	size += add_multiline(buffer + size, 67301, 65378, 62956, 59774, 58001, 53910, 54061, 59774, 0);
	size += add_multiline(buffer + size, 58001, 57399, 54539, 50801, 0);
	size += add_multiline(buffer + size, 54061, 46733, 41704, 0);
	size += add_multiline(buffer + size, 46733, 48319, 46853, 44127, 0);
	size += add_multiline(buffer + size, 74666, 72105, 69673, 71053, 71075, 73555, 74666, 0);
	size += add_multiline(buffer + size, 67927, 69673, 0);
	size += add_multiline(buffer + size, 101772, 102333, 103227, 100751, 0);
	size += add_multiline(buffer + size, 44816, 39953, 42913, 44816, 45941, 42913, 0);
	size += add_multiline(buffer + size, 110538, 111169, 110609, 111022, 110351, 0);
	size += add_multiline(buffer + size, 63121, 61317, 0);
	size += add_multiline(buffer + size, 28360, 28380, 25428, 23015, 23179, 23416, 24608, 28360, 0);
	size += add_multiline(buffer + size, 91262, 91971, 92420, 93194, 92791, 91971, 0);
	size += add_multiline(buffer + size, 45860, 45688, 44248, 41075, 0);
	size += add_multiline(buffer + size, 90422, 90568, 0);
	size += add_multiline(buffer + size, 92946, 89962, 88404, 88048, 86263, 84012, 0);
	size += add_multiline(buffer + size, 77450, 77233, 78072, 0);
	size += add_multiline(buffer + size, 77233, 76276, 77070, 77622, 79593, 0);
	size += add_multiline(buffer + size, 17440, 19780, 19921, 18772, 18597, 17440, 0);
	size += add_multiline(buffer + size, 24436, 24674, 25930, 25336, 0);
	size += add_multiline(buffer + size, 27366, 26727, 27989, 0);
	size += add_multiline(buffer + size, 26727, 26311, 25930, 0);
	size += add_multiline(buffer + size, 111954, 113368, 113246, 112948, 111188, 0);
	size += add_multiline(buffer + size, 14328, 15863, 17358, 18532, 18246, 0);
	size += add_multiline(buffer + size, 15863, 14576, 0);
	size += add_multiline(buffer + size, 40702, 51839, 52633, 0);
	size += add_multiline(buffer + size, 82273, 77952, 76440, 74946, 82273, 0);
	size += add_multiline(buffer + size, 44066, 42911, 42806, 43100, 0);
	size += add_multiline(buffer + size, 42911, 40526, 0);
	size += add_multiline(buffer + size, 8886, 6686, 4427, 3179, 746, 0);
	size += add_multiline(buffer + size, 9236, 17678, 2021, 0);
	size += add_multiline(buffer + size, 113881, 677, 1067, 113963, 0);
	size += add_multiline(buffer + size, 107315, 109427, 112029, 112447, 113963, 113881, 112158, 0);
	size += add_multiline(buffer + size, 45556, 48002, 45238, 50099, 52419, 51576, 50371, 45556, 41037, 30438, 0);
	size += add_multiline(buffer + size, 53229, 51233, 0);
	size += add_multiline(buffer + size, 100027, 100345, 101027, 102485, 102978, 104234, 105881, 106723, 107556, 106985, 105515, 104139, 100345, 0);
	size += add_multiline(buffer + size, 9640, 5447, 3092, 677, 0);
	size += add_multiline(buffer + size, 23522, 22783, 0);
	size += add_multiline(buffer + size, 68895, 64962, 57936, 56343, 54682, 53740, 52943, 51069, 49841, 48356, 46390, 47431, 45336, 43813, 43109, 42313, 42402, 42799, 43234, 43109, 0);
	size += add_multiline(buffer + size, 24305, 25985, 27288, 28103, 0);
	size += add_multiline(buffer + size, 25985, 25606, 0);
	size += add_multiline(buffer + size, 27654, 27072, 25606, 23685, 0);
	size += add_multiline(buffer + size, 47908, 48455, 50335, 50583, 49583, 49669, 54879, 57632, 54872, 50583, 0);
	size += add_multiline(buffer + size, 108085, 109111, 109908, 110997, 111043, 112122, 112623, 0);
	size += add_multiline(buffer + size, 109268, 111043, 0);
	size += add_multiline(buffer + size, 57380, 57757, 60129, 61941, 63090, 63608, 0);
	size += add_multiline(buffer + size, 61941, 64238, 66249, 0);
	size += add_multiline(buffer + size, 65474, 64238, 0);
	size += add_multiline(buffer + size, 44382, 41312, 35228, 34473, 37504, 0);
	size += add_multiline(buffer + size, 92175, 91117, 0);
	size += add_multiline(buffer + size, 94779, 95853, 97165, 100453, 102488, 104732, 0);
	size += add_multiline(buffer + size, 102098, 100453, 98110, 95947, 0);
	size += add_multiline(buffer + size, 78820, 80112, 78265, 0);
	size += add_multiline(buffer + size, 78401, 80112, 80763, 81266, 82396, 82514, 82729, 84143, 86228, 87073, 86670, 85927, 0);
	size += add_multiline(buffer + size, 87808, 85112, 84380, 81833, 81126, 79992, 0);
	size += add_multiline(buffer + size, 81833, 81693, 0);
	size += add_multiline(buffer + size, 84380, 83207, 0);
	size += add_multiline(buffer + size, 80170, 80816, 81693, 83207, 84379, 85693, 86974, 87933, 88794, 0);
	size += add_multiline(buffer + size, 59316, 59803, 60965, 61359, 59316, 0);
	size += add_multiline(buffer + size, 60718, 61084, 0);
	size += add_multiline(buffer + size, 62434, 59747, 0);
	size += add_multiline(buffer + size, 23875, 22109, 21444, 19587, 18543, 17378, 16537, 13701, 12770, 12843, 14146, 15474, 16611, 17651, 21393, 20535, 20042, 17797, 13847, 12486, 11407, 10602, 9007, 7588, 0);
	size += add_multiline(buffer + size, 55705, 54682, 53740, 55282, 55705, 0);
	size += add_multiline(buffer + size, 88048, 87108, 86742, 86032, 84345, 83000, 80883, 79593, 79882, 81377, 84012, 84970, 0);
	size += add_multiline(buffer + size, 31681, 34088, 35550, 37826, 36850, 32246, 30343, 29655, 0);
	size += add_multiline(buffer + size, 107089, 112405, 70638, 0);
	size += add_multiline(buffer + size, 37279, 36188, 0);
	size += add_multiline(buffer + size, 110130, 114996, 2484, 0);
	size += add_multiline(buffer + size, 87585, 85819, 85670, 87833, 87585, 94376, 97433, 89937, 83895, 80331, 78527, 75458, 68756, 61281, 56211, 0);
	size += add_multiline(buffer + size, 104987, 104858, 104521, 0);
	size += add_multiline(buffer + size, 30324, 32349, 33977, 34444, 33856, 33579, 30122, 0);
	size += add_multiline(buffer + size, 34444, 35904, 0);
	size += add_multiline(buffer + size, 12706, 14135, 0);
	size += add_multiline(buffer + size, 12828, 11484, 12706, 12387, 10826, 8645, 6537, 5364, 1562, 3419, 5364, 0);
	size += add_multiline(buffer + size, 8645, 8102, 0);
	size += add_multiline(buffer + size, 9884, 8903, 8832, 0);
	size += add_multiline(buffer + size, 101421, 101769, 102281, 102532, 101958, 101769, 0);
	size += add_multiline(buffer + size, 32768, 31685, 35264, 39429, 36377, 32768, 0);
	size += add_multiline(buffer + size, 39757, 38835, 38170, 37229, 36917, 35264, 0);
	size += add_multiline(buffer + size, 102422, 105199, 106032, 116727, 112724, 110991, 109492, 105199, 0);
	size += add_multiline(buffer + size, 32607, 27530, 27321, 0);
	size += add_multiline(buffer + size, 71683, 68702, 66657, 68002, 67472, 67464, 68933, 71352, 73334, 0);
	size += add_multiline(buffer + size, 66657, 61932, 59196, 0);
	size += add_multiline(buffer + size, 67464, 65109, 0);
	size += add_multiline(buffer + size, 80582, 80000, 0);
	size += add_multiline(buffer + size, 85727, 85267, 85258, 85792, 0);
	size += add_multiline(buffer + size, 83153, 83081, 82363, 0);
	size += add_multiline(buffer + size, 83081, 85258, 0);
	size += add_multiline(buffer + size, 37447, 34769, 30867, 29651, 0);
	size += add_multiline(buffer + size, 61585, 61199, 63613, 62322, 61585, 59929, 57363, 0);
	size += sprintf(buffer + size, "]}}]}");
	unsigned char *data = indigo_safe_malloc(buffer_size);
	unsigned data_size = buffer_size;
	indigo_compress("constellations.lines.json", buffer, size, data, &data_size);
	free(buffer);
	indigo_server_add_resource("/data/constellations.lines.json", data, (int)data_size, "application/json; charset=utf-8");
	return data;
}

#ifdef RPI_MANAGEMENT

static indigo_result execute_command(indigo_device *device, indigo_property *property, char *command, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);
	property->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, property, NULL);
	FILE *output = popen(buffer, "r");
	if (output) {
		char *line = NULL;
		size_t size = 0;
		if (getline(&line, &size, output) >= 0) {
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			if (!strncmp(line, "ALERT", 5)) {
				property->state = INDIGO_ALERT_STATE;
				char *message = strchr(line, ':');
				if (message) message++; /* if found ':', skip it */
				indigo_update_property(device, property, message);
			} else {
				property->state = INDIGO_OK_STATE;
				indigo_update_property(device, property, NULL);
			}
		} else {
			property->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, property, "No reply from rpi_ctrl.sh");
		}
		if (line)
			free(line);
		pclose(output);

		return INDIGO_OK;
	}
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, strerror(errno));
	return INDIGO_OK;
}

static char *execute_query(char *command, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);
	FILE *output = popen(buffer, "r");
	if (output) {
		char *line = NULL;
		size_t size = 0;
		if (getline(&line, &size, output) >= 0) {
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			pclose(output);
			return line;
		}
		pclose(output);
	}
	return NULL;
}

static void check_versions(indigo_device *device) {
	while (true) {
		bool redefine = false;
		char *line = execute_query("s_rpi_ctrl.sh --list-available-versions");
		if (line && strlen(line) > 0) {
			if (SERVER_INSTALL_PROPERTY) {
				indigo_delete_property(device, SERVER_INSTALL_PROPERTY, NULL);
				redefine = true;
			}
			pthread_mutex_lock(&install_property_mutex);
			if (redefine)
				indigo_release_property(SERVER_INSTALL_PROPERTY);
			SERVER_INSTALL_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_INSTALL_PROPERTY_NAME, MAIN_GROUP, "Available versions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 10);
			SERVER_INSTALL_PROPERTY->count = 0;
			char *pnt, *versions[10] = { strtok_r(line, " ", &pnt) };
			int count = 1;
			while ((versions[count] = strtok_r(NULL, " ", &pnt))) {
				if (count == 9)
					count = 1;
				else
					count++;
			}
			for (int i = 0; i < count; i++) {
				int smallest = 100000;
				int ii = 0;
				for (int j = 0; j < count; j++) {
					if (versions[j] == NULL)
						continue;
					char *build = strchr(versions[j], '-');
					if (build == NULL)
						continue;
					int build_number = atoi(build + 1);
					if (build_number < smallest) {
						smallest = build_number;
						ii = j;
					}
				}
				indigo_init_switch_item(SERVER_INSTALL_PROPERTY->items + i, versions[ii], versions[ii], versions[ii] == line);
				sprintf(SERVER_INSTALL_PROPERTY->items[i].hints,"warn_on_set:\"Install INDIGO %s and reboot host computer?\";", versions[ii]);
				SERVER_INSTALL_PROPERTY->count++;
				versions[ii] = NULL;
			}
			pthread_mutex_unlock(&install_property_mutex);
			free(line);
			if (redefine)
				indigo_define_property(device, SERVER_INSTALL_PROPERTY, NULL);
		}
		indigo_usleep(10 * 60 * ONE_SECOND_DELAY);
	}
}

static void update_wifi_setings(indigo_device *device) {
	char *line = execute_query("s_rpi_ctrl.sh --get-wifi-server");
	SERVER_WIFI_AP_PROPERTY->state=INDIGO_OK_STATE;
	if (line) {
		if (!strncmp(line, "ALERT:",6)) {
			SERVER_WIFI_AP_SSID_ITEM->text.value[0] = '\0';
			SERVER_WIFI_AP_PASSWORD_ITEM->text.value[0] = '\0';
			INDIGO_ERROR(indigo_error("%s", line));
			SERVER_WIFI_AP_PROPERTY->state=INDIGO_ALERT_STATE;
		} else {
			char *pnt, *token = strtok_r(line, "\t", &pnt);
			if (token) {
				indigo_copy_value(SERVER_WIFI_AP_SSID_ITEM->text.value, token);
			}
			token = strtok_r(NULL, "\t", &pnt);
			if (token) {
				indigo_copy_value(SERVER_WIFI_AP_PASSWORD_ITEM->text.value, token);
			} else {
				SERVER_WIFI_AP_PASSWORD_ITEM->text.value[0] = '\0';
			}
		}
		free(line);
	} else {
		SERVER_WIFI_AP_SSID_ITEM->text.value[0] = '\0';
		SERVER_WIFI_AP_PASSWORD_ITEM->text.value[0] = '\0';
		SERVER_WIFI_AP_PROPERTY->state=INDIGO_IDLE_STATE;
	}

	line = execute_query("s_rpi_ctrl.sh --get-wifi-client");
	SERVER_WIFI_INFRASTRUCTURE_PROPERTY->state=INDIGO_OK_STATE;
	if (line) {
		if (!strncmp(line, "ALERT:",6)) {
			SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value[0] = '\0';
			SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM->text.value[0] = '\0';
			INDIGO_ERROR(indigo_error("%s", line));
			SERVER_WIFI_INFRASTRUCTURE_PROPERTY->state=INDIGO_ALERT_STATE;
		} else {
			char *pnt, *token = strtok_r(line, "\t", &pnt);
			if (token) {
				indigo_copy_value(SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value, token);
				SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM->text.value[0] = '\0';
			}
		}
		free(line);
	} else {
		SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value[0] = '\0';
		SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM->text.value[0] = '\0';
		SERVER_WIFI_INFRASTRUCTURE_PROPERTY->state=INDIGO_IDLE_STATE;
	}

	line = execute_query("s_rpi_ctrl.sh --get-wifi-channel");
	SERVER_WIFI_CHANNEL_PROPERTY->state=INDIGO_OK_STATE;
	if (line) {
		if (!strncmp(line, "ALERT:",6)) {
			SERVER_WIFI_CHANNEL_ITEM->number.target = SERVER_WIFI_CHANNEL_ITEM->number.value = 0;
			INDIGO_ERROR(indigo_error("%s", line));
			SERVER_WIFI_CHANNEL_PROPERTY->state=INDIGO_ALERT_STATE;
		} else {
			SERVER_WIFI_CHANNEL_ITEM->number.target = SERVER_WIFI_CHANNEL_ITEM->number.value = atoi(line);
		}
		free(line);
	} else {
		SERVER_WIFI_CHANNEL_ITEM->number.target = SERVER_WIFI_CHANNEL_ITEM->number.value = 0;
	}

	line = execute_query("s_rpi_ctrl.sh --get-wifi-country-code");
	SERVER_WIFI_COUNTRY_CODE_PROPERTY->state=INDIGO_OK_STATE;
	if (line) {
		if (!strncmp(line, "ALERT:", 6)) {
			SERVER_WIFI_COUNTRY_CODE_ITEM->text.value[0] = '\0';
			INDIGO_ERROR(indigo_error("%s", line));
			SERVER_WIFI_COUNTRY_CODE_PROPERTY->state=INDIGO_ALERT_STATE;
		} else {
			indigo_copy_value(SERVER_WIFI_COUNTRY_CODE_ITEM->text.value, line);
		}
		free(line);
	} else {
		SERVER_WIFI_COUNTRY_CODE_ITEM->text.value[0] = '\0';
	}

	indigo_update_property(device, SERVER_WIFI_COUNTRY_CODE_PROPERTY, NULL);
	indigo_update_property(device, SERVER_WIFI_AP_PROPERTY, NULL);
	indigo_update_property(device, SERVER_WIFI_CHANNEL_PROPERTY, NULL);
	indigo_update_property(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, NULL);
}

#endif

static indigo_result attach(indigo_device *device) {
	assert(device != NULL);
	char hostname[INDIGO_NAME_SIZE];
	gethostname(hostname, sizeof(hostname));
	if (*indigo_local_service_name == 0) {
		indigo_service_name(hostname, indigo_server_tcp_port, indigo_local_service_name);
	}
	SERVER_INFO_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_INFO_PROPERTY_NAME, MAIN_GROUP, "Server info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
	indigo_init_text_item(SERVER_INFO_VERSION_ITEM, SERVER_INFO_VERSION_ITEM_NAME, "INDIGO version", "%d.%d-%s", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_BUILD);
	indigo_init_text_item(SERVER_INFO_SERVICE_ITEM, SERVER_INFO_SERVICE_ITEM_NAME, "INDIGO service", indigo_local_service_name);
	SERVER_DRIVERS_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_DRIVERS_PROPERTY_NAME, MAIN_GROUP, "Available drivers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	SERVER_DRIVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], indigo_available_drivers[i].name, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
	for (int i = 0; i < dynamic_drivers_count && SERVER_DRIVERS_PROPERTY->count < INDIGO_MAX_DRIVERS; i++)
		indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], dynamic_drivers[i].name, dynamic_drivers[i].description, false);
	indigo_property_sort_items(SERVER_DRIVERS_PROPERTY, 0);
	SERVER_SERVERS_PROPERTY = indigo_init_light_property(NULL, server_device.name, SERVER_SERVERS_PROPERTY_NAME, MAIN_GROUP, "Configured servers", INDIGO_OK_STATE, 2 * INDIGO_MAX_SERVERS);
	SERVER_SERVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_server_entry *entry = indigo_available_servers + i;
		if (*entry->host) {
			char buf[INDIGO_NAME_SIZE];
			if (entry->port == 7624)
				strncpy(buf, entry->host, sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "%s:%d", entry->host, entry->port);
			indigo_init_light_item(&SERVER_SERVERS_PROPERTY->items[SERVER_SERVERS_PROPERTY->count++], buf, buf, INDIGO_OK_STATE);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_subprocess_entry *entry = indigo_available_subprocesses + i;
		if (*entry->executable) {
			indigo_init_light_item(&SERVER_SERVERS_PROPERTY->items[SERVER_SERVERS_PROPERTY->count++], entry->executable, entry->executable, INDIGO_OK_STATE);
		}
	}
	indigo_property_sort_items(SERVER_SERVERS_PROPERTY, 0);
	SERVER_LOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_LOAD_PROPERTY_NAME, MAIN_GROUP, "Load driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_LOAD_ITEM, SERVER_LOAD_ITEM_NAME, "Load driver", "");
	SERVER_UNLOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_UNLOAD_PROPERTY_NAME, MAIN_GROUP, "Unload driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_UNLOAD_ITEM,SERVER_UNLOAD_ITEM_NAME, "Unload driver", "");
	SERVER_RESTART_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_RESTART_PROPERTY_NAME, MAIN_GROUP, "Restart", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(SERVER_RESTART_ITEM, SERVER_RESTART_ITEM_NAME, "Restart server", false);
	strcpy(SERVER_RESTART_ITEM->hints,"warn_on_set:\"Restart INDIGO Server?\";");
	SERVER_LOG_LEVEL_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_LOG_LEVEL_PROPERTY_NAME, MAIN_GROUP, "Log level", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
	indigo_init_switch_item(SERVER_LOG_LEVEL_ERROR_ITEM, SERVER_LOG_LEVEL_ERROR_ITEM_NAME, "Error", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_INFO_ITEM, SERVER_LOG_LEVEL_INFO_ITEM_NAME, "Info", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_DEBUG_ITEM, SERVER_LOG_LEVEL_DEBUG_ITEM_NAME, "Debug", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_TRACE_BUS_ITEM, SERVER_LOG_LEVEL_TRACE_BUS_ITEM_NAME, "Trace bus", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_TRACE_ITEM, SERVER_LOG_LEVEL_TRACE_ITEM_NAME, "Trace", false);
	SERVER_BLOB_BUFFERING_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_BLOB_BUFFERING_PROPERTY_NAME, MAIN_GROUP, "BLOB buffering", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	indigo_init_switch_item(SERVER_BLOB_BUFFERING_DISABLED_ITEM, SERVER_BLOB_BUFFERING_DISABLED_ITEM_NAME, "Disabled", !indigo_use_blob_buffering);
	indigo_init_switch_item(SERVER_BLOB_BUFFERING_ENABLED_ITEM, SERVER_BLOB_BUFFERING_ENABLED_ITEM_NAME, "Enabled", indigo_use_blob_buffering && !indigo_use_blob_compression);
	indigo_init_switch_item(SERVER_BLOB_COMPRESSION_ENABLED_ITEM, SERVER_BLOB_COMPRESSION_ENABLED_ITEM_NAME, "Enabled with compression", indigo_use_blob_buffering && indigo_use_blob_compression);
	SERVER_BLOB_PROXY_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_BLOB_PROXY_PROPERTY_NAME, MAIN_GROUP, "BLOB proxy", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	indigo_init_switch_item(SERVER_BLOB_PROXY_DISABLED_ITEM, SERVER_BLOB_PROXY_DISABLED_ITEM_NAME, "Disabled", !indigo_proxy_blob);
	indigo_init_switch_item(SERVER_BLOB_PROXY_ENABLED_ITEM, SERVER_BLOB_PROXY_ENABLED_ITEM_NAME, "Enabled", indigo_proxy_blob);
	SERVER_FEATURES_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_FEATURES_PROPERTY_NAME, MAIN_GROUP, "Features", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	indigo_init_switch_item(SERVER_BONJOUR_ITEM, SERVER_BONJOUR_ITEM_NAME, "Bonjour", indigo_use_bonjour);
	indigo_init_switch_item(SERVER_CTRL_PANEL_ITEM, SERVER_CTRL_PANEL_ITEM_NAME, "Control panel / Server manager", use_ctrl_panel);
	indigo_init_switch_item(SERVER_WEB_APPS_ITEM, SERVER_WEB_APPS_ITEM_NAME, "Web applications", use_web_apps);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		SERVER_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_WIFI_AP_PROPERTY_NAME, MAIN_GROUP, "Configure access point WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(SERVER_WIFI_AP_SSID_ITEM, SERVER_WIFI_AP_SSID_ITEM_NAME, "Network name", "");
		indigo_init_text_item(SERVER_WIFI_AP_PASSWORD_ITEM, SERVER_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		strcpy(SERVER_WIFI_AP_PROPERTY->hints,"warn_on_change:\"Changing WiFi settings will disconnect all devices connected over WiFi. Continue?\";");

		SERVER_WIFI_INFRASTRUCTURE_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_WIFI_INFRASTRUCTURE_PROPERTY_NAME, MAIN_GROUP, "Configure infrastructure WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item(SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM, SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM, SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM_NAME, "Password", "");
		strcpy(SERVER_WIFI_INFRASTRUCTURE_PROPERTY->hints,"warn_on_change:\"Changing WiFi settings will disconnect all devices connected over WiFi. Continue?\";");

		SERVER_WIFI_CHANNEL_PROPERTY = indigo_init_number_property(NULL, server_device.name, SERVER_WIFI_CHANNEL_PROPERTY_NAME, MAIN_GROUP, "WiFi server channel", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_number_item(SERVER_WIFI_CHANNEL_ITEM, SERVER_WIFI_CHANNEL_ITEM_NAME, "Channel (0 = auto 2.4GHz, [0-14] = 2.4G, [>36] = 5G)", 0, 160, 1, 0);
		strcpy(SERVER_WIFI_CHANNEL_PROPERTY->hints,"warn_on_change:\"Available WiFi channels depend on country regulations!\nChanging WiFi channel will disconnect all devices connected over WiFi. Continue?\";");

		SERVER_WIFI_COUNTRY_CODE_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_WIFI_COUNTRY_CODE_PROPERTY_NAME, MAIN_GROUP, "Configure WiFi country code", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_text_item(SERVER_WIFI_COUNTRY_CODE_ITEM, SERVER_WIFI_COUNTRY_CODE_ITEM_NAME, "Two letter country code", "");
		strcpy(SERVER_WIFI_COUNTRY_CODE_PROPERTY->hints, "warn_on_change:\"Country code affects the available WiFi channels.\nChanging the country code will reset WiFi settings to defaults. Continue?\";");

		update_wifi_setings(device);

		SERVER_INTERNET_SHARING_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_INTERNET_SHARING_PROPERTY_NAME, MAIN_GROUP, "Internet sharing", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(SERVER_INTERNET_SHARING_DISABLED_ITEM, SERVER_INTERNET_SHARING_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(SERVER_INTERNET_SHARING_ENABLED_ITEM, SERVER_INTERNET_SHARING_ENABLED_ITEM_NAME, "Enabled", false);
		char *line = execute_query("s_rpi_ctrl.sh --get-forwarding");
		if (line) {
			if (!strncmp(line, "1", 1)) {
				indigo_set_switch(SERVER_INTERNET_SHARING_PROPERTY, SERVER_INTERNET_SHARING_ENABLED_ITEM, true);
			}
			free(line);
		}
		if (SERVER_WIFI_INFRASTRUCTURE_PROPERTY->state == INDIGO_ALERT_STATE && SERVER_WIFI_AP_PROPERTY->state == INDIGO_ALERT_STATE) {
			indigo_set_timer(device, 30, update_wifi_setings, NULL);
		}
		SERVER_HOST_TIME_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_HOST_TIME_PROPERTY_NAME, MAIN_GROUP, "Set host time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_text_item(SERVER_HOST_TIME_ITEM, SERVER_HOST_TIME_ITEM_NAME, "Host time", "");
		SERVER_SHUTDOWN_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_SHUTDOWN_PROPERTY_NAME, MAIN_GROUP, "Shutdown host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(SERVER_SHUTDOWN_ITEM, SERVER_SHUTDOWN_ITEM_NAME, "Shutdown", false);
		strcpy(SERVER_SHUTDOWN_ITEM->hints,"warn_on_set:\"Shutdown host computer?\";");
		SERVER_REBOOT_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_REBOOT_PROPERTY_NAME, MAIN_GROUP, "Reboot host computer", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(SERVER_REBOOT_ITEM, SERVER_REBOOT_ITEM_NAME, "Reboot", false);
		strcpy(SERVER_REBOOT_ITEM->hints,"warn_on_set:\"Reboot host computer?\";");
		indigo_async((void *(*)(void *))check_versions, device);
	}
#endif /* RPI_MANAGEMENT */
	indigo_log_levels log_level = indigo_get_log_level();
	switch (log_level) {
		case INDIGO_LOG_ERROR:
			SERVER_LOG_LEVEL_ERROR_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_INFO:
			SERVER_LOG_LEVEL_INFO_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_DEBUG:
			SERVER_LOG_LEVEL_DEBUG_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_TRACE_BUS:
			SERVER_LOG_LEVEL_TRACE_BUS_ITEM->sw.value = true;
			break;
		case INDIGO_LOG_TRACE:
			SERVER_LOG_LEVEL_TRACE_ITEM->sw.value = true;
			break;
		default:
			break;
	}
	if (!command_line_drivers)
		indigo_load_properties(device, false);
	INDIGO_LOG(indigo_log("%s attached", device->name));
	return INDIGO_OK;
}

static indigo_result enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_define_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0)
		indigo_define_property(device, SERVER_SERVERS_PROPERTY, NULL);
	indigo_define_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_RESTART_PROPERTY, NULL);
	indigo_define_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_define_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
	indigo_define_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
	indigo_define_property(device, SERVER_FEATURES_PROPERTY, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_define_property(device, SERVER_WIFI_COUNTRY_CODE_PROPERTY, NULL);
		indigo_define_property(device, SERVER_WIFI_AP_PROPERTY, NULL);
		indigo_define_property(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, NULL);
		indigo_define_property(device, SERVER_WIFI_CHANNEL_PROPERTY, NULL);
		indigo_define_property(device, SERVER_INTERNET_SHARING_PROPERTY, NULL);
		indigo_define_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
		indigo_define_property(device, SERVER_SHUTDOWN_PROPERTY, NULL);
		indigo_define_property(device, SERVER_REBOOT_PROPERTY, NULL);
		indigo_define_property(device, SERVER_INSTALL_PROPERTY, NULL);
	}
#endif /* RPI_MANAGEMENT */
	return INDIGO_OK;
}

static void send_driver_load_error_message(indigo_result result, char *driver_name) {
	if (result == INDIGO_UNSUPPORTED_ARCH) {
		indigo_send_message(&server_device, "Driver '%s' failed to load: not supported on this architecture", driver_name);
	} else if (result == INDIGO_UNRESOLVED_DEPS) {
		indigo_send_message(&server_device, "Driver '%s' failed to load: unresolved dependencies", driver_name);
	} else if (result != INDIGO_OK){
		indigo_send_message(&server_device, "Driver '%s' failed to load", driver_name);
	}
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
#ifdef RPI_MANAGEMENT
	pthread_mutex_lock(&install_property_mutex);
	if (indigo_property_match(SERVER_INSTALL_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- INSTALL
		indigo_property_copy_values(SERVER_INSTALL_PROPERTY, property, false);
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				indigo_result result = execute_command(device, SERVER_INSTALL_PROPERTY, "s_rpi_ctrl.sh --install-version %s", property->items[i].name);
				pthread_mutex_unlock(&install_property_mutex);
				return result;
			}
		}
	}
	pthread_mutex_unlock(&install_property_mutex);
#endif /* RPI_MANAGEMENT */
	if (indigo_property_match(SERVER_DRIVERS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- DRIVERS
		if (command_line_drivers && !strcmp(client->name, CONFIG_READER))
			return INDIGO_OK;
		indigo_property_copy_values(SERVER_DRIVERS_PROPERTY, property, false);
		for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
			char *name = SERVER_DRIVERS_PROPERTY->items[i].name;
			indigo_driver_entry *driver = NULL;
			for (int j = 0; j < INDIGO_MAX_DRIVERS; j++) {
				if (!strcmp(indigo_available_drivers[j].name, name)) {
					driver = &indigo_available_drivers[j];
					break;
				}
			}
			if (SERVER_DRIVERS_PROPERTY->items[i].sw.value) {
				if (driver) {
					if (driver->dl_handle == NULL && !driver->initialized) {
						indigo_result result = driver->driver(INDIGO_DRIVER_INIT, NULL);
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = result == INDIGO_OK;
						send_driver_load_error_message(result, driver->name);
					} else if (driver->dl_handle != NULL && !driver->initialized) {
						indigo_result result = driver->driver(INDIGO_DRIVER_INIT, NULL);
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = result == INDIGO_OK;
						send_driver_load_error_message(result, driver->name);
						if (driver && !driver->initialized)
							indigo_remove_driver(driver);
					}
				} else {
					indigo_result result = indigo_load_driver(name, true, &driver);
					SERVER_DRIVERS_PROPERTY->items[i].sw.value = result == INDIGO_OK;
					if (driver && !driver->initialized) {
						indigo_remove_driver(driver);
					}
					send_driver_load_error_message(result, name);
				}
			} else if (driver) {
				indigo_result result = INDIGO_OK;
				if (driver->dl_handle) {
					result = indigo_remove_driver(driver);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					}
				} else if (driver->initialized) {
					result = driver->driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					} else {
						driver->initialized = false;
					}
				}
				if (result != INDIGO_OK) {
					if (result == INDIGO_BUSY) {
						indigo_send_message(device, "Driver %s is in use, can't be unloaded", name);
					} else {
						indigo_send_message(device, "Driver %s failed to unload", name);
					}
				}
			}
		}
		SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
		int handle = 0;
		if (!command_line_drivers) {
			indigo_save_property(device, &handle, SERVER_DRIVERS_PROPERTY);
			close(handle);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_LOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOAD
		indigo_property_copy_values(SERVER_LOAD_PROPERTY, property, false);
		if (*SERVER_LOAD_ITEM->text.value) {
			char *name = basename(SERVER_LOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) is already loaded", name, indigo_available_drivers[i].description);
					return INDIGO_OK;
				}
			indigo_driver_entry *driver = NULL;
			indigo_result result = INDIGO_OK;
			if ((result = indigo_load_driver(SERVER_LOAD_ITEM->text.value, true, &driver)) == INDIGO_OK) {
				bool found = false;
				for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
					if (!strcmp(SERVER_DRIVERS_PROPERTY->items[i].name, name)) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
						SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
						found = true;
						break;
					}
				}
				if (!found) {
					indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
					indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], driver->name, driver->description, driver->initialized);
					SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
				}
				SERVER_LOAD_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) loaded", name, driver->description);
			} else {
				SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
				if (driver && !driver->initialized) {
					indigo_remove_driver(driver);
				}
				send_driver_load_error_message(result, name);
				indigo_update_property(device, SERVER_LOAD_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_UNLOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- UNLOAD
		indigo_property_copy_values(SERVER_UNLOAD_PROPERTY, property, false);
		if (*SERVER_UNLOAD_ITEM->text.value) {
			char *name = basename(SERVER_UNLOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					indigo_result result;
					if (indigo_available_drivers[i].dl_handle) {
						result = indigo_remove_driver(&indigo_available_drivers[i]);
					} else {
						result = indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					}
					if (result == INDIGO_OK) {
						for (int j = 0; j < SERVER_DRIVERS_PROPERTY->count; j++) {
							if (!strcmp(SERVER_DRIVERS_PROPERTY->items[j].name, name)) {
								SERVER_DRIVERS_PROPERTY->items[j].sw.value = false;
								SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
								indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
								break;
							}
						}
						SERVER_UNLOAD_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s unloaded", name);
					} else if (result == INDIGO_BUSY) {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is in use, can't be unloaded", name);
					} else {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s failed to unload", name);
					}
					return INDIGO_OK;
				}
			SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is not loaded", name);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_RESTART_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- RESTART
		indigo_property_copy_values(SERVER_RESTART_PROPERTY, property, false);
		if (SERVER_RESTART_ITEM->sw.value) {
			INDIGO_LOG(indigo_log("Restarting..."));
			indigo_server_shutdown();
			exit(0);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_LOG_LEVEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOG_LEVEL
		indigo_property_copy_values(SERVER_LOG_LEVEL_PROPERTY, property, false);
		if (SERVER_LOG_LEVEL_ERROR_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_ERROR);
		} else if (SERVER_LOG_LEVEL_INFO_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_INFO);
		} else if (SERVER_LOG_LEVEL_DEBUG_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_DEBUG);
		} else if (SERVER_LOG_LEVEL_TRACE_BUS_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE_BUS);
		} else if (SERVER_LOG_LEVEL_TRACE_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE);
		}
		SERVER_LOG_LEVEL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_BLOB_BUFFERING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SERVER_BLOB_BUFFERING
		indigo_property_copy_values(SERVER_BLOB_BUFFERING_PROPERTY, property, false);
		indigo_use_blob_buffering = SERVER_BLOB_BUFFERING_ENABLED_ITEM->sw.value || SERVER_BLOB_COMPRESSION_ENABLED_ITEM->sw.value;
		indigo_use_blob_compression = SERVER_BLOB_COMPRESSION_ENABLED_ITEM->sw.value;
		SERVER_BLOB_BUFFERING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_BLOB_PROXY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SERVER_BLOB_PROXY
		indigo_property_copy_values(SERVER_BLOB_PROXY_PROPERTY, property, false);
		indigo_proxy_blob = SERVER_BLOB_PROXY_ENABLED_ITEM->sw.value;
		SERVER_BLOB_PROXY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
		return INDIGO_OK;
#ifdef RPI_MANAGEMENT
	} else if (indigo_property_match(SERVER_WIFI_COUNTRY_CODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WIFI_COUNTRY_CODE
		indigo_property_copy_values(SERVER_WIFI_COUNTRY_CODE_PROPERTY, property, false);
		execute_command(device, SERVER_WIFI_COUNTRY_CODE_PROPERTY, "s_rpi_ctrl.sh --set-wifi-country-code \"%s\"", SERVER_WIFI_COUNTRY_CODE_ITEM->text.value);
		update_wifi_setings(device);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_WIFI_AP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WIFI_AP
		indigo_property_copy_values(SERVER_WIFI_AP_PROPERTY, property, false);
		execute_command(device, SERVER_WIFI_AP_PROPERTY, "s_rpi_ctrl.sh --set-wifi-server \"%s\" \"%s\"", SERVER_WIFI_AP_SSID_ITEM->text.value, SERVER_WIFI_AP_PASSWORD_ITEM->text.value);
		update_wifi_setings(device);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_WIFI_INFRASTRUCTURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WIFI_INFRASTRUCTURE
		indigo_property_copy_values(SERVER_WIFI_INFRASTRUCTURE_PROPERTY, property, false);
		execute_command(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, "s_rpi_ctrl.sh --set-wifi-client \"%s\" \"%s\"", SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM->text.value, SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM->text.value);
		update_wifi_setings(device);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_WIFI_CHANNEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SERVER_WIFI_CHANNEL
		indigo_property_copy_values(SERVER_WIFI_CHANNEL_PROPERTY, property, false);
		if (execute_command(device, SERVER_WIFI_CHANNEL_PROPERTY, "s_rpi_ctrl.sh --set-wifi-channel %d", (int)SERVER_WIFI_CHANNEL_ITEM->number.value) == INDIGO_OK) {
			char *line = execute_query("s_rpi_ctrl.sh --get-wifi-channel");
			if (line) {
				SERVER_WIFI_CHANNEL_ITEM->number.target = SERVER_WIFI_CHANNEL_ITEM->number.value = atoi(line);
				free(line);
				SERVER_WIFI_CHANNEL_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, SERVER_WIFI_CHANNEL_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_INTERNET_SHARING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- INTERNET_SHARING
		indigo_property_copy_values(SERVER_INTERNET_SHARING_PROPERTY, property, false);
		if (SERVER_INTERNET_SHARING_ENABLED_ITEM->sw.value) { /* item[1] is enable forwarding, aka network sharing */
			indigo_send_message(device, "Internet sharing is potentially dangerous, everyone connected to your INDIGO Sky can access your network!");
			return execute_command(device, SERVER_INTERNET_SHARING_PROPERTY, "s_rpi_ctrl.sh --enable-forwarding");
		} else {
			return execute_command(device, SERVER_INTERNET_SHARING_PROPERTY, "s_rpi_ctrl.sh --disable-forwarding");
		}
	} else if (indigo_property_match(SERVER_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HOST_TIME
		indigo_property_copy_values(SERVER_HOST_TIME_PROPERTY, property, false);
		execute_command(device, SERVER_HOST_TIME_PROPERTY, "s_rpi_ctrl.sh --set-date \"%s\"", SERVER_HOST_TIME_ITEM->text.value);
		if (SERVER_HOST_TIME_PROPERTY->state == INDIGO_OK_STATE) {
			indigo_delete_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
			SERVER_HOST_TIME_PROPERTY->hidden = true;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_SHUTDOWN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SHUTDOWN
		indigo_property_copy_values(SERVER_SHUTDOWN_PROPERTY, property, false);
		if (SERVER_SHUTDOWN_ITEM->sw.value) {
			return execute_command(device, SERVER_SHUTDOWN_PROPERTY, "s_rpi_ctrl.sh --poweroff");
		}
		SERVER_SHUTDOWN_ITEM->sw.value = false;
		indigo_update_property(device, SERVER_SHUTDOWN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- REBOOT
		indigo_property_copy_values(SERVER_REBOOT_PROPERTY, property, false);
		if (SERVER_REBOOT_ITEM->sw.value) {
			return execute_command(device, SERVER_REBOOT_PROPERTY, "s_rpi_ctrl.sh --reboot");
		}
		SERVER_REBOOT_ITEM->sw.value = false;
		indigo_update_property(device, SERVER_REBOOT_PROPERTY, NULL);
		return INDIGO_OK;
#endif /* RPI_MANAGEMENT */
	// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}


static indigo_result detach(indigo_device *device) {
	assert(device != NULL);
	indigo_delete_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0)
		indigo_delete_property(device, SERVER_SERVERS_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_RESTART_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_FEATURES_PROPERTY, NULL);
#ifdef RPI_MANAGEMENT
	if (use_rpi_management) {
		indigo_delete_property(device, SERVER_WIFI_COUNTRY_CODE_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_WIFI_INFRASTRUCTURE_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_WIFI_CHANNEL_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_INTERNET_SHARING_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_HOST_TIME_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_SHUTDOWN_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, SERVER_INSTALL_PROPERTY, NULL);
	}
#endif /* RPI_MANAGEMENT */
	indigo_release_property(SERVER_INFO_PROPERTY);
	indigo_release_property(SERVER_DRIVERS_PROPERTY);
	indigo_release_property(SERVER_SERVERS_PROPERTY);
	indigo_release_property(SERVER_LOAD_PROPERTY);
	indigo_release_property(SERVER_UNLOAD_PROPERTY);
	indigo_release_property(SERVER_RESTART_PROPERTY);
	indigo_release_property(SERVER_LOG_LEVEL_PROPERTY);
	indigo_release_property(SERVER_BLOB_BUFFERING_PROPERTY);
	indigo_release_property(SERVER_BLOB_PROXY_PROPERTY);
	indigo_release_property(SERVER_FEATURES_PROPERTY);
#ifdef RPI_MANAGEMENT
	indigo_release_property(SERVER_WIFI_COUNTRY_CODE_PROPERTY);
	indigo_release_property(SERVER_WIFI_AP_PROPERTY);
	indigo_release_property(SERVER_WIFI_INFRASTRUCTURE_PROPERTY);
	indigo_release_property(SERVER_WIFI_CHANNEL_PROPERTY);
	indigo_release_property(SERVER_INTERNET_SHARING_PROPERTY);
	indigo_release_property(SERVER_HOST_TIME_PROPERTY);
	indigo_release_property(SERVER_SHUTDOWN_PROPERTY);
	indigo_release_property(SERVER_REBOOT_PROPERTY);
	indigo_release_property(SERVER_INSTALL_PROPERTY);
#endif /* RPI_MANAGEMENT */
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

static void add_drivers(const char *folder) {
	char folder_path[PATH_MAX];
	if(NULL == realpath(folder, folder_path)) {
		INDIGO_DEBUG(indigo_debug("realpath(%s, folder_path): failed", folder));
		return;
	}
	DIR *dir = opendir(folder_path);
	if (dir) {
		struct dirent *ent;
		char *line = NULL;
		size_t len = 0;
		while ((ent = readdir (dir)) != NULL) {
			if (!strncmp(ent->d_name, "indigo_", 7)) {
				char path[PATH_MAX];
				sprintf(path, "%s/%s", folder_path, ent->d_name);
				indigo_log("Loading driver list from %s", path);
				FILE *list = fopen(path, "r");
				if (list) {
					while (getline(&line, &len, list) > 0 && dynamic_drivers_count < INDIGO_MAX_DRIVERS) {
						char *pnt, *token = strtok_r(line, ",", &pnt);
						if (token && (token = strchr(token, '"'))) {
							char *end = strchr(++token, '"');
							if (end) {
								*end = 0;
								for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
									if (!strcmp(indigo_available_drivers[i].name, token)) {
										token = NULL;
										break;
									}
								}
								if (token) {
									for (int i = 0; i < dynamic_drivers_count; i++) {
										if (!strcmp(dynamic_drivers[i].name, token)) {
											token = NULL;
											break;
										}
									}
								}
								if (token) {
									dynamic_drivers[dynamic_drivers_count].name = strdup(token);
								} else {
									continue;
								}
							}
						}
						token = strtok_r(NULL, ",", &pnt);
						if (token && (token = strchr(token, '"'))) {
							char *end = strchr(token + 1, '"');
							if (end) {
								*end = 0;
								dynamic_drivers[dynamic_drivers_count].description = strdup(token + 1);
							}
						}
						dynamic_drivers_count++;
					}
					fclose(list);
				}
			}
		}
		closedir(dir);
		if (line)
			free(line);
	}
}

static void server_main() {
	indigo_start_usb_event_handler();
	indigo_start();
	indigo_log("INDIGO server %d.%d-%s built on %s %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, INDIGO_BUILD_TIME, INDIGO_BUILD_COMMIT);

	indigo_use_blob_caching = true;

	/* Make sure master token and ACL are loaded before drivers */
	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			indigo_set_master_token(indigo_string_to_token(server_argv[i + 1]));
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			indigo_load_device_tokens_from_file(server_argv[i + 1]);
			i++;
		}
	}
	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-p") || !strcmp(server_argv[i], "--port")) && i < server_argc - 1) {
			indigo_server_tcp_port = atoi(server_argv[i + 1]);
			i++;
		} else if ((!strcmp(server_argv[i], "-r") || !strcmp(server_argv[i], "--remote-server")) && i < server_argc - 1) {
			char host[INDIGO_NAME_SIZE];
			indigo_copy_name(host, server_argv[i + 1]);
			char *colon = strchr(host, ':');
			int port = 7624;
			if (colon != NULL) {
				*colon++ = 0;
				port = atoi(colon);
			}
			indigo_reshare_remote_devices = true;
			indigo_connect_server(NULL, host, port, NULL);
			i++;
		} else if ((!strcmp(server_argv[i], "-i") || !strcmp(server_argv[i], "--indi-driver")) && i < server_argc - 1) {
			char executable[INDIGO_NAME_SIZE];
			indigo_copy_name(executable, server_argv[i + 1]);
			indigo_reshare_remote_devices = true;
			indigo_start_subprocess(executable, NULL);
			i++;
		} else if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			/* just skip it - handled above */
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			/* just skip it - handled above */
			i++;
		} else if (!strcmp(server_argv[i], "-b-") || !strcmp(server_argv[i], "--disable-bonjour")) {
			indigo_use_bonjour = false;
		} else if (!strcmp(server_argv[i], "-b") || !strcmp(server_argv[i], "--bonjour")) {
			indigo_copy_name(indigo_local_service_name, server_argv[i + 1]);
			i++;
		} else if (!strcmp(server_argv[i], "-c-") || !strcmp(server_argv[i], "--disable-control-panel")) {
			use_ctrl_panel = false;
		} else if (!strcmp(server_argv[i], "-w-") || !strcmp(server_argv[i], "--disable-web-apps")) {
			use_web_apps = false;
		} else if (!strcmp(server_argv[i], "-u-") || !strcmp(server_argv[i], "--disable-blob-urls")) {
			indigo_use_blob_urls = false;
		} else if (!strcmp(server_argv[i], "-d-") || !strcmp(server_argv[i], "--disable-blob-buffering")) {
			indigo_use_blob_buffering = false;
			indigo_use_blob_compression = false;
		} else if (!strcmp(server_argv[i], "-C") || !strcmp(server_argv[i], "--enable-blob-compression")) {
			indigo_use_blob_compression = true;
		} else if (!strcmp(server_argv[i], "-x") || !strcmp(server_argv[i], "--enable-blob-proxy")) {
			indigo_proxy_blob = true;
#ifdef RPI_MANAGEMENT
		} else if (!strcmp(server_argv[i], "-f") || !strcmp(server_argv[i], "--enable-rpi-management")) {
			FILE *output = popen("which s_rpi_ctrl.sh", "r");
			if (output) {
				char *line = NULL;
				size_t size = 0;
				if (getline(&line, &size, output) >= 0) {
					use_rpi_management = true;
				}
				pclose(output);
			}
			if (!use_rpi_management) {
				indigo_log("No s_rpi_ctrl.sh found");
			}
#endif /* RPI_MANAGEMENT */
		} else if(server_argv[i][0] != '-') {
			indigo_load_driver(server_argv[i], true, NULL);
			command_line_drivers = true;
		}
	}

	use_ctrl_panel |= use_web_apps;

	if (use_ctrl_panel) {
		// INDIGO Server Manager
		static unsigned char mng_html[] = {
			#include "resource/mng.html.data"
		};
		indigo_server_add_resource("/mng.html", mng_html, sizeof(mng_html), "text/html");
		static unsigned char mng_png[] = {
			#include "resource/mng.png.data"
		};
		indigo_server_add_resource("/mng.png", mng_png, sizeof(mng_png), "image/png");
		// INDIGO Control Panel
		static unsigned char ctrl_html[] = {
			#include "resource/ctrl.html.data"
		};
		indigo_server_add_resource("/ctrl.html", ctrl_html, sizeof(ctrl_html), "text/html");
		static unsigned char ctrl_png[] = {
			#include "resource/ctrl.png.data"
		};
		indigo_server_add_resource("/ctrl.png", ctrl_png, sizeof(ctrl_png), "image/png");
		// INDIGO
		static unsigned char indigo_js[] = {
			#include "resource/indigo.js.data"
		};
		indigo_server_add_resource("/indigo.js", indigo_js, sizeof(indigo_js), "text/javascript");
		static unsigned char components_js[] = {
			#include "resource/components.js.data"
		};
		indigo_server_add_resource("/components.js", components_js, sizeof(components_js), "text/javascript");
		static unsigned char indigo_css[] = {
			#include "resource/indigo.css.data"
		};
		indigo_server_add_resource("/indigo.css", indigo_css, sizeof(indigo_css), "text/css");
		// Bootstrap
		static unsigned char bootstrap_css[] = {
			#include "resource/bootstrap.min.css.data"
		};
		indigo_server_add_resource("/bootstrap.min.css", bootstrap_css, sizeof(bootstrap_css), "text/css");
		static unsigned char bootstrap_js[] = {
			#include "resource/bootstrap.min.js.data"
		};
		indigo_server_add_resource("/bootstrap.min.js", bootstrap_js, sizeof(bootstrap_js), "text/javascript");
		static unsigned char popper_js[] = {
			#include "resource/popper.min.js.data"
		};
		indigo_server_add_resource("/popper.min.js", popper_js, sizeof(popper_js), "text/javascript");
		static unsigned char glyphicons_css[] = {
			#include "resource/glyphicons.css.data"
		};
		indigo_server_add_resource("/glyphicons.css", glyphicons_css, sizeof(glyphicons_css), "text/css");
		static unsigned char glyphicons_ttf[] = {
			#include "resource/glyphicons-regular.ttf.data"
		};
		indigo_server_add_resource("/glyphicons-regular.ttf", glyphicons_ttf, sizeof(glyphicons_ttf), "font/ttf");
		// JQuery
		static unsigned char jquery_js[] = {
			#include "resource/jquery.min.js.data"
		};
		indigo_server_add_resource("/jquery.min.js", jquery_js, sizeof(jquery_js), "text/javascript");
		// VueJS
		static unsigned char vue_js[] = {
			#include "resource/vue.min.js.data"
		};
		indigo_server_add_resource("/vue.min.js", vue_js, sizeof(vue_js), "text/javascript");
	}
	if (use_web_apps) {
		// INDIGO Imager
		static unsigned char imager_html[] = {
			#include "resource/imager.html.data"
		};
		indigo_server_add_resource("/imager.html", imager_html, sizeof(imager_html), "text/html");
		static unsigned char imager_png[] = {
			#include "resource/imager.png.data"
		};
		indigo_server_add_resource("/imager.png", imager_png, sizeof(imager_png), "image/png");
		// INDIGO Mount
		static unsigned char mount_html[] = {
			#include "resource/mount.html.data"
		};		indigo_server_add_resource("/mount.html", mount_html, sizeof(mount_html), "text/html");
		static unsigned char mount_png[] = {
			#include "resource/mount.png.data"
		};
		indigo_server_add_resource("/mount.png", mount_png, sizeof(mount_png), "image/png");
		static unsigned char celestial_js[] = {
			#include "resource/celestial.min.js.data"
		};
		indigo_server_add_resource("/celestial.min.js", celestial_js, sizeof(celestial_js), "text/javascript");
		static unsigned char d3_js[] = {
			#include "resource/d3.min.js.data"
		};
		indigo_server_add_resource("/d3.min.js", d3_js, sizeof(d3_js), "text/javascript");
		static unsigned char celestial_css[] = {
			#include "resource/celestial.css.data"
		};
		indigo_server_add_resource("/celestial.css", celestial_css, sizeof(celestial_css), "text/css");
		static unsigned char constellations_json[] = {
			#include "resource/data/constellations.json.data"
		};
		indigo_server_add_resource("/data/constellations.json", constellations_json, sizeof(constellations_json), "application/json; charset=utf-8");
		static unsigned char constellations_bounds_json[] = {
			#include "resource/data/constellations.bounds.json.data"
		};
		indigo_server_add_resource("/data/constellations.bounds.json", constellations_bounds_json, sizeof(constellations_bounds_json), "application/json; charset=utf-8");
		static unsigned char mv_json[] = {
			#include "resource/data/mw.json.data"
		};
		indigo_server_add_resource("/data/mw.json", mv_json, sizeof(mv_json), "application/json; charset=utf-8");
		static unsigned char planets_json[] = {
			#include "resource/data/planets.json.data"
		};
		indigo_server_add_resource("/data/planets.json", planets_json, sizeof(planets_json), "application/json; charset=utf-8");
		star_data = indigo_add_star_json_resource(6);
		dso_data = indigo_add_dso_json_resource(10);
		constellation_data = indigo_add_constellations_lines_json_resource();
		// INDIGO Guider
		static unsigned char guider_html[] = {
			#include "resource/guider.html.data"
		};
		indigo_server_add_resource("/guider.html", guider_html, sizeof(guider_html), "text/html");
		static unsigned char guider_png[] = {
			#include "resource/guider.png.data"
		};
		indigo_server_add_resource("/guider.png", guider_png, sizeof(guider_png), "image/png");
		// INDIGO Script
		static unsigned char script_html[] = {
			#include "resource/script.html.data"
		};
		indigo_server_add_resource("/script.html", script_html, sizeof(script_html), "text/html");
		static unsigned char script_png[] = {
			#include "resource/script.png.data"
		};
		indigo_server_add_resource("/script.png", script_png, sizeof(script_png), "image/png");
	}

	indigo_server_add_file_resource("/log", "indigo.log", "text/plain; charset=UTF-8");

	if (!command_line_drivers) {
		for (static_drivers_count = 0; static_drivers[static_drivers_count]; static_drivers_count++) {
			indigo_add_driver(static_drivers[static_drivers_count], false, NULL);
		}
		char *last = strrchr(server_argv[0], '/');
		if (last) {
			char path[PATH_MAX];
			long len = last - server_argv[0];
			strncpy(path, server_argv[0], len);
			path[len] = 0;
			strcat(path, "/../share/indigo");
			add_drivers(path);
		}
		add_drivers("/usr/share/indigo");
		add_drivers("/usr/local/share/indigo");
	}
	indigo_attach_device(&server_device);

#ifdef INDIGO_LINUX
	indigo_server_start(NULL);
#endif
#ifdef INDIGO_MACOS
	if (!indigo_async((void * (*)(void *))indigo_server_start, NULL)) {
		INDIGO_ERROR(indigo_error("Error creating thread for server"));
	}
	runLoop = true;
	while (runLoop) {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, true);
	}
#endif

	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
		if (indigo_available_drivers[i].driver) {
			indigo_remove_driver(&indigo_available_drivers[i]);
		}
	}
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_subprocesses[i].thread_started)
			indigo_kill_subprocess(&indigo_available_subprocesses[i]);
	}
	indigo_detach_device(&server_device);
	indigo_stop();
	indigo_server_remove_resources();
	if (star_data)
		free(star_data);
	if (dso_data)
		free(dso_data);
	if (constellation_data)
		free(constellation_data);
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_servers[i].thread_started)
			indigo_disconnect_server(&indigo_available_servers[i]);
	}
	exit(EXIT_SUCCESS);
}

static void signal_handler(int signo) {
	if (signo == SIGCHLD) {
		int status;
		while ((waitpid(-1, &status, WNOHANG)) > 0);
		return;
	}
	if (server_pid == 0) {
		/* SIGINT is delivered twise with CTRL-C
		   this leads to freeze during shutdown so
		   we ignore the second SIGINT */
		if (signo == SIGINT) {
			signal(SIGINT, SIG_IGN);
		}
		INDIGO_LOG(indigo_log("Shutdown initiated (signal %d)...", signo));
		indigo_server_shutdown();
#ifdef INDIGO_MACOS
		runLoop = false;
#endif
	} else {
		INDIGO_LOG(indigo_log("Signal %d received...", signo));
		keep_server_running = (signo == SIGHUP);
		if (use_sigkill)
			kill(server_pid, SIGKILL);
		else
			kill(server_pid, SIGINT);
		use_sigkill = true;
	}
}

int main(int argc, const char * argv[]) {
	bool do_fork = true;
	server_argv[0] = argv[0];
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--") || !strcmp(argv[i], "--do-not-fork")) {
			do_fork = false;
		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--use-syslog")) {
			indigo_use_syslog = true;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("INDIGO server v.%d.%d-%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);
			printf("usage: %s [-h | --help]\n", argv[0]);
			printf("       %s [options] indigo_driver_name indigo_driver_name ...\n", argv[0]);
			printf("options:\n"
			       "       --  | --do-not-fork\n"
			       "       -l  | --use-syslog\n"
			       "       -p  | --port port                     (default: 7624)\n"
			       "       -b  | --bonjour name                  (default: hostname)\n"
			       "       -T  | --master-token token            (master token for devce access default: 0 = none)\n"
			       "       -a  | --acl-file file\n"
			       "       -b- | --disable-bonjour\n"
			       "       -u- | --disable-blob-urls\n"
			       "       -d- | --disable-blob-buffering\n"
			       "       -C  | --enable-blob-compression\n"
			       "       -w- | --disable-web-apps\n"
			       "       -c- | --disable-control-panel\n"
#ifdef RPI_MANAGEMENT
			       "       -f  | --enable-rpi-management\n"
#endif /* RPI_MANAGEMENT */
			       "       -v  | --enable-info\n"
			       "       -vv | --enable-debug\n"
						 "       -vvb| --enable-trace-bus\n"
			       "       -vvv| --enable-trace\n"
			       "       -r  | --remote-server host[:port]     (default port: 7624)\n"
			       "       -x  | --enable-blob-proxy\n"
			       "       -i  | --indi-driver driver_executable\n"
			);
			return 0;
		} else {
			server_argv[server_argc++] = argv[i];
		}
	}
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGCHLD, signal_handler);
	if (do_fork) {
		while(keep_server_running) {
			server_pid = fork();
			if (server_pid == -1) {
				INDIGO_ERROR(indigo_error("Server start failed!"));
				return EXIT_FAILURE;
			} else if (server_pid == 0) {
#ifdef INDIGO_LINUX
				/* Preserve process name for logging */
				char *name = strrchr(server_argv[0], '/');
				if (name != NULL) {
					name++;
				} else {
					name = (char *)server_argv[0];
				}
				strncpy(indigo_log_name, name, 255);

				/* Change process name for user convinience */
				char *server_string = strstr(server_argv[0], "indigo_server");
				if (server_string) {
					static char process_name[] = "indigo_worker";
					int len = strlen(server_argv[0]);
					strcpy(server_string, process_name);
					prctl(PR_SET_PDEATHSIG, SIGINT, 0, 0, 0);
					/* Linux requires additional step to change process name */
					prctl(PR_SET_NAME, process_name, 0, 0, 0);
				}
#endif
				server_main();
				return EXIT_SUCCESS;
			} else {
				if (waitpid(server_pid, NULL, 0) == -1 ) {
					INDIGO_ERROR(indigo_error("waitpid() failed with error: %s", strerror(errno)));
					return EXIT_FAILURE;
				}
				use_sigkill = false;
				if (keep_server_running) {
					INDIGO_LOG(indigo_log("Shutdown complete! Starting up..."));
					indigo_usleep(2 * ONE_SECOND_DELAY);
				}
			}
		}
		INDIGO_LOG(indigo_log("Shutdown complete! See you!"));
	} else {
		server_main();
	}
}
