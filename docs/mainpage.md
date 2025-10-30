@mainpage 数字地图脚手架库 (API 文档)

欢迎使用数字地图脚手架库。本项目基于 Qt、OpenSceneGraph (OSG) 与 osgEarth，围绕“实体（Geo Entities）- 管理器（Managers）- 视图（OSG/Qt 集成）”的分层架构，提供标准化的实体生命周期管理、地图状态监控与交互扩展能力。

## 核心模块

### 实体管理模块 (geo/)

- **GeoEntity** - 通用地理实体基类
- **ImageEntity** - 图片实体实现
- **GeoEntityManager** - 实体管理器
- **MapStateManager** - 地图状态管理器

### 主窗口模块

- **MainWindow** - 应用程序主窗口
- **ImageViewerWindow** - 图片查看器窗口

### OSG-Qt集成模块 (OsgQt/)

- **GraphicsWindowQt** - Qt与OSG的集成组件

## 模块分组（Doxygen Groups）

以下章节与代码中的 Doxygen 分组一致，便于在生成文档中快速导航：

- \ref geo_entities "Geo Entities" — 通用地理实体与实现（`GeoEntity`、`ImageEntity`、`WaypointEntity`）
- \ref managers "Managers" — 实体与地图管理（`GeoEntityManager`、`MapStateManager`）

## 架构概览

```
Qt UI (MainWindow)
   │
   ├─ Managers
   │    ├─ GeoEntityManager  ── 实体创建/查询/选择/右键/延时删除/路线API
   │    └─ MapStateManager   ── 俯仰/航向/距离/鼠标与视角地理状态
   │
   └─ Geo Entities
        ├─ GeoEntity (抽象基类)
        ├─ ImageEntity (图片实体，高亮)
        └─ WaypointEntity (航点/标绘/序号标签)

OSG SceneGraph + osgEarth MapNode
```

## 主要功能

### 3D地图显示
- 基于osgEarth的3D地球渲染
- 高德地图瓦片集成
- 地球漫游控制（缩放、旋转、平移）
- 相机视角管理

### 实体管理
- 实体的创建、删除、查询
- 拖拽添加实体
- 实体位置和朝向控制
- 实体属性管理

### 2D/3D视图切换
- 动态切换地图显示模式
- 自定义视角配置
- 视角自动跳转

### 实体交互
- 实体选择系统（点击选择）
- 实体高亮显示（红色边框）
- 右键上下文菜单
- 航向角和高度设置
- 实体删除和属性查看

## 快速开始

### 1) 创建实体管理器
```cpp
// root: osg::Group*; mapNode: osgEarth::MapNode*
auto* entityManager = new GeoEntityManager(root, mapNode, this);
```

### 2) 创建实体
```cpp
GeoEntity* aircraft = entityManager->createEntity(
    "aircraft",            // 实体类型
    "F-15",                // 实体名称
    QJsonObject{},          // 属性
    116.4, 39.9, 1000.0     // 经度、纬度、高度
);
```

### 3) 连接交互信号
```cpp
connect(entityManager, &GeoEntityManager::entitySelected, this, [](GeoEntity* e){
    qDebug() << "selected:" << e->getName();
});
connect(entityManager, &GeoEntityManager::entityRightClicked, this, [](GeoEntity* e, QPoint p){
    // 展示右键菜单
});
```

### 4) 地图状态监控
```cpp
auto* mapStateManager = new MapStateManager(viewer, this);
connect(mapStateManager, &MapStateManager::stateChanged, this, [](const MapStateInfo& s){
    qDebug() << s.pitch << s.heading << s.range;
});
```

## 扩展指南

### 添加新实体类型

1. 继承`GeoEntity`基类
2. 实现纯虚函数：`initialize()`, `update()`, `cleanup()`
3. 重写`setSelected()`方法（可选）
4. 在`GeoEntityManager::createEntity()`中添加类型判断

### 添加新交互功能

1. 在`GeoEntityManager::onMousePress()`中添加事件处理
2. 定义新的信号（如需要）
3. 在`MainWindow`中连接信号并实现UI响应

## 版本信息

- **当前版本**: 2.0.0
- **发布日期**: 2025-10-24

## 生成文档

- 保持 Doxyfile 默认配置，运行 doxygen 即可生成文档。
- 分组导航位于 “Modules”，或通过上文分组链接快速进入。

## 许可证

请参考项目根目录的LICENSE文件。

## 联系方式

- **项目地址**: https://github.com/OseEntityBase/osgEarthmy_osgb
- **问题反馈**: https://github.com/OseEntityBase/osgEarthmy_osgb/issues

