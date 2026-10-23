// Audit-only reproducer: compile the unchanged library source to exercise its internal JSON parser.
#include <stdio.h>
#include <string.h>
#include "../../../indigo_drivers/shared/primalucelib/primalucelib.c"
int main(void) {
 primalucelib_link link = { 0 };
 link.driver_name = "audit-prefix";
 strcpy(link.response, "{\"res\":{\"get\":{\"MOT1\":{\"ABS_POS\":123}}}}");
 if (!primalucelib_parse_reply(&link)) { return 2; }
 char **path = PRIMALUCE_REPLY_FOCUSER_POSITION_STEP;
 printf("Missing ABS_POS_STEP accepted as present: %d, value: %.0f\n", primalucelib_reply_has(&link, path), primalucelib_reply_number(&link, path));
 return primalucelib_reply_has(&link, path) ? 1 : 0;
}
