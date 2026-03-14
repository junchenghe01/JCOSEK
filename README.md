# JCOSEK

An OS implemented based on the OSEK/VDX Operating System Specification requirements.

## Quick Start

1. Clone the repository:
```bash
git clone https://github.com/junchenghe01/JCOSEK.git
cd JCOSEK
```

2. Build (example; replace with your cross-toolchain and build steps):
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

3. Run tests (if present):
```bash
make test
```

## Contribution
We welcome contributions. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) before contributing.

## License
This project uses the Mozilla Public License 2.0 (MPL‑2.0). See `LICENSE` for the full text.

## Suggested repository layout
- `docs/`      — documentation
- `include/`   — public headers
- `portable/`  - portable
- `src/`       — kernel / drivers / MPL‑protected core code
- `tests/`     — unit and integration tests

## Distribution & Compliance
If you distribute binaries/firmware that include modified MPL files from this project, you must provide access to the modified MPL source files (or include them) per MPL‑2.0 requirements. See `LICENSE` and `LICENSE-FAQ.md` for details.

## Contact
Security contact: hejuncheng0@gmail.com
General contact: hejuncheng0@gmail.com