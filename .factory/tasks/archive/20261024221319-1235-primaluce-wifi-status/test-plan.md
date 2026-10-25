# Test and regression plan

Status: final

Unit: `wifi_status_shows_where_the_controller_is` (station with escaped credentials, access point with a stale station
address, access point off, refused reply), also under ASan/UBSan. Suites: `property_contract`, the station-mode cases,
GIOTTO `wifi_credentials` (kept SSID, refused change, escaped password) and the focuser's credential cases.
