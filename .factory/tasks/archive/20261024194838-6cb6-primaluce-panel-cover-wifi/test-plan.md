# Test and regression plan

Status: final

Unit: `lan_config_restarts_only_on_a_change`, `wifi_modes_select_the_lan_mode_first`,
`wifi_credentials_are_checked_before_sending`, also under ASan/UBSan. Simulator suites: GIOTTO `wifi_credentials`,
`wifi_modes`, `wifi_mode_refused`; ALTO `wifi_credentials`, `wifi_modes`, `wifi_restart_waits_for_the_cover`; focuser
`controller_settings` and `wifi_station_mode` extended (both fail on 3.0.0.41). Hardware: driver sessions on the ALTO1
and the GIOTTO through `indigo_prop_tool`, every value restored. The HTTP path is covered by review only.
