# 文档生成说明

## 本地生成文档

### 前提条件

安装Doxygen：

**Windows:**
```bash
# 使用Chocolatey
choco install doxygen.install

# 或下载安装包
# https://www.doxygen.nl/download.html
```

**Linux:**
```bash
sudo apt-get install doxygen
```

**macOS:**
```bash
brew install doxygen
```

### 生成文档

在项目根目录执行：

```bash
doxygen Doxyfile
```

生成的HTML文档位于 `docs/html/` 目录。

### 查看文档

打开 `docs/html/index.html` 即可查看文档。

## 自动部署到GitHub Pages

### 配置步骤

1. **启用GitHub Pages**
   - 进入仓库的 Settings → Pages
   - Source 选择 "Deploy from a branch"
   - Branch 选择 `gh-pages` 分支，目录选择 `/docs`
   - 保存设置

2. **推送代码**
   - 将代码推送到 `main` 分支
   - GitHub Actions 会自动触发文档生成
   - 生成的文档会自动部署到 `gh-pages` 分支

3. **访问文档**
   - 文档地址：`https://your-username.github.io/your-repo/docs/`
   - 等待几分钟让GitHub Pages部署完成

### 工作流说明

`.github/workflows/docs.yml` 定义了自动化流程：

- **触发条件**：
  - 推送到 `main` 分支
  - 修改了 `geo/`、`mainwindow.*`、`README.md` 或 `Doxyfile`
  - 手动触发（workflow_dispatch）

- **执行步骤**：
  1. 检出代码
  2. 安装Doxygen
  3. 生成文档
  4. 部署到GitHub Pages

### 查看构建状态

在仓库的 Actions 标签页可以查看文档生成的状态和日志。

## 文档注释规范

### 类注释

```cpp
/**
 * @brief 类的简要说明
 * 
 * 类的详细说明，可以包含多行。
 * 
 * 主要功能：
 * - 功能1
 * - 功能2
 * 
 * @note 特别注意事项
 */
class MyClass {
    // ...
};
```

### 方法注释

```cpp
/**
 * @brief 方法的简要说明
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 * @note 特别注意事项
 */
void myMethod(int param1, double param2);
```

### 信号注释

```cpp
signals:
    /**
     * @brief 信号的简要说明
     * @param param 参数说明
     */
    void mySignal(int param);
```

### 成员变量注释

```cpp
private:
    int value_;           ///< 简短说明
    QString name_;        ///< 名称
    bool isActive_;       ///< 是否激活
```

## 文档结构

```
docs/
├── html/              # 生成的HTML文档
├── mainpage.md        # 文档首页
└── README.md          # 本说明文件
```

## 常见问题

### Q: 文档没有自动更新？

A: 检查以下几点：
1. GitHub Actions 是否启用
2. 是否有权限推送到 `gh-pages` 分支
3. 查看 Actions 日志是否有错误

### Q: 如何添加更多文档页面？

A: 在 `docs/` 目录下创建 `.md` 文件，Doxygen 会自动识别并生成页面。

### Q: 如何自定义文档样式？

A: 修改 `Doxyfile` 中的 `HTML_COLORSTYLE_HUE` 和 `HTML_COLORSTYLE_SAT` 参数。

## 参考资料

- [Doxygen 官方文档](https://www.doxygen.nl/manual/)
- [Doxygen 注释规范](https://www.doxygen.nl/manual/docblocks.html)
- [GitHub Actions 文档](https://docs.github.com/en/actions)

