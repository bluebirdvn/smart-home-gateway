#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QList>

struct LogItem {
    QString timestamp;
    QString direction;
    QString tag;
    QString message;
};

class LogModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum LogRoles {
        TimeRole = Qt::UserRole + 1,
        DirectionRole,
        TagRole,
        MessageRole
    };

    explicit LogModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_logs.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_logs.size())
            return QVariant();
        const auto &item = m_logs.at(index.row());
        switch (role) {
        case TimeRole: return item.timestamp;
        case DirectionRole: return item.direction;
        case TagRole: return item.tag;
        case MessageRole: return item.message;
        default: return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {TimeRole, "logTime"},
            {DirectionRole, "logDir"},
            {TagRole, "logTag"},
            {MessageRole, "logMsg"}
        };
    }

    Q_INVOKABLE void appendLog(const QString &direction, const QString &tag, const QString &msg) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_logs.prepend({QDateTime::currentDateTime().toString("HH:mm:ss.zzz"), direction, tag, msg});
        endInsertRows();

        if (m_logs.size() > 500) {
            beginRemoveRows(QModelIndex(), 500, m_logs.size() - 1);
            while (m_logs.size() > 500) m_logs.removeLast();
            endRemoveRows();
        }
    }

private:
    QList<LogItem> m_logs;
};
