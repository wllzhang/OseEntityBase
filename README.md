# 数字地图脚手架库 (Digital Map Scaffold Library)

## 项目概述

目标是成为一个脚手架的数字地图库, 对GeoEntity进行封装, 对其状态进行管理, 可对其进行渲染, 同时将其数字地图上的任务GeoEntity和数字地图的状态信息抽象为一个信息层, 传递给其他服务.

## 技术栈

- **Qt 5/6**: GUI框架
- **OSG (OpenSceneGraph)**: 3D图形渲染引擎
- **osgEarth**: 基于OSG的地球渲染库
- **C++11**: 编程语言标准

## 项目结构

```
项目根目录/
├── geo/                           # 地理实体模块
│   ├── geoentity.h/cpp           # 通用地理实体基类
│   ├── imageentity.h/cpp         # 图片实体实现
│   ├── geoentitymanager.h/cpp    # 通用实体管理器
│   └── mapstatemanager.h/cpp     # 地图状态管理器
├── OsgQt/                        # OSG-Qt集成组件
├── images/                       # 实体图片资源
├── earth/                        # 地球配置文件
├── main.cpp                      # 程序入口
├── mainwindow.h/cpp             # 主窗口
├── imageviewerwindow.h/cpp       # 图片查看器
├── draggablelistwidget.h/cpp     # 可拖拽列表
├── images_config.json           # 实体配置文件
└── osgEarth.pro                 # 项目文件
```

## 核心功能

### ✅ 3D地图显示
- 基于osgEarth的3D地球渲染
- 高德地图瓦片集成
- 地球漫游控制（缩放、旋转、平移）
- 相机视角管理

### ✅ 实体管理系统
- 通用GeoEntity基类设计
- ImageEntity图片实体实现
- GeoEntityManager统一管理
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

### ✅ 事件处理架构
- GeoEntityManager统一处理鼠标事件
- Qt信号槽机制进行组件间通信
- 多态支持实现实体选择高亮
- 职责分离设计

## 待完成任务


1. 航向角设置有问题
2. 拖拽添加是坐标不精确
3. 选中时可以根据当前range 调整选中阈值大小
4. 选中后视角移动有问题, 不可以滚轮键调整了.
5. 路线规划功能, 地图上点击几个点, 使用贝塞尔模型,线性模型等连线, 然后将轨迹绑定到这个entity上
 
5. **信息层抽象**
   - 地图状态信息抽象
   - 实体状态信息抽象
   - 服务接口设计
 

## 使用说明

### 基本操作

#### 实体管理
1. **添加实体** - 从左侧实体列表拖拽到地图上
2. **选择实体** - 左键点击实体进行选择（显示红色边框）
3. **取消选择** - 左键点击空白区域
4. **删除实体** - 右键点击实体 → 选择"删除实体"

#### 实体属性设置
1. **设置航向角** - 右键点击实体 → 选择"设置航向角" → 输入角度值
2. **设置高度** - 右键点击实体 → 选择"设置高度" → 输入高度值
3. **查看属性** - 右键点击实体 → 选择"显示属性"

#### 视图切换
1. **2D/3D切换** - 点击右上角"切换到2D"/"切换到3D"按钮
2. **视角跳转** - 拖拽添加实体后自动跳转到实体位置

### 技术特性

#### 实体选择算法
- 使用距离计算算法，选择距离鼠标点击位置最近的实体
- 距离阈值：0.01度（约1公里）
- 支持多实体场景下的精确选择

#### 事件处理架构
- **GeoEntityManager** - 统一处理所有鼠标事件
- **信号槽机制** - 组件间通过Qt信号槽通信
- **多态支持** - 虚函数实现实体选择的高亮功能

#### 坐标系统
- **屏幕坐标** - Qt鼠标事件坐标
- **地理坐标** - 经纬度坐标系统
- **世界坐标** - OSG 3D世界坐标
- **自动转换** - 各坐标系间的自动转换

## 开发说明

### 架构设计原则

#### 1. 职责分离
- **GeoEntityManager** - 负责实体管理和鼠标事件处理
- **MainWindow** - 负责UI显示和用户交互
- **GeoEntity** - 负责实体自身的状态管理

#### 2. 信号槽通信
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

#### 3. 多态设计
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

### 扩展指南

#### 添加新实体类型
1. 继承`GeoEntity`基类
2. 实现纯虚函数：`initialize()`, `update()`, `cleanup()`
3. 重写`setSelected()`方法（可选）
4. 在`GeoEntityManager::createEntity()`中添加类型判断

#### 添加新交互功能
1. 在`GeoEntityManager::onMousePress()`中添加事件处理
2. 定义新的信号（如需要）
3. 在`MainWindow`中连接信号并实现UI响应

## 更新日志

### v1.0.0 (2025-10-24)
**初始版本 - 基础功能实现**

#### 🎯 核心功能
- **3D地图显示** - 基于osgEarth的3D地球渲染
- **实体管理系统** - GeoEntity基类和ImageEntity实现
- **拖拽功能** - 从列表拖拽实体到地图
- **MapStateManager支持** - 地图状态管理

#### 🔧 技术实现
- **Qt-OSG集成** - GraphicsWindowQt组件
- **实体配置** - JSON配置文件支持
- **模块化设计** - geo模块独立管理

### v1.1.0 (2025-10-24)
**2D/3D视图切换功能**

#### 🎯 新功能
- **2D/3D切换按钮** - 动态切换地图显示模式
- **自定义视角配置** - 2D和3D模式使用不同视角参数
- **视角自动跳转** - 切换模式时自动调整到预设视角

#### 📐 视角配置
- **2D视角参数** - Pitch: -90°, Heading: -0.916737°, Range: 540978
- **3D视角参数** - Pitch: -76.466°, Heading: 0°, Range: 12725200

### v1.2.0 (2025-10-24)
**拖拽功能优化**

#### 🔧 技术改进
- **精确坐标定位** - 拖拽时使用鼠标实际地理坐标
- **相机视角跳转** - 拖拽后自动调整视角到实体位置
- **坐标转换优化** - 屏幕坐标到地理坐标的准确转换

### v1.3.0 (2025-10-24)
**实体交互功能完善**

#### 🎯 核心功能
- **实体选择系统** - 点击实体进行选择，支持高亮显示
- **右键上下文菜单** - 完整的实体操作菜单系统
- **实体属性设置** - 航向角和高度设置功能

#### 🔧 技术改进
- **事件处理架构重构** - GeoEntityManager统一处理鼠标事件
- **信号槽机制优化** - 使用Qt信号槽进行组件间通信
- **多态支持完善** - 虚函数实现实体选择的高亮功能
- **职责分离设计** - MainWindow专注UI，GeoEntityManager专注实体交互

#### 🎨 交互功能
- **实体高亮显示** - 选中实体显示红色边框
- **航向角设置** - 通过对话框修改实体朝向
- **高度设置** - 通过对话框修改实体高度
- **实体删除** - 确认删除选中的实体
- **属性查看** - 显示实体的详细信息