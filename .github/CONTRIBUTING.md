# Contributing Guide

Thank you for your interest in the JCOSEK project! This guide will help you understand how to contribute effectively to our open-source community.

## Code of Conduct

We are committed to providing an inclusive and welcoming environment for all contributors. Please review our [Code of Conduct](./CODE_OF_CONDUCT.md).

## How to Contribute

### 1. Reporting Bugs

- **Check Existing Issues**: Before submitting a new report, please search [Issues](https://github.com) to confirm if it has already been reported.
- **Provide Detailed Information**:
  - A clear and descriptive summary of the issue.
  - Precise steps to reproduce the bug.
  - Expected behavior vs. actual behavior.
  - Environment details (hardware platform, compiler toolchain version, OS, etc.).
  - Error logs, terminal outputs, or screenshots.
- **Use the Bug Report Template**: Select the appropriate bug template when creating a new issue.

### 2. Suggesting Enhancements

- **Create an Enhancement Issue**: Use the Feature Request template.
- **Describe the Use Case**: Explain why this feature is needed and what pain points it resolves.
- **Provide Code Snippets**: Include example code or pseudocode if applicable.
- **Discuss the Solution**: Actively participate in the technical discussion within the issue thread.

### 3. Code Contributions

#### Prerequisites
- Solid understanding of C programming.
- Familiarity with embedded development or assembly language (depending on the module).
- Proficiency with the standard Git branching workflow.

#### Setting Up the Development Environment
```bash
# Clone the repository
git clone https://github.com
cd JCOSEK

# Create a local development branch (recommended prefixes: feat/ or fix/)
git checkout -b feat/your-feature-name
```
*Note: For detailed instructions on compiling, flashing, and running the code, please refer to the build section in `README.md`.*

#### Coding Standards

1. **Code Style**
   - Use **4 spaces for indentation** (do NOT use Tab characters).
   - Strictly follow the **Linux Kernel Coding Style**.
   - Use meaningful English names for variables and functions. Avoid Pinyin or cryptic abbreviations.
   - Add necessary comments, especially for complex algorithms and direct hardware/register interactions.

2. **Code Quality**
   - Compile and test your code locally before submitting.
   - Ensure the build output has **zero compiler warnings**.
   - Implement robust error handling for critical functions and hardware return statuses.
   - Avoid magic numbers; use `#define` macro constants or `enum` types instead.

3. **File Header Documentation**
   ```c
   /**
    * @file filename.c
    * @brief Short description of the file's primary purpose.
    * @author Your Name <your.email@example.com>
    * @date YYYY-MM-DD
    * @version 1.0
    */
   ```

4. **Function Documentation**
   ```c
   /**
    * @brief Brief introduction to the function's functionality.
    * @param[in] param1 Description of input parameter 1.
    * @param[out] param2 Description of output parameter 2.
    * @return Description of the return value (e.g., 0 for success, negative for error codes).
    */
   ```

#### Git Workflow & Submission

1. **Synchronize with Main Branch** (Avoid merge conflicts)
   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **Commit Changes with DCO Sign-off**
   This project strictly enforces the **DCO (Developer Certificate of Origin)** sign-off mechanism to guarantee open-source copyright compliance. You **must append the `-s` flag** whenever you run `git commit`:
   ```bash
   git add .
   git commit -s -m "feat: add spi driver for stm32"
   ```
   *-s automatically appends `Signed-off-by: Your Name <your.email@example.com>` to the end of your commit message. Commits without a valid sign-off will fail the CI check.*
   
   **Commit messages** must strictly adhere to the [Conventional Commits](https://conventionalcommits.org) specification format: `<type>: <subject>`
   - **type**: `feat` (new feature), `fix` (bug fix), `docs` (documentation), `refactor` (code refactoring), `test` (adding/fixing tests), `chore` (build tasks/dependencies).
   - **subject**: Concise summary (under 50 characters) using the imperative mood.

3. **Submit a Pull Request (PR)**
   ```bash
   git push origin feat/your-feature-name
   ```
   Go to the GitHub web interface to open a PR. You must fill out the [PR Template](./pull_request_template.md). Pay extra attention to:
   - **Link Issues**: Use GitHub closing keywords in the PR description to automatically link and close related issues (e.g., `Closes #123` or `Fixes #456`).
   - **MPL Compliance**: Clearly state if your modifications affect any files protected under the MPL-2.0 license, and list their file locations.

4. **Code Review**
   - At least 1 project maintainer must review and approve the PR.
   - Address any Code Review feedback by making changes locally and pushing directly to the same branch. The PR will update automatically.

5. **Merging**
   - Once approved and all CI checks turn green, a maintainer will merge the PR.
   - We prefer the **Squash and Merge** strategy. This condenses all your intermediate development commits into a single, clean commit on the main branch to keep git history pristine.

### 4. Documentation Improvements

- Contributions to the README, API reference manuals, architectural design docs, or inline comments are highly welcome.
- Fix typos, spelling mistakes, or grammatical errors.
- Add practical board-level examples (`Examples`) showcasing real-world runtime usage.
- Translate existing documentation into other languages.

### 5. Testing

- Write comprehensive test cases for any newly introduced modules or features.
- Ensure all existing tests and static analysis (Linter) pass completely in your local environment.
- When possible, perform cross-compilation tests across different hardware targets or compilers (e.g., GCC, Clang).

## Pull Request Review Process

### Automated Checks (CI/CD)
When a PR is submitted, GitHub Actions automatically triggers a series of checks. **All checks must pass before merging**:
- ✅ Clean compilation (zero warnings or errors).
- ✅ Automated test suite passes successfully.
- ✅ Code style compliance (Linter) checks pass.
- ✅ DCO sign-off validation (every commit must include `Signed-off-by`).
- ✅ No high-risk dependency or security vulnerabilities detected.

### Manual Review
Maintainers will evaluate the PR across several dimensions:
- **Code Quality**: Adherence to the Linux kernel style, logical clarity, and descriptive comments.
- **Correctness**: Validating if the expected behavior is fully achieved, including proper edge-case handling (such as transfer timeouts or buffer overflows).
- **Backward Compatibility**: Ensuring the changes do not unintentionally break existing public APIs or configurations.
- **Completeness**: Checking if the new feature is properly accompanied by updated API comments or user documentation.

## Getting Help

- **Technical & Architectural Questions**: Please start a conversation in our [Discussions](https://github.com) forum.
- **Contribution Queries**: Contact the maintainers directly or open an issue with the `question` label.
- **Security Vulnerabilities**: If you discover a critical security vulnerability, do NOT open a public issue. Please follow the disclosure workflow outlined in [SECURITY.md](./SECURITY.md).

## Licensing

By contributing to JCOSEK, you agree that your contributions will be licensed under the project's **[MPL-2.0 License](../LICENSE)**. Any modifications made to existing MPL-protected files must remain strictly under the MPL-2.0 terms.

Thank you for your valuable contributions to making JCOSEK a better open-source project! 🎉
