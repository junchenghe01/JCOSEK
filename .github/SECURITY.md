# Security Policy

## Reporting a Vulnerability

**Please do NOT report security vulnerabilities via public GitHub issues.**

The security of JCOSEK is of utmost importance to us. If you discover a security vulnerability, please report it privately using one of the following methods:

### Reporting Channels

1. **Email Report (Recommended)**
   - Send to: **hejuncheng0@gmail.com**
   - Subject: `[SECURITY] Vulnerability Report - JCOSEK`
   - Please include the following details:
     - A thorough description of the vulnerability.
     - Affected versions, branches, or specific hardware targets.
     - Step-by-step instructions to reproduce the issue (if applicable).
     - Potential impact or exploit scenarios.
     - Suggested remediation or fix (optional).

2. **GitHub Private Vulnerability Reporting**
   - Navigate to the [Security Advisories Page](https://github.com).
   - Click "Report a vulnerability" to submit a confidential report directly to the maintainers.

### Expected Response Timeline

- **Initial Acknowledgment**: Within 24–48 hours.
- **Vulnerability Assessment**: Within 3–7 days.
- **Remediation Planning**: Determined based on severity.
- **Patch Release**: Within 5–14 days (critical vulnerabilities will be prioritized).

## Vulnerability Rating

We evaluate vulnerability severity according to the CVSS 3.1 standard:


| Severity | CVSS Score | Response Priority |
| :--- | :--- | :--- |
| **Critical** | 9.0 – 10.0 | Within 24 hours |
| **High** | 7.0 – 8.9 | Within 3 days |
| **Medium** | 4.0 – 6.9 | Within 7 days |
| **Low** | 0.1 – 3.9 | Within 30 days |

## Disclosure Policy

- **Coordinated Disclosure**: We follow the standard industry 90-day coordinated vulnerability disclosure policy.
- Vulnerability details will remain strictly confidential until an official patch or mitigation is released.
- The discoverer and maintainers will coordinate together to determine the public disclosure date.
- The discoverer will receive full credit and attribution in the public security advisory.

## Known Limitations & Scope

JCOSEK is an embedded system project tailored for specific hardware platforms. Users should:
- Test thoroughly in controlled staging environments.
- Adhere strictly to the security guidelines provided by the chip/hardware manufacturers.
- Avoid deploying unvetted or unreviewed code into production systems.
- Regularly update to the latest stable release.

## Security Best Practices

### For Developers
- Regularly audit and review dependency updates.
- Utilize memory-safety diagnostics (such as AddressSanitizer) during testing.
- Pay close attention to security implications during the Code Review process.
- Explicitly consider side-channel attack risks at the hardware/firmware interface level.

### For Users
- Download source code and binaries exclusively from official releases.
- Verify cryptographic signatures and version hashes (Commit SHAs).
- Compile and test locally before flashing code onto physical devices.
- Strictly adhere to the deployment guidelines outlined in the project `README.md`.
- Apply critical security patches promptly.

## Additional Resources

- [OWASP Embedded Application Security Top 10](https://owasp.org/www-project-embedded-application-security/)
- [CWE - Common Weakness Enumeration](https://cwe.mitre.org/)
- [CVE - Common Vulnerabilities and Exposures](https://www.cve.org/)

---
Thank you for practicing responsible security research and helping us protect the JCOSEK community!
