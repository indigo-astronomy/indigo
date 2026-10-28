# Pentax K-m Raspberry Pi validation

Owner says K-m is connected to RPi, provides `ssh -A indigo@indigosky.local`, and asks to try. Continue prior non-interactive hardware authorization, quick grouped battery use and no physical unplug. Notify before use; no exposures in initial connection diagnostic. Camera sleeps, obtain readiness before further requests after preparation.

Local main checkout has unrelated dirty agent_solver files. Use isolated worktree /private/tmp/indigo-pentax-rpi based on published aabaed8ec; preserve those changes. Remote existing checkout /home/indigo/indigo is clean on e7cbb62a, with source3.0.0.1 and old prebuilt Pentax2.0.0.0. Do not overwrite remote checkout or installed packages. Initial USB node root-owned0644, so sudo -n is needed for write access and is available. No server was running.
