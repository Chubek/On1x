# On1x C API examples

- `embed_incident_digest.c` builds a small host, creates sample log files, loads `incident_digest.on1x`, and calls the exported `Run` function.
- `embed_rollout_policy.c` installs a host-owned `Host` module, loads `rollout_policy.on1x`, and repeatedly calls the exported `Decide` function with different service contexts.
