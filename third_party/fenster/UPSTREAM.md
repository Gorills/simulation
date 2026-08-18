# Fenster vendored source provenance

Fenster is the only required third-party source in the current foundation stack.

- Upstream repository: `https://github.com/zserge/fenster`
- Selected upstream commit: `e700581dfb7956dd161aee44fc0cff0663e789a1`
- Upstream source path: `fenster.h`
- Upstream source Git blob at that commit: `ed4332e917e9aa84a2f309d6cd4d02ebbb886596`
- License: MIT; repository copy: [`LICENSE`](LICENSE)
- Local build input: [`fenster.h`](fenster.h)
- Local integrity hashes: [`../manifest.json`](../manifest.json)

The repository copy is the only source consumed by normal configure/build/test/verification. Those commands must never fetch Fenster from the network.

The pinned upstream commit records provenance and the baseline chosen for review. The local SHA-256 in `third_party/manifest.json` is the integrity check for the exact repository build input.

Do not edit vendored files during ordinary development. An intentional dependency upgrade/modification is a separate bounded stack task and must review provenance/license, update the local source and hashes, then rerun `python tools/dev.py verify` before publication.
