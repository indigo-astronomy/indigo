# Test and regression plan

Status: final

The case alone, then `tools/run_driver_test.py ccd_simulator`. The axis model is checked for both sides by `simulator_guider_offset_model_pulses_follow_mount_axes`, which sets
SIDE_OF_PIER to east and west explicitly; this case covers the side the run happens to get.
