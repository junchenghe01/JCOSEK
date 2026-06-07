# 贡献指南

感谢你对 JCOSEK 项目的关注！本指南将帮助你理解如何以最有效的方式参与本项目。

## 行为准则

我们致力于为所有贡献者提供包容、友好的环境。请参考我们的 [行为准则](./CODE_OF_CONDUCT.md)。

## 如何贡献

### 1. 报告 Bug

- **检查现有 Issue**：在提交前，请先搜索 [Issues](https://github.com/junchenghe01/JCOSEK/issues) 确认是否已有人报告
- **提供详细信息**：
  - 问题的清晰描述
  - 复现步骤
  - 预期行为 vs 实际行为
  - 环境信息（硬件平台、编译工具链版本、操作系统等）
  - 错误日志或输出
- **使用 Bug Report 模板**：创建新 Issue 时选择相应模板

### 2. 建议新功能

- **创建 Enhancement Issue**：使用 Feature Request 模板
- **描述使用场景**：为什么需要这���功能
- **提供示例代码或伪代码**（如果适用）
- **讨论实现方案**：参与社区讨论

### 3. 提交代码

#### 前置要求

- C 语言基础知识
- 了解嵌入式开发或汇编语言（根据模块）
- 熟悉 Git 工作流

#### 开发环境设置

```bash
# 克隆仓库
git clone https://github.com/junchenghe01/JCOSEK.git
cd JCOSEK

# 创建本地开发分支
git checkout -b feature/your-feature-name

# 安装依赖（如需要）
# 参见 README.md 中的构建说明
```

#### 编码规范

1. **代码风格**
   - 使用 4 个空格缩进（不使用 Tab）
   - 遵循 Linux 内核编码风格
   - 变量和函数使用有意义的英文名称
   - 添加必要的注释，特别是复杂算法和硬件交互部分

2. **代码质量**
   - 在提交前编译并测试
   - 确保没有编译警告
   - 对关键功能添加错误处理
   - 避免魔法数字，使用 #define 常量

3. **文件头注释**
   ```c
   /**
    * @file filename.c
    * @brief 简短描述
    * @author Your Name
    * @date YYYY-MM-DD
    * @version 1.0
    */
   ```

4. **函数注释**
   ```c
   /**
    * @brief 函数简介
    * @param[in] param1 参数1说明
    * @param[out] param2 参数2说明
    * @return 返回值说明
    */
   ```

#### 提交流程

1. **同步主分支**
   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **添加和提交**
   ```bash
   git add .
   git commit -m "feat: add new feature description"
   ```
   
   提交信息格式：`<type>: <subject>`
   - **type**: feat(新功能), fix(修复), docs(文档), refactor(重构), test(测试), chore(构建/依赖)
   - **subject**: 简明扼要（50字以内），使用命令式

3. **创建 Pull Request**
   ```bash
   git push origin feature/your-feature-name
   ```
   
   然后在 GitHub 上创建 PR，使用 [PR 模板](./pull_request_template.md)

4. **代码审查**
   - 至少需要 1 名维护者审查
   - 根据反馈进行修改
   - 推送到同一分支，自动更新 PR

5. **合并**
   - 审查通过后由维护者合并
   - 优先使用 Squash and Merge（保持历史整洁）

### 4. 文档改进

- 改进 README、API 文档或注释
- 修复拼写和语法错误
- 添加使用示例
- 翻译文档到其他语言

### 5. 测试

- 为新功能编写测试用例
- 确保所有现有测试通过
- 在不同平台/编译器上测试（如适用）

## Pull Request 审查流程

### 自动检查

- ✅ CI/CD 管道通过（编译、测试）
- ✅ 代码覆盖率不下降
- ✅ 没有依赖安全漏洞

### 人工审查

- **代码质量**：风格一致、逻辑清晰
- **功能正确性**：是否实现了预期功能
- **向后兼容性**：是否破坏现有 API
- **文档完整性**：是否更新了相关文档

## 获取帮助

- **技术问题**：在 [Discussions](https://github.com/junchenghe01/JCOSEK/discussions) 中提问
- **贡献问题**：联系维护者或创建 Issue
- **安全问题**：请参见 [SECURITY.md](./SECURITY.md)

## 许可证

通过提交代码，你同意你的贡献将根据项目的 [MPL-2.0 许可证](../LICENSE) 进行许可。

感谢你的贡献！🎉
