# 数字地图脚手架库 API文档

## 项目概述

数字地图脚手架库是一个基于Qt和OSG的3D数字地图应用框架，提供地理实体管理、地图状态监控、实体交互等功能。

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

## 架构设计

### 职责分离

- **GeoEntityManager** - 负责实体管理和鼠标事件处理
- **MainWindow** - 负责UI显示和用户交互
- **GeoEntity** - 负责实体自身的状态管理

### 信号槽通信

组件间通过Qt信号槽机制通信，保持松耦合：

```cpp
// 实体选择信号
connect(entityManager_, &GeoEntityManager::entitySelected, 
        this, [this](GeoEntity* entity) {
            selectedEntity_ = entity;
        });

// 右键菜单信号
connect(entityManager_, &GeoEntityManager::entityRightClicked, 
        this, [this](GeoEntity* entity, QPoint screenPos) {
            showEntityContextMenu(screenPos, entity);
        });
```

### 多态设计

```cpp
// 基类虚函数
class GeoEntity {
public:
    virtual void setSelected(bool selected);
};

// 子类重写
class ImageEntity : public GeoEntity {
public:
    void setSelected(bool selected) override;
};
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

### 创建实体

```cpp
// 创建实体管理器
GeoEntityManager* entityManager = new GeoEntityManager(root, mapNode, this);

// 创建实体
GeoEntity* entity = entityManager->createEntity(
    "aircraft",           // 实体类型
    "F-15战斗机",         // 实体名称
    QJsonObject(),        // 属性
    116.4,                // 经度
    39.9,                 // 纬度
    1000.0                // 高度
);
```

### 实体交互

```cpp
// 连接实体选择信号
connect(entityManager, &GeoEntityManager::entitySelected, 
        this, [](GeoEntity* entity) {
            qDebug() << "选中实体:" << entity->getName();
        });

// 连接右键菜单信号
connect(entityManager, &GeoEntityManager::entityRightClicked, 
        this, [](GeoEntity* entity, QPoint pos) {
            // 显示右键菜单
        });
```

### 地图状态监控

```cpp
// 创建地图状态管理器
MapStateManager* mapStateManager = new MapStateManager(viewer, this);

// 连接状态变化信号
connect(mapStateManager, &MapStateManager::mapStateChanged, 
        this, [](const MapStateInfo& state) {
            qDebug() << "俯仰角:" << state.pitch;
            qDebug() << "航向角:" << state.heading;
            qDebug() << "距离:" << state.range;
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

- **当前版本**: 1.3.0
- **发布日期**: 2025-10-24

## 许可证

请参考项目根目录的LICENSE文件。

## 联系方式

- **项目地址**: https://github.com/your-repo/osgEarthmy_osgb
- **问题反馈**: https://github.com/your-repo/osgEarthmy_osgb/issues

