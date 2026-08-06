# On1x script examples

- `incident_digest.on1x` is runnable with the CLI in a capability-enabled host. Set `ON1X_RUN_EXAMPLE=1` to let it execute, and optionally set `ON1X_INCIDENT_INPUT` / `ON1X_INCIDENT_OUTPUT`.
- `rollout_policy.on1x` is designed for embedding. It returns a Table of exported functions so a host can evaluate it once and invoke policy entrypoints repeatedly.
