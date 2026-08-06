# On1x examples

- `examples/scripts/incident_digest.on1x` shows a real operational script that scans log files and writes a Markdown digest.
- `examples/scripts/rollout_policy.on1x` shows a host-driven rollout policy script meant to be embedded and invoked from C.
- `examples/api/embed_incident_digest.c` embeds On1x as an automation engine and drives the log-digest script from a C host.
- `examples/api/embed_rollout_policy.c` installs a host module, loads a policy script, and lets On1x make deployment decisions.
