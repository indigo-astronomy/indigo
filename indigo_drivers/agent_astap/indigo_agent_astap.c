// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASTAP plate solver agent
 \file indigo_agent_astap.c
 */

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME	"indigo_agent_astap"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <jpeglib.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_platesolver.h>

#include "indigo_agent_astap.h"

#define INDEX_BASE_URL "https://indigo-astronomy.github.io/astap/star_databases"

typedef struct _index_data {
	char *name;
	char *path;
	short *files;
	char *size;
	double fov_min, fov_max;
} index_data;

static short W08_files[] = { 101, 0 };

static short H17_files[] = {
	101, 201, 202, 203, 301, 302, 303, 304, 305, 306, 307, 308, 309, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
	412, 413, 414, 415, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520,
	521, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623,
	624, 625, 626, 627, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720,
	721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811,
	812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831, 832, 833, 834, 835,
	836, 837, 838, 901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918, 919, 920, 921,
	922, 923, 924, 925, 926, 927, 928, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943, 1001,
	1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021,
	1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038, 1039, 1040, 1041,
	1042, 1043, 1044, 1045, 1046, 1047, 1048, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113,
	1114, 1115, 1116, 1117, 1118, 1119, 1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129, 1130, 1131, 1132, 1133,
	1134, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1152, 1201,
	1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214, 1215, 1216, 1217, 1218, 1219, 1220, 1221,
	1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1238, 1239, 1240, 1241,
	1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1301, 1302, 1303, 1304, 1305,
	1306, 1307, 1308, 1309, 1310, 1311, 1312, 1313, 1314, 1315, 1316, 1317, 1318, 1319, 1320, 1321, 1322, 1323, 1324, 1325,
	1326, 1327, 1328, 1329, 1330, 1331, 1332, 1333, 1334, 1335, 1336, 1337, 1338, 1339, 1340, 1341, 1342, 1343, 1344, 1345,
	1346, 1347, 1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355, 1356, 1357, 1358, 1359, 1360, 1401, 1402, 1403, 1404, 1405,
	1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413, 1414, 1415, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1424, 1425,
	1426, 1427, 1428, 1429, 1430, 1431, 1432, 1433, 1434, 1435, 1436, 1437, 1438, 1439, 1440, 1441, 1442, 1443, 1444, 1445,
	1446, 1447, 1448, 1449, 1450, 1451, 1452, 1453, 1454, 1455, 1456, 1457, 1458, 1459, 1460, 1461, 1462, 1463, 1501, 1502,
	1503, 1504, 1505, 1506, 1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1519, 1520, 1521, 1522,
	1523, 1524, 1525, 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534, 1535, 1536, 1537, 1538, 1539, 1540, 1541, 1542,
	1543, 1544, 1545, 1546, 1547, 1548, 1549, 1550, 1551, 1552, 1553, 1554, 1555, 1556, 1557, 1558, 1559, 1560, 1561, 1562,
	1563, 1564, 1565, 1601, 1602, 1603, 1604, 1605, 1606, 1607, 1608, 1609, 1610, 1611, 1612, 1613, 1614, 1615, 1616, 1617,
	1618, 1619, 1620, 1621, 1622, 1623, 1624, 1625, 1626, 1627, 1628, 1629, 1630, 1631, 1632, 1633, 1634, 1635, 1636, 1637,
	1638, 1639, 1640, 1641, 1642, 1643, 1644, 1645, 1646, 1647, 1648, 1649, 1650, 1651, 1652, 1653, 1654, 1655, 1656, 1657,
	1658, 1659, 1660, 1661, 1662, 1663, 1664, 1665, 1666, 1667, 1701, 1702, 1703, 1704, 1705, 1706, 1707, 1708, 1709, 1710,
	1711, 1712, 1713, 1714, 1715, 1716, 1717, 1718, 1719, 1720, 1721, 1722, 1723, 1724, 1725, 1726, 1727, 1728, 1729, 1730,
	1731, 1732, 1733, 1734, 1735, 1736, 1737, 1738, 1739, 1740, 1741, 1742, 1743, 1744, 1745, 1746, 1747, 1748, 1749, 1750,
	1751, 1752, 1753, 1754, 1755, 1756, 1757, 1758, 1759, 1760, 1761, 1762, 1763, 1764, 1765, 1766, 1767, 1768, 1801, 1802,
	1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814, 1815, 1816, 1817, 1818, 1819, 1820, 1821, 1822,
	1823, 1824, 1825, 1826, 1827, 1828, 1829, 1830, 1831, 1832, 1833, 1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841, 1842,
	1843, 1844, 1845, 1846, 1847, 1848, 1849, 1850, 1851, 1852, 1853, 1854, 1855, 1856, 1857, 1858, 1859, 1860, 1861, 1862,
	1863, 1864, 1865, 1866, 1867, 1868, 1869, 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913,
	1914, 1915, 1916, 1917, 1918, 1919, 1920, 1921, 1922, 1923, 1924, 1925, 1926, 1927, 1928, 1929, 1930, 1931, 1932, 1933,
	1934, 1935, 1936, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946, 1947, 1948, 1949, 1950, 1951, 1952, 1953,
	1954, 1955, 1956, 1957, 1958, 1959, 1960, 1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 2001, 2002, 2003, 2004,
	2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024,
	2025, 2026, 2027, 2028, 2029, 2030, 2031, 2032, 2033, 2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044,
	2045, 2046, 2047, 2048, 2049, 2050, 2051, 2052, 2053, 2054, 2055, 2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 2064,
	2065, 2066, 2067, 2068, 2101, 2102, 2103, 2104, 2105, 2106, 2107, 2108, 2109, 2110, 2111, 2112, 2113, 2114, 2115, 2116,
	2117, 2118, 2119, 2120, 2121, 2122, 2123, 2124, 2125, 2126, 2127, 2128, 2129, 2130, 2131, 2132, 2133, 2134, 2135, 2136,
	2137, 2138, 2139, 2140, 2141, 2142, 2143, 2144, 2145, 2146, 2147, 2148, 2149, 2150, 2151, 2152, 2153, 2154, 2155, 2156,
	2157, 2158, 2159, 2160, 2161, 2162, 2163, 2164, 2165, 2166, 2167, 2201, 2202, 2203, 2204, 2205, 2206, 2207, 2208, 2209,
	2210, 2211, 2212, 2213, 2214, 2215, 2216, 2217, 2218, 2219, 2220, 2221, 2222, 2223, 2224, 2225, 2226, 2227, 2228, 2229,
	2230, 2231, 2232, 2233, 2234, 2235, 2236, 2237, 2238, 2239, 2240, 2241, 2242, 2243, 2244, 2245, 2246, 2247, 2248, 2249,
	2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 2259, 2260, 2261, 2262, 2263, 2264, 2265, 2301, 2302, 2303, 2304,
	2305, 2306, 2307, 2308, 2309, 2310, 2311, 2312, 2313, 2314, 2315, 2316, 2317, 2318, 2319, 2320, 2321, 2322, 2323, 2324,
	2325, 2326, 2327, 2328, 2329, 2330, 2331, 2332, 2333, 2334, 2335, 2336, 2337, 2338, 2339, 2340, 2341, 2342, 2343, 2344,
	2345, 2346, 2347, 2348, 2349, 2350, 2351, 2352, 2353, 2354, 2355, 2356, 2357, 2358, 2359, 2360, 2361, 2362, 2363, 2401,
	2402, 2403, 2404, 2405, 2406, 2407, 2408, 2409, 2410, 2411, 2412, 2413, 2414, 2415, 2416, 2417, 2418, 2419, 2420, 2421,
	2422, 2423, 2424, 2425, 2426, 2427, 2428, 2429, 2430, 2431, 2432, 2433, 2434, 2435, 2436, 2437, 2438, 2439, 2440, 2441,
	2442, 2443, 2444, 2445, 2446, 2447, 2448, 2449, 2450, 2451, 2452, 2453, 2454, 2455, 2456, 2457, 2458, 2459, 2460, 2501,
	2502, 2503, 2504, 2505, 2506, 2507, 2508, 2509, 2510, 2511, 2512, 2513, 2514, 2515, 2516, 2517, 2518, 2519, 2520, 2521,
	2522, 2523, 2524, 2525, 2526, 2527, 2528, 2529, 2530, 2531, 2532, 2533, 2534, 2535, 2536, 2537, 2538, 2539, 2540, 2541,
	2542, 2543, 2544, 2545, 2546, 2547, 2548, 2549, 2550, 2551, 2552, 2553, 2554, 2555, 2556, 2601, 2602, 2603, 2604, 2605,
	2606, 2607, 2608, 2609, 2610, 2611, 2612, 2613, 2614, 2615, 2616, 2617, 2618, 2619, 2620, 2621, 2622, 2623, 2624, 2625,
	2626, 2627, 2628, 2629, 2630, 2631, 2632, 2633, 2634, 2635, 2636, 2637, 2638, 2639, 2640, 2641, 2642, 2643, 2644, 2645,
	2646, 2647, 2648, 2649, 2650, 2651, 2652, 2701, 2702, 2703, 2704, 2705, 2706, 2707, 2708, 2709, 2710, 2711, 2712, 2713,
	2714, 2715, 2716, 2717, 2718, 2719, 2720, 2721, 2722, 2723, 2724, 2725, 2726, 2727, 2728, 2729, 2730, 2731, 2732, 2733,
	2734, 2735, 2736, 2737, 2738, 2739, 2740, 2741, 2742, 2743, 2744, 2745, 2746, 2747, 2748, 2801, 2802, 2803, 2804, 2805,
	2806, 2807, 2808, 2809, 2810, 2811, 2812, 2813, 2814, 2815, 2816, 2817, 2818, 2819, 2820, 2821, 2822, 2823, 2824, 2825,
	2826, 2827, 2828, 2829, 2830, 2831, 2832, 2833, 2834, 2835, 2836, 2837, 2838, 2839, 2840, 2841, 2842, 2843, 2901, 2902,
	2903, 2904, 2905, 2906, 2907, 2908, 2909, 2910, 2911, 2912, 2913, 2914, 2915, 2916, 2917, 2918, 2919, 2920, 2921, 2922,
	2923, 2924, 2925, 2926, 2927, 2928, 2929, 2930, 2931, 2932, 2933, 2934, 2935, 2936, 2937, 2938, 3001, 3002, 3003, 3004,
	3005, 3006, 3007, 3008, 3009, 3010, 3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020, 3021, 3022, 3023, 3024,
	3025, 3026, 3027, 3028, 3029, 3030, 3031, 3032, 3033, 3101, 3102, 3103, 3104, 3105, 3106, 3107, 3108, 3109, 3110, 3111,
	3112, 3113, 3114, 3115, 3116, 3117, 3118, 3119, 3120, 3121, 3122, 3123, 3124, 3125, 3126, 3127, 3201, 3202, 3203, 3204,
	3205, 3206, 3207, 3208, 3209, 3210, 3211, 3212, 3213, 3214, 3215, 3216, 3217, 3218, 3219, 3220, 3221, 3301, 3302, 3303,
	3304, 3305, 3306, 3307, 3308, 3309, 3310, 3311, 3312, 3313, 3314, 3315, 3401, 3402, 3403, 3404, 3405, 3406, 3407, 3408,
	3409, 3501, 3502, 3503, 3601, 0
};

static short V17_files[] = {
	101, 201, 202, 203, 204, 301, 302, 303, 304, 305, 306, 307, 308, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
	412, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 601, 602, 603, 604, 605, 606, 607,
	608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711,
	712, 713, 714, 715, 716, 717, 718, 719, 720, 721, 722, 723, 724, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811,
	812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 901, 902, 903, 904, 905, 906, 907,
	908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 928, 929, 930, 931,
	932, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019,
	1020, 1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1032, 1101, 1102, 1103, 1104, 1105, 1106, 1107,
	1108, 1109, 1110, 1111, 1112, 1113, 1114, 1115, 1116, 1117, 1118, 1119, 1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127,
	1128, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214, 1215, 1216, 1217, 1218, 1219,
	1220, 1221, 1222, 1223, 1224, 1301, 1302, 1303, 1304, 1305, 1306, 1307, 1308, 1309, 1310, 1311, 1312, 1313, 1314, 1315,
	1316, 1317, 1318, 1319, 1320, 1401, 1402, 1403, 1404, 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413, 1414, 1415,
	1416, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1508, 1509, 1510, 1511, 1512, 1601, 1602, 1603, 1604, 1605, 1606, 1607,
	1608, 1701, 1702, 1703, 1704, 1801, 0
};

static short H18_files[] = {
	101, 201, 202, 203, 301, 302, 303, 304, 305, 306, 307, 308, 309, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
	412, 413, 414, 415, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520,
	521, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623,
	624, 625, 626, 627, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720,
	721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811,
	812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831, 832, 833, 834, 835,
	836, 837, 838, 901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918, 919, 920, 921,
	922, 923, 924, 925, 926, 927, 928, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943, 1001,
	1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021,
	1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038, 1039, 1040, 1041,
	1042, 1043, 1044, 1045, 1046, 1047, 1048, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113,
	1114, 1115, 1116, 1117, 1118, 1119, 1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129, 1130, 1131, 1132, 1133,
	1134, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1152, 1201,
	1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214, 1215, 1216, 1217, 1218, 1219, 1220, 1221,
	1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1238, 1239, 1240, 1241,
	1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1301, 1302, 1303, 1304, 1305,
	1306, 1307, 1308, 1309, 1310, 1311, 1312, 1313, 1314, 1315, 1316, 1317, 1318, 1319, 1320, 1321, 1322, 1323, 1324, 1325,
	1326, 1327, 1328, 1329, 1330, 1331, 1332, 1333, 1334, 1335, 1336, 1337, 1338, 1339, 1340, 1341, 1342, 1343, 1344, 1345,
	1346, 1347, 1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355, 1356, 1357, 1358, 1359, 1360, 1401, 1402, 1403, 1404, 1405,
	1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413, 1414, 1415, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1424, 1425,
	1426, 1427, 1428, 1429, 1430, 1431, 1432, 1433, 1434, 1435, 1436, 1437, 1438, 1439, 1440, 1441, 1442, 1443, 1444, 1445,
	1446, 1447, 1448, 1449, 1450, 1451, 1452, 1453, 1454, 1455, 1456, 1457, 1458, 1459, 1460, 1461, 1462, 1463, 1501, 1502,
	1503, 1504, 1505, 1506, 1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1519, 1520, 1521, 1522,
	1523, 1524, 1525, 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534, 1535, 1536, 1537, 1538, 1539, 1540, 1541, 1542,
	1543, 1544, 1545, 1546, 1547, 1548, 1549, 1550, 1551, 1552, 1553, 1554, 1555, 1556, 1557, 1558, 1559, 1560, 1561, 1562,
	1563, 1564, 1565, 1601, 1602, 1603, 1604, 1605, 1606, 1607, 1608, 1609, 1610, 1611, 1612, 1613, 1614, 1615, 1616, 1617,
	1618, 1619, 1620, 1621, 1622, 1623, 1624, 1625, 1626, 1627, 1628, 1629, 1630, 1631, 1632, 1633, 1634, 1635, 1636, 1637,
	1638, 1639, 1640, 1641, 1642, 1643, 1644, 1645, 1646, 1647, 1648, 1649, 1650, 1651, 1652, 1653, 1654, 1655, 1656, 1657,
	1658, 1659, 1660, 1661, 1662, 1663, 1664, 1665, 1666, 1667, 1701, 1702, 1703, 1704, 1705, 1706, 1707, 1708, 1709, 1710,
	1711, 1712, 1713, 1714, 1715, 1716, 1717, 1718, 1719, 1720, 1721, 1722, 1723, 1724, 1725, 1726, 1727, 1728, 1729, 1730,
	1731, 1732, 1733, 1734, 1735, 1736, 1737, 1738, 1739, 1740, 1741, 1742, 1743, 1744, 1745, 1746, 1747, 1748, 1749, 1750,
	1751, 1752, 1753, 1754, 1755, 1756, 1757, 1758, 1759, 1760, 1761, 1762, 1763, 1764, 1765, 1766, 1767, 1768, 1801, 1802,
	1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814, 1815, 1816, 1817, 1818, 1819, 1820, 1821, 1822,
	1823, 1824, 1825, 1826, 1827, 1828, 1829, 1830, 1831, 1832, 1833, 1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841, 1842,
	1843, 1844, 1845, 1846, 1847, 1848, 1849, 1850, 1851, 1852, 1853, 1854, 1855, 1856, 1857, 1858, 1859, 1860, 1861, 1862,
	1863, 1864, 1865, 1866, 1867, 1868, 1869, 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913,
	1914, 1915, 1916, 1917, 1918, 1919, 1920, 1921, 1922, 1923, 1924, 1925, 1926, 1927, 1928, 1929, 1930, 1931, 1932, 1933,
	1934, 1935, 1936, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946, 1947, 1948, 1949, 1950, 1951, 1952, 1953,
	1954, 1955, 1956, 1957, 1958, 1959, 1960, 1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 2001, 2002, 2003, 2004,
	2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024,
	2025, 2026, 2027, 2028, 2029, 2030, 2031, 2032, 2033, 2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044,
	2045, 2046, 2047, 2048, 2049, 2050, 2051, 2052, 2053, 2054, 2055, 2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 2064,
	2065, 2066, 2067, 2068, 2101, 2102, 2103, 2104, 2105, 2106, 2107, 2108, 2109, 2110, 2111, 2112, 2113, 2114, 2115, 2116,
	2117, 2118, 2119, 2120, 2121, 2122, 2123, 2124, 2125, 2126, 2127, 2128, 2129, 2130, 2131, 2132, 2133, 2134, 2135, 2136,
	2137, 2138, 2139, 2140, 2141, 2142, 2143, 2144, 2145, 2146, 2147, 2148, 2149, 2150, 2151, 2152, 2153, 2154, 2155, 2156,
	2157, 2158, 2159, 2160, 2161, 2162, 2163, 2164, 2165, 2166, 2167, 2201, 2202, 2203, 2204, 2205, 2206, 2207, 2208, 2209,
	2210, 2211, 2212, 2213, 2214, 2215, 2216, 2217, 2218, 2219, 2220, 2221, 2222, 2223, 2224, 2225, 2226, 2227, 2228, 2229,
	2230, 2231, 2232, 2233, 2234, 2235, 2236, 2237, 2238, 2239, 2240, 2241, 2242, 2243, 2244, 2245, 2246, 2247, 2248, 2249,
	2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 2259, 2260, 2261, 2262, 2263, 2264, 2265, 2301, 2302, 2303, 2304,
	2305, 2306, 2307, 2308, 2309, 2310, 2311, 2312, 2313, 2314, 2315, 2316, 2317, 2318, 2319, 2320, 2321, 2322, 2323, 2324,
	2325, 2326, 2327, 2328, 2329, 2330, 2331, 2332, 2333, 2334, 2335, 2336, 2337, 2338, 2339, 2340, 2341, 2342, 2343, 2344,
	2345, 2346, 2347, 2348, 2349, 2350, 2351, 2352, 2353, 2354, 2355, 2356, 2357, 2358, 2359, 2360, 2361, 2362, 2363, 2401,
	2402, 2403, 2404, 2405, 2406, 2407, 2408, 2409, 2410, 2411, 2412, 2413, 2414, 2415, 2416, 2417, 2418, 2419, 2420, 2421,
	2422, 2423, 2424, 2425, 2426, 2427, 2428, 2429, 2430, 2431, 2432, 2433, 2434, 2435, 2436, 2437, 2438, 2439, 2440, 2441,
	2442, 2443, 2444, 2445, 2446, 2447, 2448, 2449, 2450, 2451, 2452, 2453, 2454, 2455, 2456, 2457, 2458, 2459, 2460, 2501,
	2502, 2503, 2504, 2505, 2506, 2507, 2508, 2509, 2510, 2511, 2512, 2513, 2514, 2515, 2516, 2517, 2518, 2519, 2520, 2521,
	2522, 2523, 2524, 2525, 2526, 2527, 2528, 2529, 2530, 2531, 2532, 2533, 2534, 2535, 2536, 2537, 2538, 2539, 2540, 2541,
	2542, 2543, 2544, 2545, 2546, 2547, 2548, 2549, 2550, 2551, 2552, 2553, 2554, 2555, 2556, 2601, 2602, 2603, 2604, 2605,
	2606, 2607, 2608, 2609, 2610, 2611, 2612, 2613, 2614, 2615, 2616, 2617, 2618, 2619, 2620, 2621, 2622, 2623, 2624, 2625,
	2626, 2627, 2628, 2629, 2630, 2631, 2632, 2633, 2634, 2635, 2636, 2637, 2638, 2639, 2640, 2641, 2642, 2643, 2644, 2645,
	2646, 2647, 2648, 2649, 2650, 2651, 2652, 2701, 2702, 2703, 2704, 2705, 2706, 2707, 2708, 2709, 2710, 2711, 2712, 2713,
	2714, 2715, 2716, 2717, 2718, 2719, 2720, 2721, 2722, 2723, 2724, 2725, 2726, 2727, 2728, 2729, 2730, 2731, 2732, 2733,
	2734, 2735, 2736, 2737, 2738, 2739, 2740, 2741, 2742, 2743, 2744, 2745, 2746, 2747, 2748, 2801, 2802, 2803, 2804, 2805,
	2806, 2807, 2808, 2809, 2810, 2811, 2812, 2813, 2814, 2815, 2816, 2817, 2818, 2819, 2820, 2821, 2822, 2823, 2824, 2825,
	2826, 2827, 2828, 2829, 2830, 2831, 2832, 2833, 2834, 2835, 2836, 2837, 2838, 2839, 2840, 2841, 2842, 2843, 2901, 2902,
	2903, 2904, 2905, 2906, 2907, 2908, 2909, 2910, 2911, 2912, 2913, 2914, 2915, 2916, 2917, 2918, 2919, 2920, 2921, 2922,
	2923, 2924, 2925, 2926, 2927, 2928, 2929, 2930, 2931, 2932, 2933, 2934, 2935, 2936, 2937, 2938, 3001, 3002, 3003, 3004,
	3005, 3006, 3007, 3008, 3009, 3010, 3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020, 3021, 3022, 3023, 3024,
	3025, 3026, 3027, 3028, 3029, 3030, 3031, 3032, 3033, 3101, 3102, 3103, 3104, 3105, 3106, 3107, 3108, 3109, 3110, 3111,
	3112, 3113, 3114, 3115, 3116, 3117, 3118, 3119, 3120, 3121, 3122, 3123, 3124, 3125, 3126, 3127, 3201, 3202, 3203, 3204,
	3205, 3206, 3207, 3208, 3209, 3210, 3211, 3212, 3213, 3214, 3215, 3216, 3217, 3218, 3219, 3220, 3221, 3301, 3302, 3303,
	3304, 3305, 3306, 3307, 3308, 3309, 3310, 3311, 3312, 3313, 3314, 3315, 3401, 3402, 3403, 3404, 3405, 3406, 3407, 3408,
	3409, 3501, 3502, 3503, 3601, 0
};

static index_data astap_index[] = {
	{ "W08", "%s/W08/w08_%04d.001", W08_files, "496K", 20.0, 180.0 },
	{ "H17", "%s/H17/h17_%04d.1476", H17_files, "535M", 1, 10 },
	{ "V17", "%s/V17/v17_%04d.290", V17_files, "726M", 1, 20 },
	{ "H18", "%s/H18/h18_%04d.1476", H18_files, "1G", 0.25, 10 },
	{ NULL }
};
static char base_dir[512];

#define ASTAP_DEVICE_PRIVATE_DATA				((astap_private_data *)device->private_data)
#define ASTAP_CLIENT_PRIVATE_DATA				((astap_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_ASTAP_INDEX_PROPERTY	(ASTAP_DEVICE_PRIVATE_DATA->index_property)
#define AGENT_ASTAP_INDEX_W08_ITEM  (AGENT_ASTAP_INDEX_PROPERTY->items+0)
#define AGENT_ASTAP_INDEX_V17_ITEM  (AGENT_ASTAP_INDEX_PROPERTY->items+1)

typedef struct {
	platesolver_private_data platesolver;
	indigo_property *index_property;
	indigo_timer *time_limit;
	int frame_width;
	int frame_height;
	bool abort_requested;
	pid_t pid;
} astap_private_data;

// --------------------------------------------------------------------------------

extern char **environ;

static void parse_line(indigo_device *device, char *line) {
	char *s = strchr(line, '\n');
	if (s)
		*s = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", line);
	if ((s = strstr(line, "PLTSOLVD="))) {
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = s[9] != 'T';
	} else if ((s = strstr(line, "CRPIX1="))) {
		ASTAP_DEVICE_PRIVATE_DATA->frame_width = 2 * (int)atof(s + 7);
	} else if ((s = strstr(line, "CRPIX1="))) {
		ASTAP_DEVICE_PRIVATE_DATA->frame_height = 2 * (int)atof(s + 7);
	} else if ((s = strstr(line, "CRVAL1="))) {
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = atof(s + 7) / 15.0;
	} else if ((s = strstr(line, "CRVAL2="))) {
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = atof(s + 7);
		if (AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target == 0) {
			indigo_j2k_to_jnow(&AGENT_PLATESOLVER_WCS_RA_ITEM->number.value, &AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value);
			AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 0;
		} else {
			AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 2000;
		}
	} else if ((s = strstr(line, "CROTA1="))) {
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = atof(s + 7);
	} else if ((s = strstr(line, "CROTA2="))) {
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = -(AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value + atof(s + 7)) / 2.0;
	} else if ((s = strstr(line, "CD1_1="))) {
		double d = atof(s + 6);
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = d;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = d >= 0 ? -1 : 1;
	} else if ((s = strstr(line, "CD2_2="))) {
		double d = atof(s + 6);
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = (fabs(AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value) + fabs(d)) / 2.0;
		AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = ASTAP_DEVICE_PRIVATE_DATA->frame_width * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
		AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = ASTAP_DEVICE_PRIVATE_DATA->frame_height * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value * (d >= 0 ? 1 : -1);
	} else if ((s = strstr(line, "ERROR="))) {
		indigo_send_message(device, s + 6);
		indigo_error("ASTAP Error: %s", s + 8);
	} else if ((s = strstr(line, "WARNING="))) {
		indigo_send_message(device, s + 8);
		indigo_error("ASTAP Warning: %s", s + 8);
	} else if ((s = strstr(line, "COMMENT="))) {
		indigo_log("ASTAP Comment: %s", s + 8);
	}
	if ((s = strstr(line, "Solved in "))) {
		indigo_send_message(device, "Solved in %gs", atof(s + 10));
	}
}

static void time_limit_timer(indigo_device *device) {
	kill(-ASTAP_DEVICE_PRIVATE_DATA->pid, SIGTERM);
	ASTAP_DEVICE_PRIVATE_DATA->pid = 0;
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
	indigo_send_message(device, "Time limit reached!");
}

static bool execute_command(indigo_device *device, char *command, ...) {
	char buffer[8 * 1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);

	ASTAP_DEVICE_PRIVATE_DATA->abort_requested = false;
	char command_buf[8 * 1024];
	sprintf(command_buf, "%s 2>&1", buffer);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
	int pipe_stdout[2];
	if (pipe(pipe_stdout)) {
		return false;
	}
	switch (ASTAP_DEVICE_PRIVATE_DATA->pid = fork()) {
		case -1: {
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			indigo_send_message(device, "Failed to execute %s (%s)", command_buf, strerror(errno));
			return false;
		}
		case 0: {
			setpgid(0, 0);
			close(pipe_stdout[0]);
			dup2(pipe_stdout[1], STDOUT_FILENO);
			close(pipe_stdout[1]);
			execl("/bin/sh", "sh", "-c", buffer, NULL, environ);
			perror("execl");
			_exit(127);
		}
	}
	close(pipe_stdout[1]);
	if (!strncmp(command, "astap_cli", 9) && AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value > 0) {
		indigo_set_timer(device, AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value, time_limit_timer, &ASTAP_DEVICE_PRIVATE_DATA->time_limit);
	} else {
		ASTAP_DEVICE_PRIVATE_DATA->time_limit = NULL;
	}
	FILE *output = fdopen(pipe_stdout[0], "r");
	bool res = true;
	char *line = NULL;
	size_t size = 0;
	while (getline(&line, &size, output) >= 0)
		parse_line(device, line);
	if (line)
		free(line);
	fclose(output);
	indigo_cancel_timer(device, &ASTAP_DEVICE_PRIVATE_DATA->time_limit);
	ASTAP_DEVICE_PRIVATE_DATA->pid = 0;
	if (ASTAP_DEVICE_PRIVATE_DATA->abort_requested) {
		res = false;
		ASTAP_DEVICE_PRIVATE_DATA->abort_requested = false;
		indigo_send_message(device, "Aborted");
	}
	return res;
}

#define astap_save_config indigo_platesolver_save_config

static void astap_abort(indigo_device *device) {
	if (ASTAP_DEVICE_PRIVATE_DATA->pid) {
		ASTAP_DEVICE_PRIVATE_DATA->abort_requested = true;
		/* NB: To kill the whole process group with PID you should send kill signal to -PID (-1 * PID) */
		kill(-ASTAP_DEVICE_PRIVATE_DATA->pid, SIGTERM);
	}
}

static bool astap_solve(indigo_device *device, void *image, unsigned long image_size) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		char *ext = "raw";
		bool use_stdin = false;
		char *message = "";
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_SOLVING;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		if (!strncmp("SIMPLE", (const char *)image, 6)) {
			ext = "fits";
			const char *header = (const char *)image;
			while (strncmp(header, "END", 3) && header < (const char *)image + image_size) {
				if (sscanf(header, "NAXIS1  = %d", &ASTAP_DEVICE_PRIVATE_DATA->frame_width) == 0)
					if (sscanf(header, "NAXIS2  = %d", &ASTAP_DEVICE_PRIVATE_DATA->frame_height) == 1)
						break;
				header += 80;
			}
		} else if (((uint8_t *)image)[0] == 0xFF && ((uint8_t *)image)[1] == 0xD8 && ((uint8_t *)image)[2] == 0xFF) {
			ext = "jpeg";
			ASTAP_DEVICE_PRIVATE_DATA->frame_width = ASTAP_DEVICE_PRIVATE_DATA->frame_height = 0;
		} else if (!strncmp("RAW", (const char *)(image), 3)) {
			indigo_raw_header *header = (indigo_raw_header *)image;
			ASTAP_DEVICE_PRIVATE_DATA->frame_width = header->width;
			ASTAP_DEVICE_PRIVATE_DATA->frame_height = header->height;
			use_stdin = true;
		} else {
			image = NULL;
		}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		char base[512], file[512], ini[512];
		sprintf(base, "%s/%s_%lX", base_dir, "image", time(0));
		sprintf(file, "%s.%s", base, ext);
		sprintf(ini, "%s.ini", base);
#pragma clang diagnostic pop
		int handle = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			message = "Can't create temporary image file";
			goto cleanup;
		}
		indigo_write(handle, (const char *)image, image_size);
		close(handle);
		// execute astap plate solver
		char params[512] = "";
		int params_index = 0;
		params_index = sprintf(params, "-z %d", (int)AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value);
		if (AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -r %g", AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -ra %g", AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -spd %g", AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value + 90);
		}
		if (AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -s %d", (int)AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value > 0 && ASTAP_DEVICE_PRIVATE_DATA->frame_height > 0) {
			params_index += sprintf(params + params_index, " -fov %.1f", AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value * ASTAP_DEVICE_PRIVATE_DATA->frame_height);
		} else if (AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value < 0 && ASTAP_DEVICE_PRIVATE_DATA->frame_height > 0 && INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale > 0) {
			params_index += sprintf(params + params_index, " -fov %.1f", INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale * ASTAP_DEVICE_PRIVATE_DATA->frame_height);
		}
		for (int k = 0; k < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; k++) {
			indigo_item *item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + k;
			if (item->sw.value) {
				params_index += sprintf(params + params_index, " -d \"%s/%s\"", base_dir, item->name);
				AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = k;
				break;
			}
		}
		if (use_stdin) {
			if (!execute_command(device, "astap_cli %s -o \"%s\" -f stdin <\"%s\"", params, base, file))
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (execute_command(device, "astap_cli %s -f \"%s\"", params, file)) {
				FILE *output = fopen(ini, "r");
				if (output == NULL) {
					AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
					message = "Plate solver failed";
					goto cleanup;
				}
				char *line = NULL;
				size_t size = 0;
				while (getline(&line, &size, output) >= 0)
					parse_line(device, line);
				if (line)
					free(line);
				fclose(output);
			} else {
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	cleanup:
		/* globs do not work in quotes */
		execute_command(device, "rm -rf \"%s\"/image_*", base_dir);
		if (message[0] == '\0')
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		else
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, message);
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		return !INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
	return false;
}

static void sync_installed_indexes(indigo_device *device, char *dir, indigo_property *property) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char path[INDIGO_VALUE_SIZE];
	char url[INDIGO_VALUE_SIZE];
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		bool add = false;
		bool remove = false;
		char *name;
		for (int j = 0; (name = astap_index[j].name); j++) {
			if (!strncmp(name, item->name, 4)) {
				if (item->sw.value) {
					sprintf(path, "%s/%s", base_dir, astap_index[j].name);
					bool first_one = true;
					if (access(path, F_OK) != 0) {
						execute_command(device, "mkdir \"%s\"", path);
					}
					short *files = astap_index[j].files;
					for (int k = 0; files[k]; k++) {
						snprintf(path, sizeof((path)), astap_index[j].path, base_dir, files[k]);
						if (access(path, F_OK) == 0)
							continue;
						if (first_one) {
							first_one = false;
							indigo_send_message(device, "Downloading %s...", astap_index[j].name);
						}
						snprintf(url, sizeof((path)), astap_index[j].path, INDEX_BASE_URL, files[k]);
						if (!execute_command(device, "curl -L -s --compressed -o \"%s\" \"%s\"", path, url)) {
							item->sw.value = false;
							property->state = INDIGO_ALERT_STATE;
							indigo_update_property(device, property, strerror(errno));
							pthread_mutex_unlock(&mutex);
							return;
						}
						if (access(path, F_OK) != 0) {
							item->sw.value = false;
							property->state = INDIGO_ALERT_STATE;
							indigo_update_property(device, property, "Failed to download index %s", url);
							pthread_mutex_unlock(&mutex);
							return;
						}
					}
					indigo_send_message(device, "Done");
					add = true;
					continue;
				} else {
					sprintf(path, "%s/%s", base_dir, astap_index[j].name);
					if (access(path, F_OK) == 0) {
						indigo_send_message(device, "Removing %s...", astap_index[j].name);
						execute_command(device, "rm -rf \"%s\"", path);
						remove = true;
						indigo_send_message(device, "Done");
					}
				}
			}
		}
		if (add) {
			indigo_item *added_item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++;
			indigo_init_switch_item(added_item, item->name, item->label, false);
		}
		if (remove) {
			for (int j = 0; j < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; j++) {
				if (!strcmp(item->name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items[j].name)) {
					memmove(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + j, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + (j + 1), (AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j) * sizeof(indigo_item));
					AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count--;
					break;
				}
			}
		}
	}
	indigo_delete_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	indigo_property_sort_items(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, 0);
	AGENT_PLATESOLVER_USE_INDEX_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	astap_save_config(device);
	pthread_mutex_unlock(&mutex);
}

static void index_handler(indigo_device *device) {
	static int instances = 0;
	instances++;
	sync_installed_indexes(device, "index", AGENT_ASTAP_INDEX_PROPERTY);
	instances--;
	if (AGENT_ASTAP_INDEX_PROPERTY->state == INDIGO_BUSY_STATE)
		AGENT_ASTAP_INDEX_PROPERTY->state = instances ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_platesolver_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
		AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.min = AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.min = AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.max = 0;
		// -------------------------------------------------------------------------------- AGENT_ASTAP_INDEX
		char *name, label[INDIGO_VALUE_SIZE], path[INDIGO_VALUE_SIZE];
		bool present;
		AGENT_ASTAP_INDEX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ASTAP_INDEX_PROPERTY_NAME, "Index managememt", "Installed ASTAP indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 10);
		if (AGENT_ASTAP_INDEX_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_ASTAP_INDEX_PROPERTY->count = 0;
		for (int i = 0; (name = astap_index[i].name); i++) {
			sprintf(label, "Index %s (FOV %g-%g°, size %sB)", name, astap_index[i].fov_min, astap_index[i].fov_max, astap_index[i].size);
			present = true;
			short *files = astap_index[i].files;
			for (int k = 0; files[k]; k++) {
				snprintf(path, sizeof((path)), astap_index[i].path, base_dir, files[k]);
				if (access(path, F_OK) != 0) {
					present = false;
					break;
				}
			}
			indigo_init_switch_item(AGENT_ASTAP_INDEX_PROPERTY->items + i, name, label, present);
			if (present)
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, false);
			AGENT_ASTAP_INDEX_PROPERTY->count++;
		}
		indigo_property_sort_items(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, 0);
		// --------------------------------------------------------------------------------
		ASTAP_DEVICE_PRIVATE_DATA->platesolver.save_config = astap_save_config;
		ASTAP_DEVICE_PRIVATE_DATA->platesolver.solve = astap_solve;
		ASTAP_DEVICE_PRIVATE_DATA->platesolver.abort = astap_abort;
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTAP_INDEX_PROPERTY, property))
		indigo_define_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
	return indigo_platesolver_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTAP_INDEX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_ASTAP_INDEX
		indigo_property_copy_values(AGENT_ASTAP_INDEX_PROPERTY, property, false);
		AGENT_ASTAP_INDEX_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
		indigo_set_timer(device, 0, index_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_platesolver_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_ASTAP_INDEX_PROPERTY);
	return indigo_platesolver_device_detach(device);
}

// -------------------------------------------------------------------------------- Initialization

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

static void kill_children() {
	indigo_device *device = agent_device;
	if (device && device->private_data) {
		if (ASTAP_DEVICE_PRIVATE_DATA->pid)
			kill(-ASTAP_DEVICE_PRIVATE_DATA->pid, SIGTERM);
		indigo_device **additional_devices = DEVICE_CONTEXT->additional_device_instances;
		if (additional_devices) {
			for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {
				device = additional_devices[i];
				if (device && device->private_data && ASTAP_DEVICE_PRIVATE_DATA->pid)
					kill(-ASTAP_DEVICE_PRIVATE_DATA->pid, SIGTERM);
			}
		}
	}
}

indigo_result indigo_agent_astap(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ASTAP_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ASTAP_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_platesolver_client_attach,
		indigo_platesolver_define_property,
		indigo_platesolver_update_property,
		indigo_platesolver_delete_property,
		NULL,
		indigo_platesolver_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, ASTAP_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			if (!indigo_platesolver_validate_executable("astap_cli")) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASTAP is not available");
				return INDIGO_UNRESOLVED_DEPS;
			}
			last_action = action;
			char *env = getenv("INDIGO_CACHE_BASE");
			if (env) {
				snprintf(base_dir, sizeof((base_dir)), "%s/astap", env);
			} else {
				snprintf(base_dir, sizeof((base_dir)), "%s/.indigo/astap", getenv("HOME"));
			}
			mkdir(base_dir, 0777);
			void *private_data = indigo_safe_malloc(sizeof(astap_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			static bool first_time = true;
			if (first_time) {
				first_time = false;
				atexit(kill_children);
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
