# 数字地图脚手架库 (Digital Map Scaffold Library)

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-5.14%2B-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-11-orange.svg)](https://isocpp.org/)

## 📖 项目概述

数字地图脚手架库是一个基于Qt和OSG的数字地图应用框架，提供完整的地理实体管理、渲染和交互功能。本项目旨在成为一个可扩展的脚手架库，对GeoEntity进行封装，对其状态进行管理，可对其进行渲染，同时将数字地图上的任务GeoEntity和数字地图的状态信息抽象为一个信息层，传递给其他服务。

### 核心特性

- 🗺️ **3D地球渲染** - 基于osgEarth的高性能3D地球渲染
- 🎯 **实体管理系统** - 统一的地理实体生命周期管理
- 🔄 **2D/3D视图切换** - 灵活的视角切换和相机控制
- 📋 **方案规划** - 完整的方案文件管理和实体规划功能
- 🎨 **可扩展架构** - 基于继承的实体类型扩展机制

## 🛠️ 技术栈

- **Qt 5.14+**: GUI框架和跨平台支持
- **OSG (OpenSceneGraph)**: 高性能3D图形渲染引擎
- **osgEarth**: 基于OSG的地球渲染库
- **SQLite**: 数据库存储
- **C++11**: 编程语言标准

## 📁 项目结构

```
项目根目录/
├── ui/                              # UI界面模块
│   ├── MainWidget.h/cpp/ui         # 主窗口
│   ├── ComponentConfigDialog.h/cpp # 组件配置对话框
│   ├── ModelAssemblyDialog.h/cpp   # 模型组装对话框
│   ├── ModelDeployDialog.h/cpp     # 模型部署对话框
│   └── EntityPropertyDialog.h/cpp  # 实体属性编辑对话框
├── widgets/                         # 自定义控件模块
│   ├── OsgMapWidget.h/cpp          # OSG地图控件
│   ├── draggablelistwidget.h/cpp   # 可拖拽列表控件
│   └── imageviewerwindow.h/cpp     # 图片查看器窗口
├── geo/                             # 地理实体模块
│   ├── geoentity.h/cpp             # 通用地理实体基类
│   ├── imageentity.h/cpp           # 图片实体实现
│   ├── waypointentity.h/cpp        # 航点实体实现
│   ├── geoentitymanager.h/cpp      # 通用实体管理器
│   ├── mapstatemanager.h/cpp       # 地图状态管理器
│   └── geoutils.h/cpp               # 地理工具类
├── plan/                            # 方案管理模块
│   └── planfilemanager.h/cpp       # 方案文件管理器
├── util/                            # 工具模块
│   └── databaseutils.h/cpp         # 数据库工具类
├── OsgQt/                           # OSG-Qt集成组件
│   ├── GraphicsWindowQt.h/cpp       # Qt图形窗口适配器
│   ├── QGraphicsViewAdapter.h/cpp  # QGraphicsView适配器
│   └── QWidgetImage.h/cpp          # Qt Widget图像处理
├── images/                          # 实体图片资源
├── earth/                           # 地球配置文件
│   ├── my.earth                     # osgEarth配置文件
│   └── world.tif                    # 地球纹理
├── docs/                            # 文档目录
│   └── mainpage.md                  # 主文档页面
├── main.cpp                         # 程序入口
├── ScenePlan2.pro                   # Qt项目文件
├── res.qrc                          # Qt资源文件
├── README.md                        # 项目说明文档
└── WorkRecord.md                    # 工作记录文档
```

## 🚀 快速开始

### 环境要求

- **Qt**: 5.14 或更高版本（推荐5.15+）
- **OSG**: 3.6.x 或更高版本
- **osgEarth**: 2.10+ 或更高版本
- **编译器**: MSVC 2017+ / GCC 7+ / Clang 8+
- **CMake**: 3.10+（可选，项目使用qmake）

### 构建步骤

1. **克隆项目**
   ```bash
   git clone <repository-url>
   cd osgEarthmy_osgb
   ```

2. **配置OSG路径**
   
   编辑 `ScenePlan2.pro` 文件，修改OSG库路径：
   ```pro
   OSGDIR = E:/osgqtlib/  # 修改为你的OSG安装路径
   ```

3. **配置数据库路径**
   
   编辑 `main.cpp`，设置数据库绝对路径：
   ```cpp
   DatabaseUtils::setDatabasePath("E:/osgqtlib/osgEarthmy_osgb/MyDatabase.db");
   ```

4. **使用Qt Creator打开项目**
   - 打开 `ScenePlan2.pro`
   - 选择合适的构建套件（Kit）
   - 点击"构建"按钮

5. **运行程序**
   - 确保所有依赖库（OSG、osgEarth）在系统PATH中，或与可执行文件在同一目录
   - 运行生成的可执行文件

## ✨ 核心功能

### ✅ 3D地图显示
- 基于osgEarth的3D地球渲染
- 高德地图瓦片集成
- 地球漫游控制（缩放、旋转、平移）
- 相机视角管理和保存/恢复

### ✅ 实体管理系统
- 通用GeoEntity基类设计，支持生命周期管理
- ImageEntity图片实体实现
- WaypointEntity航点实体实现
- GeoEntityManager统一管理所有实体
- 拖拽添加实体功能

### ✅ 2D/3D视图切换
- 动态切换地图显示模式
- 自定义视角配置
- 视角自动跳转
- 按钮状态同步

### ✅ 实体交互功能
- 实体选择系统（点击选择）
- 实体高亮显示（红色边框）
- 右键上下文菜单
- 航向角和高度设置
- 实体删除和属性查看
- 双击编辑实体属性

### ✅ 方案规划功能
- 方案文件管理（新建、打开、保存、另存为）
- 实体规划部署（拖拽部署到地图）
- 实体属性编辑（规划属性、模型组装、组件配置）
- 相机视角保存和恢复
- 自动保存功能
- 最近文件管理

### ✅ 事件处理架构
- GeoEntityManager统一处理鼠标事件
- Qt信号槽机制进行组件间通信
- 多态支持实现实体选择高亮
- 职责分离设计

### ✅ 代码架构
- 模块化设计，清晰的目录结构
- 基于继承的实体扩展机制
- 统一的工具类和辅助方法
- 完善的Doxygen文档注释

## 📖 使用示例

### 创建自定义实体

```cpp
#include "geo/geoentity.h"
#include <osg/Node>

class CustomEntity : public GeoEntity {
public:
    CustomEntity(const QString& name, double lon, double lat, double alt)
        : GeoEntity("custom", name, lon, lat, alt) {}

protected:
    osg::Node* createNode() override {
        // 创建你的自定义OSG节点
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        // ... 添加几何体或模型
        return geode.release();
    }
    
    void onInitialized() override {
        // 初始化完成后的自定义逻辑
    }
};
```

### 添加实体到地图

```cpp
GeoEntityManager* manager = getEntityManager();
GeoEntity* entity = manager->createEntity(
    "aircraft",           // 实体类型
    "F-16",              // 实体名称
    QJsonObject(),       // 属性
    116.4,               // 经度
    39.9,                // 纬度
    100000.0             // 高度（米）
);
```

## 📚 文档

- **API文档**: 使用Doxygen生成，运行 `doxygen Doxyfile` 生成HTML文档
- **工作记录**: 查看 [WorkRecord.md](WorkRecord.md) 了解开发历史和更新日志
- **架构文档**: 查看 `docs/mainpage.md` 了解详细架构设计

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📝 工作记录

详细的工作总结、待完成任务和更新日志请参考 [WorkRecord.md](WorkRecord.md) 文档。

## 📄 许可证

本项目采用MIT许可证。

---

**注意**: 本项目需要预先安装OSG和osgEarth库。请确保这些库正确配置在系统环境变量中，或在项目配置文件中指定正确的路径。
