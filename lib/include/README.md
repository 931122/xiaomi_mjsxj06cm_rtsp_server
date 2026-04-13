Third-party headers vendored for local and cross builds.

Included:
- OpenSSL headers generated from `OpenSSL_1_0_0-stable`
- zlib headers from `v1.2.11`

Layout:
- `openssl/*.h`
- `zlib.h`
- `zconf.h`

Notes:
- These headers are intended to match the shared libraries under `lib/`.
- `libmortox.so` is proprietary; no public header set was added for it.
