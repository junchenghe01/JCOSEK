# Tagging Specification

## Common tags

| 标记 |   含义 | 场景示例 |
| :----: | :-----| :-----|
| TODO           | 待办           | // TODO: 增加对多核(Multi-core)的支持 |
| FIXME          | 待修复         | // FIXME: 此处高并发下有竞态条件，需加锁 |
| HACK           | 奇技淫巧       | // HACK: 为了绕过硬件 A 版本的 Bug，强制延时 10ms |
| XXX            | 危险/需警惕    | // XXX: 这段逻辑极其复杂，修改前务必咨询架构师 |
| OPTIMIZE       | 性能优化       | // OPTIMIZE: 现在的线性搜索太慢，后期改用哈希表 |
| REFACTOR       | 重构           | // REFACTOR: 将这个 500 行的函数拆分为三个子函数 |
| DOC            | 文档补全       | // DOC: 需要在技术手册中补全该 API 的参数说明 |
| PERF           | 性能表现       | // PERF: 降低中断嵌套时的上下文切换开销 |
| NOTE           | 特别注意       | // NOTE: 此处必须先清中断位，再读取数据寄存器 |
| DEPRECATED     | 废弃预警       | // DEPRECATED: 该接口将在下个版本被删除，请改用 Os_NewApi |
| STUB           | 桩代码         | // STUB: 暂时返回 E_OK 以通过编译，逻辑未实现 |
| WORKAROUND     | 权宜之计       | // WORKAROUND: 因编译器不支持 C99，手动对齐内存 |

## Tag usage

// [分类] (责任人/日期): 描述 | 关联项
