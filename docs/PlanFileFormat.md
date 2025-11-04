# 方案规划文件格式设计文档

## 1. 概述

方案规划功能采用**结构化文件**方式存储规划数据，而非数据库。每个方案保存为一个独立的JSON文件，包含完整的规划信息，便于备份、分享、版本管理和导出。

## 2. 文件格式设计

### 2.1 文件结构

```json
{
  "version": "1.0",
  "metadata": {
    "name": "方案名称",
    "description": "方案描述",
    "author": "创建者",
    "createTime": "2024-01-01T10:00:00",
    "updateTime": "2024-01-01T12:00:00",
    "coordinateSystem": "WGS84"
  },
  "entities": [
    {
      "id": "entity_001",
      "name": "F-15战斗机_01",
      "type": "aircraft",
      "modelName": "F-15战斗机",
      "position": {
        "longitude": 116.3974,
        "latitude": 39.9093,
        "altitude": 10000.0
      },
      "heading": 45.0,
      "visible": true,
      "properties": {
        "projectId": "plan_001",
        "customProperty1": "value1",
        "customProperty2": 123
      }
    }
  ],
  "waypoints": [
    {
      "id": "waypoint_001",
      "label": "航点1",
      "position": {
        "longitude": 116.3974,
        "latitude": 39.9093,
        "altitude": 0.0
      },
      "groupId": "route_001"
    }
  ],
  "routes": [
    {
      "id": "route_001",
      "name": "航线1",
      "waypointIds": ["waypoint_001", "waypoint_002"],
      "color": "#FF0000",
      "width": 2.0
    }
  ],
  "camera": {
    "viewpoint": {
      "longitude": 116.3974,
      "latitude": 39.9093,
      "altitude": 100000.0,
      "heading": 0.0,
      "pitch": -90.0,
      "range": 1000000.0
    }
  }
}
```

### 2.2 字段说明

> **重要说明**：实体属性分为三类：
> 1. **模型组装属性**：从数据库`ModelInformation`表获取（modelName, location, icon, componentList）
> 2. **模型组件属性**：从数据库`ComponentInformation`表获取（每个组件的configInfo配置）
> 3. **规划补充属性**：在规划阶段添加的位置、姿态等信息（position, heading, visible等）
> 
> **设计原则**：模型组件和模型组装的属性由资源管理模块管理，规划模块只负责补充和修改规划相关的属性（位置、姿态等）。加载方案时，需要从数据库重新获取最新的模型和组件配置。

#### 2.2.1 根对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `version` | string | 是 | 文件格式版本，用于兼容性检查 |
| `metadata` | object | 是 | 方案元数据 |
| `entities` | array | 是 | 实体列表（飞机、模型等） |
| `waypoints` | array | 否 | 航点列表 |
| `routes` | array | 否 | 航线列表 |
| `camera` | object | 否 | 相机视角信息 |

#### 2.2.2 metadata 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `name` | string | 是 | 方案名称 |
| `description` | string | 否 | 方案描述 |
| `author` | string | 否 | 创建者 |
| `createTime` | string | 是 | ISO 8601格式时间戳 |
| `updateTime` | string | 是 | ISO 8601格式时间戳 |
| `coordinateSystem` | string | 否 | 坐标系，默认WGS84 |

#### 2.2.3 entity 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 实体唯一ID（方案内唯一） |
| `name` | string | 是 | 实体显示名称（可自定义，默认使用模型名称） |
| `type` | string | 是 | 实体类型（aircraft/image等） |
| `modelId` | string | 是 | 模型ID（对应数据库ModelInformation表的id字段） |
| `modelName` | string | 是 | 模型名称（对应数据库ModelInformation表的name字段，用于快速识别） |
| `position` | object | 是 | **规划属性**：位置信息（longitude, latitude, altitude） |
| `heading` | number | 否 | **规划属性**：航向角（度），默认0 |
| `visible` | boolean | 否 | **规划属性**：可见性，默认true |
| `modelAssembly` | object | 否 | **模型组装属性**：模型的组装配置（可选，如果与数据库不同则覆盖） |
| `componentConfigs` | object | 否 | **模型组件属性**：组件配置覆盖（可选，key为componentId，value为configInfo） |
| `properties` | object | 否 | 其他自定义属性 |

**modelAssembly 对象结构**：
| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `location` | string | 否 | 部署位置（空中/地面/海面） |
| `icon` | string | 否 | 二维军标图片路径（绝对路径） |
| `componentList` | array | 否 | 组件ID列表（如果为空则使用数据库中的配置） |

**componentConfigs 对象结构**：
```json
{
  "componentConfigs": {
    "componentId_001": {
      "param1": "value1",
      "param2": 123,
      "param3": true
    },
    "componentId_002": {
      "param1": "value2"
    }
  }
}
```
> **说明**：`componentConfigs`中只存储与数据库默认配置不同的组件配置。如果某个组件ID不在`componentConfigs`中，则使用数据库中的配置。

**完整entity示例**：
```json
{
  "id": "entity_001",
  "name": "F-15战斗机_01",
  "type": "aircraft",
  "modelId": "model_123",
  "modelName": "F-15战斗机",
  "position": {
    "longitude": 116.3974,
    "latitude": 39.9093,
    "altitude": 10000.0
  },
  "heading": 45.0,
  "visible": true,
  "modelAssembly": {
    "location": "空中",
    "icon": "E:/path/to/icon.png",
    "componentList": ["comp_001", "comp_002"]
  },
  "componentConfigs": {
    "comp_001": {
      "power": 100,
      "enabled": true
    }
  },
  "properties": {
    "customTag": "重要目标"
  }
}
```

#### 2.2.4 position 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `longitude` | number | 是 | 经度（度） |
| `latitude` | number | 是 | 纬度（度） |
| `altitude` | number | 是 | 高度（米） |

#### 2.2.5 waypoint 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 航点唯一ID |
| `label` | string | 是 | 航点标签 |
| `position` | object | 是 | 位置信息 |
| `groupId` | string | 否 | 所属航线组ID |

#### 2.2.6 route 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 航线唯一ID |
| `name` | string | 否 | 航线名称 |
| `waypointIds` | array | 是 | 航点ID列表 |
| `color` | string | 否 | 航线颜色（十六进制） |
| `width` | number | 否 | 航线宽度（像素） |

#### 2.2.7 camera 对象

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `viewpoint` | object | 是 | 相机视角信息 |

## 3. 文件操作流程

### 3.1 方案文件管理

```
方案文件存储位置：项目根目录/plans/
文件命名规则：{方案名称}_{时间戳}.plan.json
示例：作战方案_20240101_100000.plan.json
```

### 3.2 工作流程

#### 3.2.1 创建新方案
( 这点还没实现. )
1. 用户点击"模型部署"按钮
2. 如果当前没有打开方案，弹出对话框：
   - 输入方案名称
   - 选择：新建方案 / 打开现有方案
3. 如果选择新建：
   - 创建新的方案文件
   - 设置当前方案文件路径
   - 进入规划模式

#### 3.2.2 规划操作

1. **拖拽实体到地图**
   - 从`ModelDeployDialog`拖拽模型到地图
   - 创建实体并添加到当前方案
   - 实时保存到方案文件

2. **修改实体属性**（**重要**：属性编辑对话框需要包含所有可编辑属性）
   - 点击实体 → 打开属性编辑对话框
   - 对话框包含以下选项卡：
     - **基本信息**：实体名称（可编辑）、模型信息（只读，显示modelName和modelId）
     - **规划属性**：位置（经纬度、高度）、航向角、可见性
     - **模型组装属性**：部署位置（location，下拉框：空中/地面/海面）、二维军标（icon，文件选择）、组件列表（componentList，显示当前模型包含的组件，可添加/移除组件）
     - **组件配置**：显示模型包含的所有组件，可以编辑每个组件的configInfo（动态表单，根据组件的templateInfo生成，参考ComponentConfigDialog的实现）
   - 修改后保存 → 更新方案文件
   
   > **实现注意**：
   > - 模型组件的`templateInfo`和默认`configInfo`从数据库`ComponentInformation`和`ComponentType`表获取
   > - 如果用户修改了组件配置，则在方案文件的`componentConfigs`中保存覆盖值
   > - 如果用户修改了模型组装属性，则在方案文件的`modelAssembly`中保存覆盖值
   > - 如果用户没有修改，则这些字段可以省略，加载时使用数据库中的默认值

3. **删除实体**
   - 右键删除 → 从方案文件中移除

#### 3.2.3 保存方案

- **自动保存**：每次操作后自动保存（可选）
- **手动保存**：用户点击"保存方案"按钮
- **另存为**：保存为新的方案文件

#### 3.2.4 加载方案

1. 选择方案文件
2. 解析JSON文件
3. 清空当前场景
4. 根据文件内容创建实体、航点、航线
5. 设置相机视角

## 4. 实体与方案的关联

### 4.1 关联方式

实体创建时通过`setProperty("planFile", planFilePath)`关联到方案文件：

```cpp
// 创建实体时
GeoEntity* entity = createEntity(...);
if (entity && !currentPlanFile_.isEmpty()) {
    entity->setProperty("planFile", currentPlanFile_);
    entity->setProperty("planEntityId", generateEntityId()); // 方案内的实体ID
}
```

### 4.2 方案实体过滤

提供方法获取属于当前方案的实体：

```cpp
QList<GeoEntity*> getEntitiesByPlanFile(const QString& planFile) {
    QList<GeoEntity*> result;
    for (auto entity : entities_) {
        if (entity->getProperty("planFile").toString() == planFile) {
            result.append(entity);
        }
    }
    return result;
}
```

## 5. 实现架构

### 5.1 类设计

#### 5.1.1 PlanFileManager（方案文件管理器）

```cpp
class PlanFileManager : public QObject
{
    Q_OBJECT
    
public:
    // 创建新方案
    bool createPlan(const QString& name, const QString& description = "");
    
    // 保存方案
    bool savePlan(const QString& filePath);
    
    // 加载方案
    bool loadPlan(const QString& filePath);
    
    // 获取当前方案文件路径
    QString getCurrentPlanFile() const;
    
    // 添加实体到方案
    void addEntityToPlan(GeoEntity* entity);
    
    // 移除实体
    void removeEntityFromPlan(const QString& entityId);
    
    // 更新实体
    void updateEntityInPlan(GeoEntity* entity);
    
signals:
    void planFileChanged(const QString& filePath);
    void planSaved(const QString& filePath);
    void planLoaded(const QString& filePath);
    
private:
    QString currentPlanFile_;
    QJsonObject currentPlanData_;
    
    // 序列化实体为JSON
    QJsonObject entityToJson(GeoEntity* entity);
    
    // 从JSON创建实体
    GeoEntity* jsonToEntity(const QJsonObject& json);
};
```

#### 5.1.2 ModelDeployDialog（模型部署对话框）

```cpp
class ModelDeployDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ModelDeployDialog(QWidget* parent = nullptr);
    
signals:
    // 拖拽信号，传递模型名称
    void modelDragged(const QString& modelName);
    
private:
    DraggableListWidget* modelListWidget_;
    QLabel* previewLabel_;
    
    void loadModelsFromDatabase();
    void setupDragAndDrop();
};
```

#### 5.1.3 EntityPropertyDialog（实体属性编辑对话框）

```cpp
class EntityPropertyDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit EntityPropertyDialog(GeoEntity* entity, QWidget* parent = nullptr);
    
private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onTabChanged(int index);
    
private:
    void setupUI();
    void loadEntityData();
    void loadModelAssemblyData();
    void loadComponentConfigs();
    
    // 基本信息标签页
    void setupBasicInfoTab();
    // 规划属性标签页
    void setupPlanningTab();
    // 模型组装属性标签页
    void setupModelAssemblyTab();
    // 组件配置标签页
    void setupComponentConfigTab();
    
    GeoEntity* entity_;
    QTabWidget* tabWidget_;
    
    // 基本信息
    QLineEdit* nameEdit_;
    QLabel* modelNameLabel_;
    QLabel* modelIdLabel_;
    
    // 规划属性
    QDoubleSpinBox* longitudeSpin_;
    QDoubleSpinBox* latitudeSpin_;
    QDoubleSpinBox* altitudeSpin_;
    QDoubleSpinBox* headingSpin_;
    QCheckBox* visibleCheck_;
    
    // 模型组装属性
    QComboBox* locationComboBox_;
    QLineEdit* iconEdit_;
    QPushButton* browseIconButton_;
    QListWidget* componentListWidget_;  // 显示组件列表
    
    // 组件配置
    QScrollArea* componentConfigScrollArea_;
    QWidget* componentConfigWidget_;
    QMap<QString, QMap<QString, QWidget*>> componentParamWidgets_;  // componentId -> (paramName -> widget)
    
    QString modelId_;
    QStringList componentIds_;  // 从数据库获取的组件列表
};
```

**关键实现点**：
1. **加载模型组装数据**：从数据库`ModelInformation`表查询`modelId`对应的`location`、`icon`、`componentList`
2. **加载组件配置**：遍历`componentList`，从数据库`ComponentInformation`表查询每个组件的`configInfo`和`templateInfo`
3. **动态生成组件配置表单**：根据`templateInfo`动态生成表单控件（参考`ComponentConfigDialog::createParameterForm`）
4. **保存时**：
   - 规划属性直接保存到entity和方案文件
   - 如果模型组装属性被修改，保存到方案文件的`modelAssembly`
   - 如果组件配置被修改，保存到方案文件的`componentConfigs`

### 5.2 文件结构

```
新增文件：
- plan/planfilemanager.h/cpp          // 方案文件管理器
- plan/planfilemanager.h/cpp          // 方案文件管理器
- ModelDeployDialog.h/cpp             // 模型部署对话框
- EntityPropertyDialog.h/cpp           // 实体属性编辑对话框

修改文件：
- MainWidget.h/cpp                    // 添加方案管理UI
- OsgMapWidget.h/cpp                  // 添加拖拽处理
- geo/geoentitymanager.h/cpp          // 添加方案相关查询方法
```

## 6. 实现步骤

### 步骤1：创建方案文件管理器

1. 创建`PlanFileManager`类
2. 实现方案文件的创建、保存、加载功能
3. 实现实体序列化/反序列化

### 步骤2：创建模型部署对话框

1. 创建`ModelDeployDialog`类
2. 从数据库加载模型列表
3. 实现拖拽功能

### 步骤3：实现拖拽处理

1. 在`OsgMapWidget`中实现拖拽事件处理
2. 创建实体时关联到当前方案
3. 实时更新方案文件

### 步骤4：实现属性编辑

1. 创建`EntityPropertyDialog`类
2. 连接实体双击/右键菜单
3. 保存时更新方案文件

### 步骤5：集成到MainWidget

1. 添加方案管理UI（新建/打开/保存）
2. 连接`ModelDeployDialog`
3. 管理方案文件生命周期

## 7. 优势

1. **独立性**：每个方案是独立的文件，便于管理
2. **可移植性**：方案文件可以轻松备份、分享、版本控制
3. **可扩展性**：JSON格式易于扩展新字段
4. **灵活性**：支持多个方案文件，不依赖数据库
5. **可读性**：JSON格式易于阅读和调试

## 8. 扩展考虑

### 8.1 方案模板

可以创建方案模板文件，快速创建新方案：

```json
{
  "template": true,
  "entities": [
    // 预定义的实体模板
  ]
}
```

### 8.2 方案版本管理

可以在metadata中添加版本信息：

```json
{
  "metadata": {
    "version": "1.0",
    "revision": 1,
    "parentVersion": null
  }
}
```

### 8.3 方案导出

支持导出为其他格式（如KML、GeoJSON等）：

```cpp
bool exportToKML(const QString& planFile, const QString& kmlFile);
bool exportToGeoJSON(const QString& planFile, const QString& geoJsonFile);
```

## 9. 示例代码片段

### 9.1 保存方案

```cpp
bool PlanFileManager::savePlan(const QString& filePath)
{
    QJsonObject planObject;
    planObject["version"] = "1.0";
    
    // 元数据
    QJsonObject metadata;
    metadata["name"] = planName_;
    metadata["createTime"] = createTime_;
    metadata["updateTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    planObject["metadata"] = metadata;
    
    // 实体列表
    QJsonArray entitiesArray;
    for (auto entity : getEntitiesByPlanFile(currentPlanFile_)) {
        entitiesArray.append(entityToJson(entity));
    }
    planObject["entities"] = entitiesArray;
    
    // 写入文件
    QJsonDocument doc(planObject);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));  // 使用缩进格式，便于阅读
    file.close();
    
    return true;
}

QJsonObject PlanFileManager::entityToJson(GeoEntity* entity)
{
    QJsonObject entityObj;
    
    // 基本信息
    entityObj["id"] = entity->getProperty("planEntityId").toString();
    entityObj["name"] = entity->getProperty("displayName").toString(entity->getName());
    entityObj["type"] = entity->getType();
    entityObj["modelId"] = entity->getProperty("modelId").toString();
    entityObj["modelName"] = entity->getName();  // 模型名称
    
    // 规划属性：位置
    double longitude, latitude, altitude;
    entity->getPosition(longitude, latitude, altitude);
    QJsonObject position;
    position["longitude"] = longitude;
    position["latitude"] = latitude;
    position["altitude"] = altitude;
    entityObj["position"] = position;
    
    // 规划属性：姿态和可见性
    entityObj["heading"] = entity->getHeading();
    entityObj["visible"] = entity->isVisible();
    
    // 模型组装属性（如果与数据库默认值不同，则保存）
    QJsonObject modelAssembly = entity->getProperty("modelAssembly").toJsonObject();
    QJsonObject dbModelAssembly = getModelAssemblyFromDatabase(entity->getProperty("modelId").toString());
    
    QJsonObject modelAssemblyOverride;
    bool hasModelAssemblyOverride = false;
    if (modelAssembly.contains("location") && 
        modelAssembly["location"].toString() != dbModelAssembly["location"].toString()) {
        modelAssemblyOverride["location"] = modelAssembly["location"].toString();
        hasModelAssemblyOverride = true;
    }
    if (modelAssembly.contains("icon") && 
        modelAssembly["icon"].toString() != dbModelAssembly["icon"].toString()) {
        modelAssemblyOverride["icon"] = modelAssembly["icon"].toString();
        hasModelAssemblyOverride = true;
    }
    if (modelAssembly.contains("componentList")) {
        QJsonArray savedList = modelAssembly["componentList"].toArray();
        QJsonArray dbList = dbModelAssembly["componentList"].toArray();
        if (savedList != dbList) {
            modelAssemblyOverride["componentList"] = savedList;
            hasModelAssemblyOverride = true;
        }
    }
    
    if (hasModelAssemblyOverride) {
        entityObj["modelAssembly"] = modelAssemblyOverride;
    }
    
    // 组件配置覆盖（只保存与数据库默认值不同的配置）
    QJsonObject componentConfigs = entity->getProperty("componentConfigs").toJsonObject();
    QJsonObject componentConfigsOverride;
    
    for (auto it = componentConfigs.begin(); it != componentConfigs.end(); ++it) {
        QString componentId = it.key();
        QJsonObject savedConfig = it.value().toObject();
        QJsonObject dbConfig = getComponentConfigFromDatabase(componentId);
        
        // 比较配置是否不同
        if (savedConfig != dbConfig) {
            componentConfigsOverride[componentId] = savedConfig;
        }
    }
    
    if (!componentConfigsOverride.isEmpty()) {
        entityObj["componentConfigs"] = componentConfigsOverride;
    }
    
    return entityObj;
}
```

### 9.2 加载方案

```cpp
bool PlanFileManager::loadPlan(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject planObject = doc.object();
    
    // 清空当前场景
    entityManager_->clearAllEntities();
    
    // 加载实体
    QJsonArray entitiesArray = planObject["entities"].toArray();
    for (const auto& entityValue : entitiesArray) {
        QJsonObject entityObj = entityValue.toObject();
        GeoEntity* entity = jsonToEntity(entityObj);
        if (entity) {
            entity->setProperty("planFile", filePath);
            entity->setProperty("planEntityId", entityObj["id"].toString());
            entity->setProperty("modelId", entityObj["modelId"].toString());
            
            // 保存模型组装和组件配置覆盖到entity属性中
            if (entityObj.contains("modelAssembly")) {
                QJsonObject modelAssembly = entityObj["modelAssembly"].toObject();
                entity->setProperty("modelAssembly", modelAssembly);
            }
            if (entityObj.contains("componentConfigs")) {
                QJsonObject componentConfigs = entityObj["componentConfigs"].toObject();
                entity->setProperty("componentConfigs", componentConfigs);
            }
            
            entityManager_->addEntity(entity);
        }
    }
    
    currentPlanFile_ = filePath;
    emit planLoaded(filePath);
    
    return true;
}

GeoEntity* PlanFileManager::jsonToEntity(const QJsonObject& json)
{
    QString modelId = json["modelId"].toString();
    QString modelName = json["modelName"].toString();
    
    // 从数据库获取模型信息（包括默认的location、icon、componentList）
    ModelInfo modelInfo = getModelInfoFromDatabase(modelId);
    
    // 如果方案文件中有覆盖，则使用覆盖值
    if (json.contains("modelAssembly")) {
        QJsonObject modelAssembly = json["modelAssembly"].toObject();
        if (modelAssembly.contains("location")) {
            modelInfo.location = modelAssembly["location"].toString();
        }
        if (modelAssembly.contains("icon")) {
            modelInfo.icon = modelAssembly["icon"].toString();
        }
        if (modelAssembly.contains("componentList")) {
            QJsonArray compList = modelAssembly["componentList"].toArray();
            modelInfo.componentList.clear();
            for (const auto& compId : compList) {
                modelInfo.componentList << compId.toString();
            }
        }
    }
    
    // 获取位置信息
    QJsonObject position = json["position"].toObject();
    double longitude = position["longitude"].toDouble();
    double latitude = position["latitude"].toDouble();
    double altitude = position["altitude"].toDouble();
    
    // 创建实体
    GeoEntity* entity = entityManager_->createEntity(
        json["type"].toString(),
        modelName,
        QJsonObject(),  // properties会在后面设置
        longitude,
        latitude,
        altitude
    );
    
    if (entity) {
        // 设置规划属性
        if (json.contains("heading")) {
            entity->setHeading(json["heading"].toDouble());
        }
        if (json.contains("visible")) {
            entity->setVisible(json["visible"].toBool());
        }
        if (json.contains("name")) {
            entity->setProperty("displayName", json["name"].toString());
        }
        
        // 保存模型组装和组件配置到entity属性
        entity->setProperty("modelId", modelId);
        entity->setProperty("modelAssembly", QJsonObject(modelInfo));  // 转换为QJsonObject
        if (json.contains("componentConfigs")) {
            entity->setProperty("componentConfigs", json["componentConfigs"].toObject());
        }
    }
    
    return entity;
}
```

---

**文档版本**：1.0  
**最后更新**：2024-01-01

