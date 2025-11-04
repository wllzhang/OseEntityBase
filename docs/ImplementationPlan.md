# 方案规划功能实现计划

## 概述

本计划基于`PlanFileFormat.md`文档，按照模块化和渐进式开发原则，分阶段实现方案规划功能。

## 实施策略

- **阶段1（基础）**：核心文件管理和基本拖拽功能
- **阶段2（扩展）**：完整的属性编辑和方案管理
- **阶段3（完善）**：优化和扩展功能

---

## 阶段1：核心功能实现（优先级：高）

### 目标
实现基本的方案文件管理和模型拖拽部署功能。

### 任务清单

#### 1.1 创建PlanFileManager类（核心）
**文件**：`plan/planfilemanager.h`, `plan/planfilemanager.cpp`

**实现内容**：
- [ ] 类定义和基础成员变量
- [ ] `createPlan()` - 创建新方案文件
- [ ] `savePlan()` - 保存方案文件（基础版本，只保存规划属性）
- [ ] `loadPlan()` - 加载方案文件（基础版本）
- [ ] `getCurrentPlanFile()` - 获取当前方案文件路径
- [ ] `setCurrentPlanFile()` - 设置当前方案文件路径
- [ ] `entityToJson()` - 实体序列化（基础版本，只序列化规划属性）
- [ ] `jsonToEntity()` - 实体反序列化（基础版本）
- [ ] 数据库辅助方法：`getModelInfoFromDatabase()` - 从数据库获取模型信息
- [ ] 信号：`planFileChanged`, `planSaved`, `planLoaded`

**依赖**：
- `util/databaseutils.h` - 数据库操作
- `geo/geoentitymanager.h` - 实体管理

**预计工作量**：4-6小时

---

#### 1.2 创建ModelDeployDialog类
**文件**：`ModelDeployDialog.h`, `ModelDeployDialog.cpp`

**实现内容**：
- [ ] 类定义和UI布局（左侧列表，右侧预览）
- [ ] `loadModelsFromDatabase()` - 从数据库加载模型列表
- [ ] `populateModelList()` - 填充模型列表到DraggableListWidget
- [ ] `setupDragAndDrop()` - 设置拖拽功能（复用DraggableListWidget）
- [ ] 拖拽数据格式：`"modeldeploy:{modelId}:{modelName}"`
- [ ] 预览功能：显示选中模型的图标和名称

**依赖**：
- `draggablelistwidget.h` - 可拖拽列表控件
- `util/databaseutils.h` - 数据库操作

**预计工作量**：3-4小时

---

#### 1.3 在OsgMapWidget中实现拖拽处理
**文件**：`OsgMapWidget.h`, `OsgMapWidget.cpp`

**实现内容**：
- [ ] 重写`dragEnterEvent()` - 检查拖拽数据格式
- [ ] 重写`dropEvent()` - 处理拖拽放下事件
- [ ] 坐标转换：屏幕坐标 → 地理坐标（参考MainWindow的实现）
- [ ] 创建实体时：
  - 调用`GeoEntityManager::createEntity()`
  - 设置实体属性：`planFile`, `planEntityId`, `modelId`
  - 调用`PlanFileManager::addEntityToPlan()`添加到方案
- [ ] 启用拖放：`setAcceptDrops(true)`

**依赖**：
- `plan/planfilemanager.h`
- `geo/geoentitymanager.h`

**预计工作量**：3-4小时

---

#### 1.4 在GeoEntityManager中添加方案相关方法
**文件**：`geo/geoentitymanager.h`, `geo/geoentitymanager.cpp`

**实现内容**：
- [ ] `getEntitiesByPlanFile(const QString& planFile)` - 获取属于指定方案的实体列表
- [ ] `clearEntitiesByPlanFile(const QString& planFile)` - 清除指定方案的所有实体（可选）

**预计工作量**：1小时

---

#### 1.5 在MainWidget中集成方案管理
**文件**：`MainWidget.h`, `MainWidget.cpp`

**实现内容**：
- [ ] 添加成员变量：
  - `PlanFileManager* planFileManager_`
  - `ModelDeployDialog* modelDeployDialog_`
- [ ] 添加方案管理UI（工具栏或菜单）：
  - "新建方案"按钮
  - "打开方案"按钮
  - "保存方案"按钮
  - 当前方案名称显示
- [ ] `onModelDeployBtnClicked()` - 处理"模型部署"按钮点击
  - 如果当前没有方案，弹出方案选择对话框（新建/打开）
  - 如果有方案，打开`ModelDeployDialog`
- [ ] `onNewPlanClicked()` - 创建新方案
- [ ] `onOpenPlanClicked()` - 打开现有方案
- [ ] `onSavePlanClicked()` - 保存当前方案
- [ ] 连接`PlanFileManager`的信号到相应槽函数

**预计工作量**：4-5小时

---

#### 1.6 创建方案选择对话框（新建/打开）
**文件**：`PlanSelectDialog.h`, `PlanSelectDialog.cpp`（可选，也可以使用QFileDialog）

**实现内容**：
- [ ] 对话框：新建方案 / 打开现有方案
- [ ] 新建方案：输入方案名称和描述
- [ ] 打开方案：文件选择对话框，过滤`.plan.json`文件

**预计工作量**：1-2小时（如果使用QFileDialog可以减少工作量）

---

### 阶段1测试要点
- [ ] 可以创建新方案文件
- [ ] 可以从模型部署对话框拖拽模型到地图
- [ ] 实体正确创建并关联到方案文件
- [ ] 可以保存方案文件
- [ ] 可以加载方案文件并恢复实体位置

---

## 阶段2：完整属性编辑功能（优先级：高）

### 目标
实现完整的实体属性编辑对话框，支持编辑所有属性类型。

### 任务清单

#### 2.1 创建EntityPropertyDialog类（核心）
**文件**：`EntityPropertyDialog.h`, `EntityPropertyDialog.cpp`

**实现内容**：
- [ ] 类定义和UI布局（使用QTabWidget）
- [ ] **基本信息标签页**：
  - [ ] 实体名称编辑（QLineEdit）
  - [ ] 模型信息显示（只读，QLabel）
- [ ] **规划属性标签页**：
  - [ ] 位置：经度、纬度、高度（QDoubleSpinBox）
  - [ ] 航向角（QDoubleSpinBox）
  - [ ] 可见性（QCheckBox）
- [ ] **模型组装属性标签页**：
  - [ ] 部署位置（QComboBox：空中/地面/海面）
  - [ ] 二维军标（QLineEdit + QPushButton文件选择）
  - [ ] 组件列表（QListWidget，显示组件名称，可添加/移除）
- [ ] **组件配置标签页**：
  - [ ] 动态生成组件配置表单（参考ComponentConfigDialog）
  - [ ] 为每个组件创建独立的表单区域
  - [ ] 根据`templateInfo`动态生成控件
- [ ] `loadEntityData()` - 加载实体数据到表单
- [ ] `loadModelAssemblyData()` - 从数据库加载模型组装数据
- [ ] `loadComponentConfigs()` - 从数据库加载组件配置和模板
- [ ] `onSaveClicked()` - 保存修改：
  - [ ] 更新实体属性（位置、航向等）
  - [ ] 比较模型组装属性与数据库默认值，保存差异
  - [ ] 比较组件配置与数据库默认值，保存差异
  - [ ] 调用`PlanFileManager::updateEntityInPlan()`
- [ ] `onCancelClicked()` - 取消修改

**依赖**：
- `plan/planfilemanager.h`
- `util/databaseutils.h`
- `ComponentConfigDialog.h`（参考动态表单生成）

**预计工作量**：8-10小时

---

#### 2.2 增强PlanFileManager的序列化功能
**文件**：`plan/planfilemanager.cpp`

**实现内容**：
- [ ] 完善`entityToJson()`：
  - [ ] 比较模型组装属性与数据库默认值
  - [ ] 只保存有差异的属性到`modelAssembly`
  - [ ] 比较组件配置与数据库默认值
  - [ ] 只保存有差异的配置到`componentConfigs`
- [ ] 完善`jsonToEntity()`：
  - [ ] 从数据库加载模型默认信息
  - [ ] 应用方案文件中的覆盖值
  - [ ] 设置实体的所有属性

**预计工作量**：3-4小时

---

#### 2.3 连接实体选择事件
**文件**：`MainWidget.cpp` 或 `OsgMapWidget.cpp`

**实现内容**：
- [ ] 实体双击事件 → 打开`EntityPropertyDialog`
- [ ] 实体右键菜单 → 添加"编辑属性"选项
- [ ] 只对属于当前方案的实体启用编辑

**预计工作量**：1-2小时

---

#### 2.4 实现实体删除功能
**文件**：`plan/planfilemanager.cpp`, `MainWidget.cpp`

**实现内容**：
- [ ] `PlanFileManager::removeEntityFromPlan()` - 从方案中移除实体
- [ ] 右键菜单添加"删除"选项
- [ ] 删除确认对话框
- [ ] 删除后更新方案文件

**预计工作量**：2小时

---

### 阶段2测试要点
- [ ] 可以双击实体打开属性编辑对话框
- [ ] 可以编辑所有类型的属性
- [ ] 修改后的属性正确保存到方案文件
- [ ] 只保存与数据库默认值不同的配置
- [ ] 加载方案时正确应用覆盖值

---

## 阶段3：完善和优化（优先级：中）

### 目标
优化用户体验，添加扩展功能。

### 任务清单

#### 3.1 添加自动保存功能
**文件**：`plan/planfilemanager.cpp`

**实现内容**：
- [ ] 添加自动保存选项（设置或配置）
- [ ] 每次实体操作后自动保存（可选）
- [ ] 防抖处理，避免频繁保存

**预计工作量**：1-2小时

---

#### 3.2 添加方案文件管理功能
**文件**：`MainWidget.cpp`

**实现内容**：
- [ ] "另存为"功能
- [ ] "最近打开的文件"列表
- [ ] 方案文件列表对话框（显示所有方案文件）

**预计工作量**：2-3小时

---

#### 3.3 添加相机视角保存/恢复
**文件**：`plan/planfilemanager.cpp`, `OsgMapWidget.cpp`

**实现内容**：
- [ ] 保存方案时保存当前相机视角
- [ ] 加载方案时恢复相机视角

**预计工作量**：2小时

---

#### 3.4 优化UI和用户体验
**实现内容**：
- [ ] 添加加载/保存进度提示
- [ ] 错误提示和日志
- [ ] 方案文件有效性验证

**预计工作量**：2-3小时

---

## 文件修改清单

### 新增文件
```
plan/
  ├── planfilemanager.h
  └── planfilemanager.cpp

ModelDeployDialog.h
ModelDeployDialog.cpp
EntityPropertyDialog.h
EntityPropertyDialog.cpp
PlanSelectDialog.h          (可选)
PlanSelectDialog.cpp        (可选)
```

### 修改文件
```
MainWidget.h                (+ PlanFileManager, ModelDeployDialog成员)
MainWidget.cpp              (+ 方案管理UI和方法)
OsgMapWidget.h              (+ 拖拽相关方法声明)
OsgMapWidget.cpp            (+ dragEnterEvent, dropEvent实现)
geo/geoentitymanager.h      (+ getEntitiesByPlanFile方法)
geo/geoentitymanager.cpp    (+ 方案相关方法实现)
ScenePlan2.pro              (+ 新增文件到项目)
```

---

## 开发顺序建议

### 第一步：搭建基础框架
1. 创建`PlanFileManager`类（基础版本）
2. 创建`ModelDeployDialog`类
3. 在`MainWidget`中集成基础UI

### 第二步：实现拖拽功能
4. 在`OsgMapWidget`中实现拖拽处理
5. 测试完整的拖拽流程

### 第三步：完善文件管理
6. 完善`PlanFileManager`的序列化/反序列化
7. 添加方案选择对话框

### 第四步：实现属性编辑
8. 创建`EntityPropertyDialog`类
9. 实现所有标签页
10. 连接实体选择事件

### 第五步：测试和优化
11. 全面测试
12. 修复bug
13. 优化用户体验

---

## 关键实现点

### 1. 实体ID生成策略
```cpp
QString generatePlanEntityId() {
    static int counter = 0;
    return QString("entity_%1").arg(++counter);
}
```

### 2. 方案文件路径管理
```cpp
QString plansDirectory = QCoreApplication::applicationDirPath() + "/plans/";
QDir().mkpath(plansDirectory);  // 确保目录存在
```

### 3. 拖拽数据格式
```cpp
// 拖拽时
QString dragData = QString("modeldeploy:%1:%2").arg(modelId).arg(modelName);

// 接收时
QStringList parts = dragData.split(":");
if (parts[0] == "modeldeploy") {
    QString modelId = parts[1];
    QString modelName = parts[2];
}
```

### 4. 配置差异比较
```cpp
bool hasDifference(const QJsonObject& saved, const QJsonObject& db) {
    // 比较两个JSON对象是否不同
    return saved != db;
}
```

---

## 风险评估

### 高风险项
1. **数据库查询性能**：如果模型和组件数量很大，查询可能较慢
   - **缓解措施**：添加缓存机制，只查询需要的模型和组件

2. **坐标转换精度**：拖拽时的坐标转换可能不准确
   - **缓解措施**：参考MainWindow中已有的实现，确保一致性

3. **属性编辑对话框复杂度**：组件配置的动态表单生成较复杂
   - **缓解措施**：参考ComponentConfigDialog的实现，复用代码

### 中风险项
1. **方案文件格式兼容性**：未来可能需要支持不同版本的格式
   - **缓解措施**：在metadata中添加version字段，实现版本检查

2. **大量实体的性能**：方案文件中实体数量很大时，序列化/反序列化可能较慢
   - **缓解措施**：使用异步加载，添加进度提示

---

## 测试计划

### 单元测试
- [ ] PlanFileManager的序列化/反序列化
- [ ] 实体属性比较逻辑
- [ ] 坐标转换精度

### 集成测试
- [ ] 完整的拖拽流程
- [ ] 方案文件保存/加载
- [ ] 属性编辑和保存

### 用户测试
- [ ] 创建新方案
- [ ] 拖拽多个模型
- [ ] 编辑实体属性
- [ ] 保存和加载方案

---

## 预计总工作量

- **阶段1**：15-20小时
- **阶段2**：14-18小时
- **阶段3**：7-10小时
- **总计**：36-48小时

---

## 后续扩展（可选）

1. **方案模板**：支持方案模板功能
2. **方案版本管理**：支持方案版本和回滚
3. **方案导出**：导出为KML、GeoJSON等格式
4. **方案合并**：合并多个方案文件
5. **方案对比**：对比两个方案的差异

---

**计划版本**：1.0  
**制定日期**：2024-01-01  
**最后更新**：2024-01-01

