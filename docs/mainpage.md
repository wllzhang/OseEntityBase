@mainpage 数字地图脚手架库 (API 文档)

欢迎使用数字地图脚手架库。本项目基于 Qt、OpenSceneGraph (OSG) 与 osgEarth，围绕"实体（Geo Entities）- 管理器（Managers）- 视图（OSG/Qt 集成）"的分层架构，提供标准化的实体生命周期管理、地图状态监控、方案规划与交互扩展能力。

## 📖 项目概述

数字地图脚手架库是一个可扩展的数字地图应用框架，提供完整的地理实体管理、渲染和交互功能。主要特性包括：

- 🗺️ **3D地球渲染** - 基于osgEarth的高性能3D地球渲染
- 🎯 **实体管理系统** - 统一的地理实体生命周期管理
- 📋 **方案规划** - 完整的方案文件管理和实体规划功能
- 🔄 **2D/3D视图切换** - 灵活的视角切换和相机控制
- 🎨 **可扩展架构** - 基于继承的实体类型扩展机制

## 📁 模块结构

### 地理实体模块 (geo/)

核心实体类和地理相关功能：

- **GeoEntity** - 通用地理实体基类，提供统一的生命周期管理
- **ImageEntity** - 图片实体实现，支持2D图标显示和高亮
- **WaypointEntity** - 航点实体实现，支持航点标绘和路线规划
- **GeoEntityManager** - 实体管理器，统一管理所有实体的创建、删除、查询和交互
- **MapStateManager** - 地图状态管理器，监控相机视角和鼠标位置
- **GeoUtils** - 地理工具类，提供坐标转换、距离计算等实用功能

### UI界面模块 (ui/)

用户界面相关的对话框和主窗口：

- **MainWidget** - 应用程序主窗口，整合所有功能模块
- **ComponentConfigDialog** - 组件配置对话框，用于配置模型组件参数
- **ModelAssemblyDialog** - 模型组装对话框，管理模型组装配置
- **ModelDeployDialog** - 模型部署对话框，显示模型列表并支持拖拽部署
- **EntityPropertyDialog** - 实体属性编辑对话框，编辑实体的所有属性（规划属性、模型组装、组件配置）

### 自定义控件模块 (widgets/)

可重用的自定义Qt控件：

- **OsgMapWidget** - OSG地图控件，封装OSG渲染和地图交互
- **DraggableListWidget** - 可拖拽列表控件，支持拖拽操作
- **ImageViewerWindow** - 图片查看器窗口，显示和选择实体图片

### 方案管理模块 (plan/)

方案文件管理和规划功能：

- **PlanFileManager** - 方案文件管理器，负责方案的创建、保存、加载和管理
  - 方案文件格式：JSON格式，包含实体信息、相机视角等
  - 组件信息深层复制：保存完整的组件配置，独立于数据库

### 工具模块 (util/)

通用工具类：

- **DatabaseUtils** - 数据库工具类，统一管理数据库连接和操作

### OSG-Qt集成模块 (OsgQt/)

OSG与Qt的集成组件：

- **GraphicsWindowQt** - Qt与OSG的图形窗口适配器
- **QGraphicsViewAdapter** - QGraphicsView适配器
- **QWidgetImage** - Qt Widget图像处理

## 📚 模块分组（Doxygen Groups）

以下章节与代码中的 Doxygen 分组一致，便于在生成文档中快速导航：

- \ref geo_entities "Geo Entities" — 通用地理实体与实现（`GeoEntity`、`ImageEntity`、`WaypointEntity`）
- \ref managers "Managers" — 实体与地图管理（`GeoEntityManager`、`MapStateManager`）

## 🏗️ 架构概览

```
应用层 (Application Layer)
├── MainWidget (主窗口)
│   ├── UI组件
│   │   ├── ComponentConfigDialog (组件配置)
│   │   ├── ModelAssemblyDialog (模型组装)
│   │   ├── ModelDeployDialog (模型部署)
│   │   └── EntityPropertyDialog (实体属性编辑)
│   └── Widgets
│       ├── OsgMapWidget (地图控件)
│       ├── DraggableListWidget (可拖拽列表)
│       └── ImageViewerWindow (图片查看器)
│
├── 方案管理层 (Plan Layer)
│   └── PlanFileManager (方案文件管理)
│       ├── 方案创建/加载/保存
│       ├── 实体序列化/反序列化
│       └── 相机视角保存/恢复
│
├── 管理器层 (Manager Layer)
│   ├── GeoEntityManager
│   │   ├── 实体创建/查询/删除
│   │   ├── 实体选择/高亮
│   │   ├── 鼠标事件处理
│   │   └── 信号槽通信
│   └── MapStateManager
│       ├── 地图状态监控
│       ├── 相机视角管理
│       └── 鼠标位置跟踪
│
└── 实体层 (Entity Layer)
    ├── GeoEntity (抽象基类)
    │   ├── 生命周期管理 (initialize/update/cleanup)
    │   ├── 位置和朝向管理
    │   └── 属性管理
    ├── ImageEntity (图片实体)
    └── WaypointEntity (航点实体)

渲染层 (Rendering Layer)
└── OSG SceneGraph + osgEarth MapNode
    ├── osg::Group (场景图)
    ├── osgEarth::MapNode (地图节点)
    └── osgQt::GraphicsWindowQt (Qt集成)
```

## ✨ 主要功能

### 3D地图显示
- 基于osgEarth的3D地球渲染
- 高德地图瓦片集成
- 地球漫游控制（缩放、旋转、平移）
- 相机视角管理和保存/恢复

### 实体管理
- 实体的创建、删除、查询
- 拖拽添加实体（从模型部署对话框）
- 实体位置和朝向控制
- 实体属性管理（规划属性、模型组装、组件配置）
- 实体选择和高亮显示

### 方案规划
- 方案文件管理（新建、打开、保存、另存为）
- 实体规划部署（拖拽部署到地图）
- 实体属性编辑（完整的属性编辑对话框）
- 相机视角保存和恢复
- 自动保存功能
- 最近文件管理
- 组件信息深层复制（独立于数据库）

### 2D/3D视图切换
- 动态切换地图显示模式
- 自定义视角配置
- 视角自动跳转
- 按钮状态同步

### 实体交互
- 实体选择系统（点击选择）
- 实体高亮显示（红色边框）
- 右键上下文菜单
- 双击编辑实体属性
- 航向角和高度设置
- 实体删除和属性查看

## 🚀 快速开始

### 1) 初始化地图和实体管理器

```cpp
#include "widgets/OsgMapWidget.h"
#include "geo/geoentitymanager.h"
#include "geo/mapstatemanager.h"

// 创建地图控件
OsgMapWidget* mapWidget = new OsgMapWidget(this);
mapWidget->loadMap("earth/my.earth");

// 等待地图加载完成
connect(mapWidget, &OsgMapWidget::mapLoaded, this, [this, mapWidget]() {
    // 获取场景根节点和地图节点
    osg::Group* root = mapWidget->getRoot();
    osgEarth::MapNode* mapNode = mapWidget->getMapNode();
    
    // 创建实体管理器
    GeoEntityManager* entityManager = new GeoEntityManager(root, mapNode, this);
    
    // 创建地图状态管理器
    MapStateManager* mapStateManager = new MapStateManager(
        mapWidget->getViewer(), this
    );
    
    // 设置实体管理器
    mapWidget->setEntityManager(entityManager);
    mapStateManager->setMapNode(mapNode);
});
```

### 2) 创建实体

```cpp
// 方法1: 直接创建
GeoEntity* aircraft = entityManager->createEntity(
    "aircraft",            // 实体类型
    "F-15",                // 实体名称
    QJsonObject{},          // 属性
    116.4,                  // 经度
    39.9,                   // 纬度
    100000.0                // 高度（米）
);

// 方法2: 从方案文件加载（通过PlanFileManager）
PlanFileManager* planManager = new PlanFileManager(this);
planManager->setEntityManager(entityManager);
planManager->loadPlan("path/to/plan.json");  // 自动创建实体
```

### 3) 连接交互信号

```cpp
// 实体选择信号
connect(entityManager, &GeoEntityManager::entitySelected, 
        this, [](GeoEntity* e) {
    qDebug() << "选中实体:" << e->getName();
});

// 实体双击信号（打开属性编辑对话框）
connect(entityManager, &GeoEntityManager::entityDoubleClicked,
        this, [this, planManager](GeoEntity* e) {
    EntityPropertyDialog dialog(e, planManager, this);
    dialog.exec();
});

// 实体右键信号
connect(entityManager, &GeoEntityManager::entityRightClicked,
        this, [](GeoEntity* e, QPoint p) {
    // 显示右键菜单
    QMenu menu;
    menu.addAction("编辑属性", [e, planManager]() {
        EntityPropertyDialog dialog(e, planManager, this);
        dialog.exec();
    });
    menu.addAction("删除", [e, planManager]() {
        planManager->removeEntityFromPlan(e);
        // 删除实体...
    });
    menu.exec(p);
});
```

### 4) 地图状态监控

```cpp
// 地图状态变化
connect(mapStateManager, &MapStateManager::stateChanged,
        this, [](const MapStateInfo& s) {
    qDebug() << "俯仰角:" << s.pitch
             << "航向角:" << s.heading
             << "距离:" << s.range;
    qDebug() << "视角位置:" << s.viewLongitude << s.viewLatitude;
    qDebug() << "鼠标位置:" << s.mouseLongitude << s.mouseLatitude;
});

// 视角位置变化
connect(mapStateManager, &MapStateManager::viewPositionChanged,
        this, [](double lon, double lat, double alt) {
    qDebug() << "视角位置变化:" << lon << lat << alt;
});
```

### 5) 方案规划示例

```cpp
// 创建方案
PlanFileManager* planManager = new PlanFileManager(this);
planManager->setEntityManager(entityManager);
planManager->createPlan("测试方案", "这是一个测试方案");

// 添加实体到方案
GeoEntity* entity = entityManager->createEntity(...);
planManager->addEntityToPlan(entity);

// 保存方案
planManager->savePlan("test_plan.json");

// 加载方案
planManager->loadPlan("test_plan.json");

// 恢复相机视角
double lon, lat, alt, heading, pitch, range;
if (planManager->getCameraViewpoint(lon, lat, alt, heading, pitch, range)) {
    osgEarth::Viewpoint vp("", lon, lat, alt, heading, pitch, range);
    mapWidget->getViewer()->getCameraManipulator()->setViewpoint(vp);
}
```

## 🔧 扩展指南

### 添加新实体类型

1. **继承GeoEntity基类**
   ```cpp
   #include "geo/geoentity.h"
   
   class CustomEntity : public GeoEntity {
   public:
       CustomEntity(const QString& name, double lon, double lat, double alt)
           : GeoEntity("custom", name, lon, lat, alt) {}
   
   protected:
       // 必须实现：创建OSG节点
       osg::Node* createNode() override {
           osg::ref_ptr<osg::Geode> geode = new osg::Geode;
           // 添加你的几何体或模型
           return geode.release();
       }
       
       // 可选：重写回调方法
       void onInitialized() override {
           // 初始化完成后的自定义逻辑
       }
       
       void onUpdated() override {
           // 更新完成后的自定义逻辑（如更新标签）
       }
   };
   ```

2. **在GeoEntityManager中注册**
   ```cpp
   GeoEntity* GeoEntityManager::createEntity(...) {
       if (entityType == "custom") {
           return new CustomEntity(entityName, longitude, latitude, altitude);
       }
       // ...
   }
   ```

### 添加新交互功能

1. **在GeoEntityManager中添加事件处理**
   ```cpp
   void GeoEntityManager::onMousePress(QMouseEvent* event) {
       // 添加你的自定义鼠标事件处理
   }
   ```

2. **定义新信号（如需要）**
   ```cpp
   signals:
       void customEvent(GeoEntity* entity);
   ```

3. **在MainWidget中连接信号**
   ```cpp
   connect(entityManager, &GeoEntityManager::customEvent,
           this, &MainWidget::handleCustomEvent);
   ```

## 📦 依赖关系

```
MainWidget
├── OsgMapWidget ──→ GeoEntityManager ──→ GeoEntity
│                      └─→ MapStateManager
├── PlanFileManager ──→ GeoEntityManager
│     └─→ DatabaseUtils (仅用于初始加载)
├── ModelDeployDialog ──→ DatabaseUtils
├── ComponentConfigDialog ──→ DatabaseUtils
└── ModelAssemblyDialog ──→ DatabaseUtils

GeoEntityManager
├── GeoEntity (基类)
│   ├── ImageEntity
│   └── WaypointEntity
└── MapStateManager
```

## 📝 数据存储

### 方案文件格式

方案文件使用JSON格式存储，包含：
- **metadata** - 方案元数据（名称、描述、创建时间等）
- **entities** - 实体列表（包含完整的组件信息深层复制）
- **camera** - 相机视角信息
- **version** - 文件格式版本

### 组件信息深层复制

方案文件中的组件信息包含完整的配置：
- `componentId` - 组件ID
- `name` - 组件名称
- `type` - 组件类型
- `configInfo` - 完整配置信息
- `templateInfo` - 模板信息（用于编辑）

这使得方案文件完全独立，可在无数据库环境下使用。

## 🔍 工具类使用

### GeoUtils - 地理工具类

```cpp
// 屏幕坐标转地理坐标
double lon, lat, alt;
if (GeoUtils::screenToGeoCoordinates(viewer, mapNode, screenPos, lon, lat, alt)) {
    qDebug() << "地理坐标:" << lon << lat << alt;
}

// 获取EarthManipulator
osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
if (em) {
    osgEarth::Viewpoint vp("", lon, lat, alt, heading, pitch, range);
    em->setViewpoint(vp, 2.0);  // 2秒动画过渡
}
```

### DatabaseUtils - 数据库工具类

```cpp
// 设置数据库路径
DatabaseUtils::setDatabasePath("path/to/database.db");

// 打开数据库
if (DatabaseUtils::openDatabase()) {
    // 执行查询
    QSqlQuery query;
    query.exec("SELECT * FROM ModelInformation");
    // ...
}
```

## 📖 版本信息

- **当前版本**: 2.2.0
- **最新更新**: 2025-11-05
- **文档版本**: 1.0

## 📚 生成文档

### Windows环境

1. **安装Doxygen**
   - 访问 https://www.doxygen.nl/download.html
   - 下载并安装Windows版本

2. **生成文档**
   ```cmd
   doxygen Doxyfile
   ```

3. **查看文档**
   - 打开 `docs/html/index.html`

### 文档导航

- **Modules** - 查看模块分组（Geo Entities、Managers）
- **Classes** - 查看所有类列表
- **Files** - 查看文件列表
- **搜索** - 使用顶部搜索框快速查找

## 📄 许可证

请参考项目根目录的LICENSE文件。

## 🔗 相关文档

- **README.md** - 项目概述和快速开始指南
- **WorkRecord.md** - 开发历史和更新日志
- **API文档** - 使用Doxygen生成的完整API文档

---

**注意**: 本项目需要预先安装OSG和osgEarth库。请确保这些库正确配置在系统环境变量中，或在项目配置文件中指定正确的路径。
