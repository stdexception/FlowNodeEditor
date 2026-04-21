#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

class FlowEditorScene;

class FlowEditorSceneHistory : public QObject
{
    Q_OBJECT

public:
    explicit FlowEditorSceneHistory(FlowEditorScene *scene, QObject *parent = nullptr);

    void clear();
    void storeInitialHistoryStamp();

    void storeHistory(const QString &desc, bool setModified = false);

    bool canUndo() const;
    bool canRedo() const;

    void undo();
    void redo();

    bool isRestoring() const
    {
        return m_restoring;
    }

signals:
    void historyModified();
    void historyRestored();

private:
    struct Stamp
    {
        QString desc;
        QJsonObject snapshot;
        QJsonObject selection;
    };

    QJsonObject captureSelection() const;
    Stamp createStamp(const QString &desc);
    void restoreStamp(const Stamp &stamp);

    FlowEditorScene *m_scene = nullptr;
    QVector<Stamp> m_stack;
    int m_currentStep = -1;
    static constexpr int kHistoryLimit = 32;
    bool m_restoring = false;
};
