# Contributing Guide

Thank you for your interest in contributing!

Please follow these steps to make the contribution process smooth:

1. Discuss major changes in an issue before implementing them (especially changes that affect `src/` or drivers).
2. Fork the repository and create a feature branch (naming examples: `feat/<short-desc>`, `fix/<short-desc>`).
3. Use Conventional Commits for commit messages (e.g. `feat: add spi driver`, `fix: correct irq handling`).
4. Add tests for your changes if applicable.
5. When opening a PR:
   - Link related issue(s).
   - Describe the motivation, tests performed, and potential regressions.
   - State whether you changed any MPL‑protected files (and where).
6. Review policy:
   - Small bugfixes may be merged by one maintainer.
   - Significant changes should have at least two approvals (see `MAINTAINERS.md` / `CODEOWNERS`).
7. Sign off your commits using DCO (Developer Certificate of Origin):
   Add `Signed-off-by: Your Name <your.email@example.com>` to your commits (or confirm in PR description).

Code style and tooling:
- Use project formatting tools (e.g., `clang-format`) before submitting.
- Run local tests (`make test`, `ctest`, etc.) to ensure CI will pass.

License and compliance:
- This project is licensed under MPL‑2.0. Modifications to MPL‑protected files must be published under MPL‑2.0. See `LICENSE` and `LICENSE-FAQ.md`.