#ifndef BEHAVIORPLANNINGDIALOG_H
#define BEHAVIORPLANNINGDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <functional>

class QListWidget;
class QRadioButton;
class QStackedWidget;
class QPushButton;
class QPlainTextEdit;
class QLineEdit;
class GeoEntityManager;
class PlanFileManager;
class GeoEntity;

class RuleEditorWidget;

class BehaviorPlanningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BehaviorPlanningDialog(QWidget* parent = nullptr);

    void setEntityManager(GeoEntityManager* manager);
    void setPlanFileManager(PlanFileManager* manager);

    void refreshEntities(const QString& selectUid = QString());

signals:
    void requestRefreshEntities();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onEntitySelectionChanged(int row);
    void onModeChanged();
    void onSaveClicked();
    void onClearClicked();
    void onScriptInsertTemplate();

private:
    void commitCurrentEntity();
    void loadBehaviorForEntity(GeoEntity* entity);
    void applyBehaviorToUi(const QJsonObject& behavior);
    QJsonObject collectBehaviorFromUi() const;
    void resetUi();
    void updateWindowTitle();

    QListWidget* entityListWidget_;
    QRadioButton* ruleModeRadio_;
    QRadioButton* scriptModeRadio_;
    QStackedWidget* modeStack_;

    RuleEditorWidget* ruleEditor_;

    QPlainTextEdit* scriptEdit_;
    QLineEdit* scriptCommentEdit_;
    QPushButton* saveButton_;
    QPushButton* clearButton_;

    GeoEntityManager* entityManager_;
    PlanFileManager* planFileManager_;
    QString currentEntityUid_;
    bool loading_;
    bool dirty_;
};

#endif // BEHAVIORPLANNINGDIALOG_H
