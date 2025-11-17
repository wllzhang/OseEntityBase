#include "BehaviorPlanningDialog.h"

#include "EntityManagementDialog.h"
#include "../geo/geoentitymanager.h"
#include "../geo/geoentity.h"
#include "../plan/planfilemanager.h"

#include <QListWidget>
#include <QRadioButton>
#include <QStackedWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QDateTime>
#include <QCloseEvent>
#include <QJsonArray>
#include <QGridLayout>
#include <QListWidgetItem>
#include <functional>
#include <QSplitter>
#include <QFontDatabase>

namespace {

struct FieldOption {
    QString label;
    QString expression;
};

static const QList<FieldOption> kFieldOptions = {
    {QString::fromUtf8(u8"当前高度"), QStringLiteral("PLATFORM.Altitude()")},
    {QString::fromUtf8(u8"当前速度"), QStringLiteral("PLATFORM.Speed()")},
    {QString::fromUtf8(u8"当前航向"), QStringLiteral("PLATFORM.Heading()")},
    {QString::fromUtf8(u8"目标距离"), QStringLiteral("PLATFORM.GroundRangeToTarget()")}
};

static const QString kRuleIdPrefix = QStringLiteral("rule-");

} // namespace

class ConditionRowWidget : public QWidget
{
public:
    explicit ConditionRowWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        fieldCombo_ = new QComboBox(this);
        fieldCombo_->setEditable(true);
        fieldCombo_->setMinimumWidth(150);
        for (const auto& option : kFieldOptions) {
            fieldCombo_->addItem(option.label, option.expression);
        }
        layout->addWidget(fieldCombo_, 1);

        opCombo_ = new QComboBox(this);
        opCombo_->addItems({">", "<", ">=", "<=", "==", "!="});
        opCombo_->setFixedWidth(70);
        layout->addWidget(opCombo_);

        valueEdit_ = new QLineEdit(this);
        valueEdit_->setPlaceholderText(QString::fromUtf8(u8"数值/表达式"));
        valueEdit_->setMinimumWidth(150);
        layout->addWidget(valueEdit_, 1);

        actionCombo_ = new QComboBox(this);
        actionCombo_->setEditable(true);
        actionCombo_->addItem(QString::fromUtf8(u8"发射武器"), QStringLiteral("fire_weapon"));
        actionCombo_->addItem(QString::fromUtf8(u8"停止射击"), QStringLiteral("hold_fire"));
        actionCombo_->addItem(QString::fromUtf8(u8"无动作"), QStringLiteral("none"));
        actionCombo_->setMinimumWidth(150);
        layout->addWidget(actionCombo_, 1);

        auto* removeBtn = new QPushButton(QString::fromUtf8(u8"移除"), this);
        removeBtn->setFixedWidth(60);
        layout->addWidget(removeBtn);
        layout->setAlignment(removeBtn, Qt::AlignCenter);

        connect(removeBtn, &QPushButton::clicked, this, [this]() {
            if (removeCallback_) {
                removeCallback_(this);
            }
        });

        auto markChanged = [this]() {
            if (changeCallback_) {
                changeCallback_();
            }
        };
        connect(fieldCombo_, &QComboBox::currentTextChanged, this, markChanged);
        connect(opCombo_, &QComboBox::currentTextChanged, this, markChanged);
        connect(valueEdit_, &QLineEdit::textChanged, this, markChanged);
        connect(actionCombo_, &QComboBox::currentTextChanged, this, markChanged);
    }

    void setRule(const QJsonObject& obj)
    {
        QString left;
        QString op;
        QString right;
        QString actionType;

        if (obj.contains("condition")) {
            QJsonObject cond = obj.value("condition").toObject();
            left = cond.value("left").toString();
            op = cond.value("operator").toString();
            right = cond.value("right").toVariant().toString();
            QJsonValue actionVal = obj.value("action");
            if (actionVal.isObject()) {
                actionType = actionVal.toObject().value("type").toString();
            } else if (actionVal.isString()) {
                actionType = actionVal.toString();
            }
        } else {
            left = obj.value("left").toString();
            op = obj.value("operator").toString();
            right = obj.value("right").toVariant().toString();
            QJsonValue actionVal = obj.value("action");
            if (actionVal.isString()) {
                actionType = actionVal.toString();
            } else if (actionVal.isObject()) {
                actionType = actionVal.toObject().value("type").toString();
            }
        }

        int index = fieldCombo_->findData(left);
        if (index >= 0) {
            fieldCombo_->setCurrentIndex(index);
        } else if (!left.isEmpty()) {
            fieldCombo_->setEditText(left);
        }

        int opIndex = opCombo_->findText(op);
        if (opIndex >= 0) {
            opCombo_->setCurrentIndex(opIndex);
        }

        if (!right.isEmpty()) {
            valueEdit_->setText(right);
        }

        if (!actionType.isEmpty()) {
            int actionIndex = actionCombo_->findData(actionType);
            if (actionIndex >= 0) {
                actionCombo_->setCurrentIndex(actionIndex);
            } else {
                actionCombo_->setEditText(actionType);
            }
        }
    }

    QJsonObject toRuleJson() const
    {
        QString left = fieldCombo_->currentText().trimmed();
        if (left.isEmpty()) {
            return QJsonObject();
        }
        QString expression = fieldCombo_->currentData().toString();
        if (!expression.isEmpty()) {
            left = expression;
        }

        QString op = opCombo_->currentText().trimmed();
        QString right = valueEdit_->text().trimmed();
        if (right.isEmpty()) {
            return QJsonObject();
        }

        QJsonObject rule;
        rule["left"] = left;
        rule["operator"] = op;
        rule["right"] = right;

        QString actionValue = actionCombo_->currentData().toString();
        if (actionValue.isEmpty()) {
            actionValue = actionCombo_->currentText().trimmed();
        }
        if (!actionValue.isEmpty()) {
            rule["action"] = actionValue;
        }
        return rule;
    }

    void setRemoveCallback(const std::function<void(ConditionRowWidget*)>& callback)
    {
        removeCallback_ = callback;
    }

    void setChangeCallback(const std::function<void()>& callback)
    {
        changeCallback_ = callback;
    }

private:
    QComboBox* fieldCombo_;
    QComboBox* opCombo_;
    QLineEdit* valueEdit_;
    QComboBox* actionCombo_;
    std::function<void(ConditionRowWidget*)> removeCallback_;
    std::function<void()> changeCallback_;
};

class RuleEditorWidget : public QWidget
{
public:
    explicit RuleEditorWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(10);

        auto* conditionGroup = new QGroupBox(QString::fromUtf8(u8"规则列表（每行 = 条件 + 动作）"), this);
        auto* conditionGroupLayout = new QVBoxLayout(conditionGroup);
        conditionGroupLayout->setContentsMargins(8, 12, 8, 8);
        conditionGroupLayout->setSpacing(6);
        mainLayout->addWidget(conditionGroup);

        auto* headerRow = new QWidget(conditionGroup);
        auto* headerLayout = new QHBoxLayout(headerRow);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(8);
        auto createHeaderLabel = [](const QString& text, Qt::Alignment alignment = Qt::AlignLeft) {
            QLabel* label = new QLabel(text);
            label->setStyleSheet("font-weight:600; color:#555;");
            label->setAlignment(static_cast<Qt::Alignment>(alignment | Qt::AlignVCenter));
            return label;
        };
        headerLayout->addWidget(createHeaderLabel(QString::fromUtf8(u8"条件字段")), 1);
        headerLayout->addWidget(createHeaderLabel(QString::fromUtf8(u8"比较"), Qt::AlignCenter));
        headerLayout->addWidget(createHeaderLabel(QString::fromUtf8(u8"值")), 1);
        headerLayout->addWidget(createHeaderLabel(QString::fromUtf8(u8"动作")), 1);
        headerLayout->addWidget(createHeaderLabel(QString::fromUtf8(u8"操作"), Qt::AlignCenter));
        conditionGroupLayout->addWidget(headerRow);

        auto* scrollArea = new QScrollArea(conditionGroup);
        scrollArea->setWidgetResizable(true);
        conditionGroupLayout->addWidget(scrollArea);

        auto* scrollWidget = new QWidget(scrollArea);
        conditionsLayout_ = new QVBoxLayout(scrollWidget);
        conditionsLayout_->setContentsMargins(0, 0, 0, 0);
        conditionsLayout_->setSpacing(6);
        scrollWidget->setLayout(conditionsLayout_);
        scrollArea->setWidget(scrollWidget);

        auto* addConditionBtn = new QPushButton(QString::fromUtf8(u8"新增规则"), conditionGroup);
        conditionGroupLayout->addWidget(addConditionBtn, 0, Qt::AlignLeft);

        mainLayout->addStretch();

        addConditionBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(addConditionBtn, &QPushButton::clicked, this, [this]() {
            addRuleRow(QJsonObject());
        });

        addRuleRow(QJsonObject());
    }

    void clear()
    {
        qDeleteAll(ruleRows_);
        ruleRows_.clear();
        addRuleRow(QJsonObject());
        notifyChanged();
    }

    void setRules(const QJsonArray& rules)
    {
        qDeleteAll(ruleRows_);
        ruleRows_.clear();

        if (!rules.isEmpty()) {
            for (const QJsonValue& val : rules) {
                addRuleRow(val.toObject());
            }
        }

        if (ruleRows_.isEmpty()) {
            addRuleRow(QJsonObject());
        }

        notifyChanged();
    }

    QJsonArray toRulesJson() const
    {
        QJsonArray rules;
        for (ConditionRowWidget* row : ruleRows_) {
            QJsonObject rule = row->toRuleJson();
            if (!rule.isEmpty()) {
                rules.append(rule);
            }
        }
        return rules;
    }

    void setChangedCallback(const std::function<void()>& callback)
    {
        changedCallback_ = callback;
    }

private:
    void addRuleRow(const QJsonObject& defaultValue)
    {
        auto* row = new ConditionRowWidget(this);
        if (!defaultValue.isEmpty()) {
            row->setRule(defaultValue);
        }
        row->setRemoveCallback([this](ConditionRowWidget* widget) {
            removeRuleRow(widget);
        });
        row->setChangeCallback([this]() {
            notifyChanged();
        });
        conditionsLayout_->addWidget(row);
        ruleRows_.append(row);
        notifyChanged();
    }

    void removeRuleRow(ConditionRowWidget* row)
    {
        if (!row) {
            return;
        }
        ruleRows_.removeOne(row);
        row->deleteLater();
        if (ruleRows_.isEmpty()) {
            addRuleRow(QJsonObject());
        }
        notifyChanged();
    }

    void notifyChanged()
    {
        if (changedCallback_) {
            changedCallback_();
        }
    }

private:
    QVBoxLayout* conditionsLayout_;
    QList<ConditionRowWidget*> ruleRows_;
    std::function<void()> changedCallback_;
};

BehaviorPlanningDialog::BehaviorPlanningDialog(QWidget* parent)
    : QDialog(parent)
    , entityListWidget_(nullptr)
    , ruleModeRadio_(nullptr)
    , scriptModeRadio_(nullptr)
    , modeStack_(nullptr)
    , ruleEditor_(nullptr)
    , scriptEdit_(nullptr)
    , scriptCommentEdit_(nullptr)
    , saveButton_(nullptr)
    , clearButton_(nullptr)
    , entityManager_(nullptr)
    , planFileManager_(nullptr)
    , loading_(false)
    , dirty_(false)
{
    setWindowTitle(QString::fromUtf8(u8"行为规划"));
    resize(1000, 620);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    mainLayout->addWidget(splitter);

    QWidget* entityPanel = new QWidget(splitter);
    auto* entityPanelLayout = new QVBoxLayout(entityPanel);
    entityPanelLayout->setContentsMargins(0, 0, 0, 0);
    entityPanelLayout->setSpacing(8);

    auto* entityLabel = new QLabel(QString::fromUtf8(u8"可配置实体"), entityPanel);
    entityLabel->setStyleSheet("font-weight:600; color:#444;" );
    entityPanelLayout->addWidget(entityLabel);

    entityListWidget_ = new QListWidget(entityPanel);
    entityListWidget_->setMinimumWidth(240);
    entityListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    entityListWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    entityListWidget_->setAlternatingRowColors(true);
    entityListWidget_->setUniformItemSizes(true);
    entityListWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    entityPanelLayout->addWidget(entityListWidget_, 1);

    splitter->addWidget(entityPanel);

    QWidget* editorPanel = new QWidget(splitter);
    auto* editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(10);

    entityInfoLabel_ = new QLabel(QString::fromUtf8(u8"当前实体：--"), editorPanel);
    entityInfoLabel_->setStyleSheet("font-weight:600; color:#333;" );
    entityInfoLabel_->setWordWrap(true);
    editorLayout->addWidget(entityInfoLabel_);

    auto* modeGroup = new QGroupBox(QString::fromUtf8(u8"行为定义模式"), editorPanel);
    auto* modeLayout = new QHBoxLayout(modeGroup);
    modeLayout->setContentsMargins(12, 8, 12, 8);
    modeLayout->setSpacing(16);
    ruleModeRadio_ = new QRadioButton(QString::fromUtf8(u8"规则化行为"), modeGroup);
    scriptModeRadio_ = new QRadioButton(QString::fromUtf8(u8"自定义脚本"), modeGroup);
    ruleModeRadio_->setChecked(true);
    modeLayout->addWidget(ruleModeRadio_);
    modeLayout->addWidget(scriptModeRadio_);
    modeLayout->addStretch();
    editorLayout->addWidget(modeGroup);

    modeStack_ = new QStackedWidget(editorPanel);
    ruleEditor_ = new RuleEditorWidget(modeStack_);
    modeStack_->addWidget(ruleEditor_);

    auto* scriptPage = new QWidget(modeStack_);
    auto* scriptLayout = new QVBoxLayout(scriptPage);
    scriptLayout->setContentsMargins(0, 0, 0, 0);
    scriptLayout->setSpacing(8);

    auto* scriptHint = new QLabel(QString::fromUtf8(u8"在下方编辑 AFSim 脚本，或点击“插入示例”快速填充模板。"), scriptPage);
    scriptHint->setWordWrap(true);
    scriptHint->setStyleSheet("color:#666;" );
    scriptLayout->addWidget(scriptHint);

    scriptEdit_ = new QPlainTextEdit(scriptPage);
    scriptEdit_->setPlaceholderText(QString::fromUtf8(u8"在此输入 AFSim 脚本内容"));
    scriptEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    scriptEdit_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
#ifdef QT_VERSION
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    scriptEdit_->setTabStopDistance(4 * scriptEdit_->fontMetrics().horizontalAdvance(QLatin1Char(' ')));
#endif
#endif
    scriptLayout->addWidget(scriptEdit_, 1);

    auto* commentLayout = new QHBoxLayout;
    commentLayout->setContentsMargins(0, 0, 0, 0);
    commentLayout->setSpacing(8);
    commentLayout->addWidget(new QLabel(QString::fromUtf8(u8"备注"), scriptPage));
    scriptCommentEdit_ = new QLineEdit(scriptPage);
    commentLayout->addWidget(scriptCommentEdit_);
    scriptLayout->addLayout(commentLayout);

    auto* templateButton = new QPushButton(QString::fromUtf8(u8"插入示例"), scriptPage);
    templateButton->setFixedWidth(100);
    scriptLayout->addWidget(templateButton, 0, Qt::AlignLeft);
    scriptLayout->addStretch();

    modeStack_->addWidget(scriptPage);
    editorLayout->addWidget(modeStack_, 1);

    auto* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);
    clearButton_ = new QPushButton(QString::fromUtf8(u8"清除行为"), editorPanel);
    saveButton_ = new QPushButton(QString::fromUtf8(u8"应用"), editorPanel);
    auto* closeButton = new QPushButton(QString::fromUtf8(u8"关闭"), editorPanel);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(closeButton);
    editorLayout->addLayout(buttonLayout);

    splitter->addWidget(editorPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    connect(entityListWidget_, &QListWidget::currentRowChanged, this, &BehaviorPlanningDialog::onEntitySelectionChanged);
    connect(ruleModeRadio_, &QRadioButton::toggled, this, &BehaviorPlanningDialog::onModeChanged);
    connect(scriptModeRadio_, &QRadioButton::toggled, this, &BehaviorPlanningDialog::onModeChanged);
    connect(saveButton_, &QPushButton::clicked, this, &BehaviorPlanningDialog::onSaveClicked);
    connect(clearButton_, &QPushButton::clicked, this, &BehaviorPlanningDialog::onClearClicked);
    connect(closeButton, &QPushButton::clicked, this, &BehaviorPlanningDialog::close);
    connect(templateButton, &QPushButton::clicked, this, &BehaviorPlanningDialog::onScriptInsertTemplate);

    auto markDirty = [this]() {
        if (!loading_) {
            dirty_ = true;
            updateWindowTitle();
        }
    };
    connect(ruleModeRadio_, &QRadioButton::toggled, this, markDirty);
    connect(scriptModeRadio_, &QRadioButton::toggled, this, markDirty);
    connect(scriptEdit_, &QPlainTextEdit::textChanged, this, markDirty);
    connect(scriptCommentEdit_, &QLineEdit::textChanged, this, markDirty);
    ruleEditor_->setChangedCallback(markDirty);
    updateWindowTitle();
}

void BehaviorPlanningDialog::setEntityManager(GeoEntityManager* manager)
{
    entityManager_ = manager;
}

void BehaviorPlanningDialog::setPlanFileManager(PlanFileManager* manager)
{
    planFileManager_ = manager;
}

void BehaviorPlanningDialog::refreshEntities(const QString& selectUid)
{
    if (!entityManager_) {
        entityListWidget_->clear();
        currentEntityUid_.clear();
        resetUi();
        updateEntityInfoLabel(nullptr);
        return;
    }

    QString previousUid = selectUid.isEmpty() ? currentEntityUid_ : selectUid;
    loading_ = true;

    entityListWidget_->clear();
    QList<GeoEntity*> entities = entityManager_->getAllEntities();
    int rowToSelect = -1;
    for (int i = 0; i < entities.size(); ++i) {
        GeoEntity* entity = entities.at(i);
        if (!entity) {
            continue;
        }
        if (entity->getType().compare(QStringLiteral("image"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        QString uid = entity->getUid();
        QString label = QStringLiteral("%1 (%2)").arg(entity->getName(), uid);
        auto* item = new QListWidgetItem(label, entityListWidget_);
        item->setData(Qt::UserRole, uid);
        if (!previousUid.isEmpty() && uid == previousUid) {
            rowToSelect = entityListWidget_->count() - 1;
        }
    }
    loading_ = false;

    if (entityListWidget_->count() == 0) {
        currentEntityUid_.clear();
        resetUi();
        updateEntityInfoLabel(nullptr);
        return;
    }

    if (rowToSelect < 0) {
        rowToSelect = 0;
    }
    entityListWidget_->setCurrentRow(rowToSelect);
}

void BehaviorPlanningDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void BehaviorPlanningDialog::onEntitySelectionChanged(int row)
{
    if (loading_) {
        return;
    }

    QString newUid;
    if (row >= 0) {
        QListWidgetItem* item = entityListWidget_->item(row);
        if (item) {
            newUid = item->data(Qt::UserRole).toString();
        }
    }

    if (newUid == currentEntityUid_) {
        return;
    }

    if (!loading_ && !currentEntityUid_.isEmpty() && dirty_) {
        loading_ = true;
        int currentRow = -1;
        for (int i = 0; i < entityListWidget_->count(); ++i) {
            QListWidgetItem* item = entityListWidget_->item(i);
            if (item && item->data(Qt::UserRole).toString() == currentEntityUid_) {
                currentRow = i;
                break;
            }
        }
        if (currentRow >= 0) {
            entityListWidget_->setCurrentRow(currentRow);
        }
        loading_ = false;
        updateWindowTitle();
        return;
    }

    currentEntityUid_ = newUid;

    GeoEntity* entity = nullptr;
    if (!currentEntityUid_.isEmpty() && entityManager_) {
        entity = entityManager_->getEntityByUid(currentEntityUid_);
    }
    loadBehaviorForEntity(entity);
    dirty_ = false;
    updateEntityInfoLabel(entity);
    updateWindowTitle();
}

void BehaviorPlanningDialog::onModeChanged()
{
    if (loading_) {
        return;
    }
    if (ruleModeRadio_->isChecked()) {
        modeStack_->setCurrentIndex(0);
    } else {
        modeStack_->setCurrentIndex(1);
    }
}

void BehaviorPlanningDialog::onSaveClicked()
{
    commitCurrentEntity();
    dirty_ = false;
    updateWindowTitle();
}

void BehaviorPlanningDialog::onClearClicked()
{
    ruleEditor_->clear();
    scriptEdit_->clear();
    scriptCommentEdit_->clear();
    dirty_ = true;
    if (!currentEntityUid_.isEmpty() && entityManager_) {
        if (GeoEntity* entity = entityManager_->getEntityByUid(currentEntityUid_)) {
            entity->setProperty("behavior", QJsonObject());
            // 只标记为已修改，不直接保存文件。真正的保存应该通过"保存方案"按钮
            if (planFileManager_) {
                planFileManager_->markPlanModified();
            }
        }
    }
    updateWindowTitle();
}

void BehaviorPlanningDialog::onScriptInsertTemplate()
{
    static const QString templateText = QStringLiteral(
        "script bool fireEnemy(WsfTrack tTrack, string wpName)\n"
        "{\n"
        "    bool isSuccess = false;\n"
        "    WsfWeapon weapon = PLATFORM.Weapon(wpName);\n"
        "    if (weapon.IsValid() && weapon.QuantityRemaining() > 0) {\n"
        "        if (PLATFORM.Altitude() > 9000 &&\n"
        "            weapon.AuxDataDouble(\"strikeDistance\") > PLATFORM.GroundRangeTo(tTrack)) {\n"
        "            isSuccess = weapon.FireSalvo(tTrack, 1);\n"
        "            writeln(\"T = \", TIME_NOW, \" , \", PLATFORM.Name(), \" launches \", wpName, \" result = \", isSuccess);\n"
        "        }\n"
        "    }\n"
        "    return isSuccess;\n"
        "}\n"
        "end_script\n");
    if (!scriptEdit_->toPlainText().trimmed().isEmpty()) {
        auto reply = QMessageBox::question(this, QString::fromUtf8(u8"替换示例"),
                                           QString::fromUtf8(u8"是否替换当前脚本为示例模板？"));
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    scriptEdit_->setPlainText(templateText);
}

void BehaviorPlanningDialog::commitCurrentEntity()
{
    if (loading_ || currentEntityUid_.isEmpty() || !entityManager_) {
        return;
    }

    GeoEntity* entity = entityManager_->getEntityByUid(currentEntityUid_);
    if (!entity) {
        return;
    }
    if (entity->getType().compare(QStringLiteral("image"), Qt::CaseInsensitive) != 0) {
        return;
    }

    QJsonObject behavior = collectBehaviorFromUi();
    if (behavior.isEmpty()) {
        entity->setProperty("behavior", QJsonObject());
    } else {
        entity->setProperty("behavior", behavior);
    }
    // 只标记为已修改，不直接保存文件。真正的保存应该通过"保存方案"按钮
    if (planFileManager_) {
        planFileManager_->markPlanModified();
    }
    dirty_ = false;
    updateWindowTitle();
}

void BehaviorPlanningDialog::loadBehaviorForEntity(GeoEntity* entity)
{
    loading_ = true;
    if (!entity) {
        resetUi();
        loading_ = false;
        dirty_ = false;
        updateEntityInfoLabel(nullptr);
        updateWindowTitle();
        return;
    }
    if (entity->getType().compare(QStringLiteral("image"), Qt::CaseInsensitive) != 0) {
        resetUi();
        loading_ = false;
        dirty_ = false;
        updateEntityInfoLabel(nullptr);
        updateWindowTitle();
        return;
    }

    QVariant behaviorVar = entity->getProperty("behavior");
    QJsonObject behaviorObj;
    if (behaviorVar.canConvert<QJsonObject>()) {
        behaviorObj = behaviorVar.toJsonObject();
    } else if (behaviorVar.canConvert<QVariantMap>()) {
        behaviorObj = QJsonObject::fromVariantMap(behaviorVar.toMap());
    }

    applyBehaviorToUi(behaviorObj);
    loading_ = false;
    onModeChanged();
    dirty_ = false;
    updateEntityInfoLabel(entity);
    updateWindowTitle();
}

void BehaviorPlanningDialog::applyBehaviorToUi(const QJsonObject& behavior)
{
    loading_ = true;
    QJsonArray rules = behavior.value("rules").toArray();
    ruleEditor_->setRules(rules);

    scriptEdit_->setPlainText(behavior.value("script").toString());
    scriptCommentEdit_->setText(behavior.value("comment").toString());

    bool hasRules = !rules.isEmpty();
    bool hasScript = !scriptEdit_->toPlainText().trimmed().isEmpty();

    QString mode = behavior.value("mode").toString();
    if (mode == "script") {
        scriptModeRadio_->setChecked(true);
    } else if (mode == "rule") {
        ruleModeRadio_->setChecked(true);
    } else {
        if (hasRules) {
            ruleModeRadio_->setChecked(true);
        } else if (hasScript) {
            scriptModeRadio_->setChecked(true);
        } else {
            ruleModeRadio_->setChecked(true);
        }
    }
    loading_ = false;
}

QJsonObject BehaviorPlanningDialog::collectBehaviorFromUi() const
{
    QJsonObject behavior;
    behavior["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray rules = ruleEditor_->toRulesJson();
    if (!rules.isEmpty()) {
        for (int i = 0; i < rules.size(); ++i) {
            QJsonObject rule = rules[i].toObject();
            rule["id"] = QStringLiteral("%1%2").arg(kRuleIdPrefix).arg(i + 1);
            rules[i] = rule;
        }
        behavior["rules"] = rules;
    }

    QString scriptText = scriptEdit_->toPlainText().trimmed();
    QString comment = scriptCommentEdit_->text().trimmed();
    if (!scriptText.isEmpty()) {
        behavior["script"] = scriptText;
    }
    if (!comment.isEmpty()) {
        behavior["comment"] = comment;
    }

    if (!behavior.contains("rules") && !behavior.contains("script") && !behavior.contains("comment")) {
        return QJsonObject();
    }
    return behavior;
}

void BehaviorPlanningDialog::resetUi()
{
    bool previousLoading = loading_;
    loading_ = true;
    ruleModeRadio_->setChecked(true);
    ruleEditor_->clear();
    scriptEdit_->clear();
    scriptCommentEdit_->clear();
    modeStack_->setCurrentIndex(0);
    loading_ = previousLoading;
    dirty_ = false;
    updateEntityInfoLabel(nullptr);
    updateWindowTitle();
}

void BehaviorPlanningDialog::updateWindowTitle()
{
    QString title = QString::fromUtf8(u8"行为规划");
    if (!currentEntityUid_.isEmpty() && entityManager_) {
        if (GeoEntity* entity = entityManager_->getEntityByUid(currentEntityUid_)) {
            title += QString::fromUtf8(u8" - %1").arg(entity->getName());
        }
    }
    if (dirty_) {
        title += QString::fromUtf8(u8" *未应用");
    }
    setWindowTitle(title);
}

void BehaviorPlanningDialog::updateEntityInfoLabel(GeoEntity* entity)
{
    if (!entityInfoLabel_) {
        return;
    }
    if (entity) {
        QString typeText = entity->getType();
        entityInfoLabel_->setText(QString::fromUtf8(u8"当前实体：%1  (类型：%2)")
                                      .arg(entity->getName())
                                      .arg(typeText));
    } else {
        entityInfoLabel_->setText(QString::fromUtf8(u8"当前实体：--"));
    }
}
