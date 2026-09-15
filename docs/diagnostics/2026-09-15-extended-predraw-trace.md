# Extended PRE boundary trace — 2026-09-15

An authorized extended server-side op trace was run across sequence numbers
7000–8200 with the logical SDL mode correction active.

Confirmed again:

```text
late FBO1/program18 PRE = 0/786432
late FBO1/program18 POST = 0/786432
texture 3 remains nonblack
```

The trace emitted a large operation window, but the existing server log format
escaped embedded newlines from several diagnostic records. Consequently the
operation list is not treated as a trustworthy exact GOOD_POST-to-BAD_PRE
classifier. No unsupported claim is made about a specific clear/draw from that
output.

The valid conclusion remains that the first blackening operation is earlier
than the late program-18 draws. A future tiny trace must use one-line structured
records at the server boundary (or raw binary log parsing) before attributing
the transition to a particular GL operation.

Production was restored from the distribution package and verified after the
run. No diagnostic preload remains.
