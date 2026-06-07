# 安全政策

## 报告安全漏洞

**请不要通过 public GitHub issues 报告安全漏洞。**

JCOSEK 的安全性对我们很重要。如果你发现了安全漏洞，请按照以下步骤报告：

### 报告方式

1. **Email 报告**（推荐）
   - 发送至：[请在此处添加安全联系邮箱]
   - 标题：`[SECURITY] Vulnerability Report`
   - 包含以下信息：
     - 漏洞描述
     - 受影响的版本/分支
     - 复现步骤（如果适用）
     - 潜在的影响或攻击场景
     - 建议的修复方案（可选）

2. **GitHub Security Advisory**
   - 访问 [Security Advisory](https://github.com/junchenghe01/JCOSEK/security/advisories)
   - 点击 "Report a vulnerability"

### 预期响应时间

- **初始确认**：24-48 小时内
- **漏洞评估**：3-7 天内
- **修复计划**：根据严重程度确定
- **补丁发布**：5-14 天内（严重漏洞优先）

## 漏洞评级

我们使用 CVSS 3.1 标准评估漏洞严重程度：

| 严重程度 | CVSS 分数 | 响应优先级 |
|---------|----------|---------|
| Critical | 9.0-10.0 | 24 小时内 |
| High | 7.0-8.9 | 3 天内 |
| Medium | 4.0-6.9 | 7 天内 |
| Low | 0.1-3.9 | 30 天内 |

## 披露政策

- **协调披露**：我们遵循业界标准的 90 天协调披露政策
- 在补丁发布前，漏洞信息将保持保密
- 发布者和维护者将共同协商公开时间
- 发现者将在安全公告中获得适当的致谢

## 已知局限

JCOSEK 是一个嵌入式系统项目，主要用于特定硬件平台。使用者应当：

- 在受控环境中进行彻底测试
- 遵循硬件制造商的安全指南
- 不将未经审查的代码部署到生产环境
- 定期更新到最新版本

## 安全最佳实践

### 开发者

- 定期审查依赖项更新
- 使用内存安全工具（如 AddressSanitizer）进行测试
- 进行代码审查时特别关注安全问题
- 在硬件层面考虑旁路攻击风险

### 用户

- 仅从官方来源下载代码
- 验证代码签名和版本哈希
- 在使用前进行本地编译和测试
- 遵循项目的 README 中的部署指南
- 及时应用安全补丁

## 附加资源

- [OWASP Embedded Security Top 10](https://owasp.org/www-project-embedded-application-security/)
- [CWE - Common Weakness Enumeration](https://cwe.mitre.org/)
- [CVE - Common Vulnerabilities and Exposures](https://www.cve.org/)

---

感谢你的负责任的安全研究！
