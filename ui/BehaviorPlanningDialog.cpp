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
        for (const auto& option : kFieldOptions) {
            fieldCombo_->addItem(option.label, option.expression);
        }
        layout->addWidget(fieldCombo_, 1);

        opCombo_ = new QComboBox(this);
        opCombo_->addItems({">", "<", ">=", "<=", "==", "!="});
        layout->addWidget(opCombo_);

        valueEdit_ = new QLineEdit(this);
        valueEdit_->setPlaceholderText(QString::fromUtf8(u8"数值/表达式"));
        layout->addWidget(valueEdit_, 1);

        actionCombo_ = new QComboBox(this);
        actionCombo_->addItem(QString::fromUtf8(u8"发射武器"), QStringLiteral("fire_weapon"));
        actionCombo_->addItem(QString::fromUtf8(u8"日志记录"), QStringLiteral("log"));
        actionCombo_->addItem(QString::fromUtf8(u8"无动作"), QStringLiteral("none"));
        layout->addWidget(actionCombo_, 1);

        auto* removeBtn = new QPushButton(QString::fromUtf8(u8"移除"), this);
        removeBtn->setFixedWidth(60);
        layout->addWidget(removeBtn);

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
        QJsonObject cond = obj.value("condition").toObject();
        if (cond.isEmpty()) {
            // 兼容旧字段
            QJsonObject legacy;
            legacy["left"] = obj.value("left");
            legacy["operator"] = obj.value("operator");
            legacy["right"] = obj.value("right");
            cond = legacy;
        }

        QString left = cond.value("left").toString();
        int index = fieldCombo_->findData(left);
        if (index >= 0) {
            fieldCombo_->setCurrentIndex(index);
        } else if (!left.isEmpty()) {
            fieldCombo_->setEditText(left);
        }

        QString op = cond.value("operator").toString();
        int opIndex = opCombo_->findText(op);
        if (opIndex >= 0) {
            opCombo_->setCurrentIndex(opIndex);
        }

        if (cond.contains("right")) {
            valueEdit_->setText(cond.value("right").toVariant().toString());
        }

        QString actionType = obj.value("action").toObject().value("type").toString();
        int actionIndex = actionCombo_->findData(actionType);
        if (actionIndex >= 0) {
            actionCombo_->setCurrentIndex(actionIndex);
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
        QJsonObject condition;
        condition["left"] = left;
        condition["operator"] = op;
        condition["right"] = right;
        rule["condition"] = condition;

        QJsonObject action;
        action["type"] = actionCombo_->currentData().toString();
        rule["action"] = action;
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

signals:
    void changed();

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
        mainLayout->addWidget(conditionGroup);

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
        clear();
        if (!rules.isEmpty()) {
            qDeleteAll(ruleRows_);
            ruleRows_.clear();
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
    resize(960, 600);

    auto* mainLayout = new QHBoxLayout(this);
    entityListWidget_ = new QListWidget(this);
    entityListWidget_->setMinimumWidth(220);
    entityListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(entityListWidget_, 0);

    auto* rightLayout = new QVBoxLayout;
    mainLayout->addLayout(rightLayout, 1);

    auto* modeLayout = new QHBoxLayout;
    ruleModeRadio_ = new QRadioButton(QString::fromUtf8(u8"规则化行为"), this);
    scriptModeRadio_ = new QRadioButton(QString::fromUtf8(u8"自定义脚本"), this);
    modeLayout->addWidget(ruleModeRadio_);
    modeLayout->addWidget(scriptModeRadio_);
    modeLayout->addStretch();
    rightLayout->addLayout(modeLayout);

    modeStack_ = new QStackedWidget(this);
    ruleEditor_ = new RuleEditorWidget(this);
    modeStack_->addWidget(ruleEditor_);

    auto* scriptPage = new QWidget(this);
    auto* scriptLayout = new QVBoxLayout(scriptPage);
    scriptLayout->setContentsMargins(0, 0, 0, 0);
    scriptEdit_ = new QPlainTextEdit(scriptPage);
    scriptEdit_->setPlaceholderText(QString::fromUtf8(u8"在此输入 AFSim 脚本，或点击示例按钮填充模板"));
    scriptLayout->addWidget(scriptEdit_);

    auto* commentLayout = new QHBoxLayout;
    commentLayout->addWidget(new QLabel(QString::fromUtf8(u8"备注"), scriptPage));
    scriptCommentEdit_ = new QLineEdit(scriptPage);
    commentLayout->addWidget(scriptCommentEdit_);
    scriptLayout->addLayout(commentLayout);

    auto* templateButton = new QPushButton(QString::fromUtf8(u8"插入示例"), scriptPage);
    scriptLayout->addWidget(templateButton, 0, Qt::AlignLeft);
    scriptLayout->addStretch();

    modeStack_->addWidget(scriptPage);
    rightLayout->addWidget(modeStack_, 1);

    auto* buttonLayout = new QHBoxLayout;
    clearButton_ = new QPushButton(QString::fromUtf8(u8"清除行为"), this);
    saveButton_ = new QPushButton(QString::fromUtf8(u8"保存"), this);
    auto* closeButton = new QPushButton(QString::fromUtf8(u8"关闭"), this);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(closeButton);
    rightLayout->addLayout(buttonLayout);

    ruleModeRadio_->setChecked(true);
    modeStack_->setCurrentIndex(0);

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
        updateWindowTitle();
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
        updateWindowTitle();
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
    if (!behavior.isEmpty()) {
        entity->setProperty("behavior", behavior);
        if (planFileManager_) {
            planFileManager_->markPlanModified();
        }
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
        updateWindowTitle();
        return;
    }
    if (entity->getType().compare(QStringLiteral("image"), Qt::CaseInsensitive) != 0) {
        resetUi();
        loading_ = false;
        dirty_ = false;
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
    updateWindowTitle();
}

void BehaviorPlanningDialog::applyBehaviorToUi(const QJsonObject& behavior)
{
    loading_ = true;
    QString mode = behavior.value("mode").toString();
    if (mode == "script") {
        scriptModeRadio_->setChecked(true);
        scriptEdit_->setPlainText(behavior.value("script").toString());
        scriptCommentEdit_->setText(behavior.value("comment").toString());
    } else {
        ruleModeRadio_->setChecked(true);
        QJsonArray rules = behavior.value("rules").toArray();
        ruleEditor_->setRules(rules);
    }
    loading_ = false;
}

QJsonObject BehaviorPlanningDialog::collectBehaviorFromUi() const
{
    QJsonObject behavior;
    QString mode = ruleModeRadio_->isChecked() ? QStringLiteral("rule") : QStringLiteral("script");
    behavior["mode"] = mode;
    behavior["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (mode == "rule") {
        QJsonArray rules = ruleEditor_->toRulesJson();
        if (rules.isEmpty()) {
            return QJsonObject();
        }
        for (int i = 0; i < rules.size(); ++i) {
            QJsonObject rule = rules[i].toObject();
            rule["id"] = QStringLiteral("%1%2").arg(kRuleIdPrefix).arg(i + 1);
            rules[i] = rule;
        }
        behavior["rules"] = rules;
        behavior.remove("script");
        behavior.remove("comment");
    } else {
        QString scriptText = scriptEdit_->toPlainText().trimmed();
        QString comment = scriptCommentEdit_->text().trimmed();
        if (scriptText.isEmpty() && comment.isEmpty()) {
            return QJsonObject();
        }
        behavior["rules"] = QJsonArray();
        behavior["script"] = scriptText;
        behavior["comment"] = comment;
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
        title += QString::fromUtf8(u8" *未保存");
    }
    setWindowTitle(title);
}
