// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QAbstractListModel>
#include <QSettings>
#include <QUrl>
#include <QVariantMap>
#include <armswing/profile.h>

class MappingModel : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Role { SourceRole = Qt::UserRole + 1, DestinationRole };
    explicit MappingModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void setInput(int row, bool source, const QString& value);
    const QVector<armswing::ButtonMapping>& entries() const;
    void replace(const QVector<armswing::ButtonMapping>& entries);
  signals:
    void edited();

  private:
    QVector<armswing::ButtonMapping> entries_;
};

class ProfileController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(MappingModel* mappings READ mappings CONSTANT)
    Q_PROPERTY(bool dirty READ dirty NOTIFY stateChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QVariantList recentProfiles READ recentProfiles NOTIFY stateChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY themeChanged)
  public:
    explicit ProfileController(const QString& settingsFile, QObject* parent = nullptr);
    QVariantMap profile() const;
    MappingModel* mappings();
    bool dirty() const;
    QUrl fileUrl() const;
    QString status() const;
    QVariantList recentProfiles() const;
    bool darkMode() const;
    void setDarkMode(bool dark);
    Q_INVOKABLE void setField(const QString& field, const QVariant& value);
    Q_INVOKABLE void newProfile();
    Q_INVOKABLE void duplicateProfile();
    Q_INVOKABLE bool load(const QUrl& url);
    Q_INVOKABLE bool save(const QUrl& url);
  signals:
    void profileChanged();
    void stateChanged();
    void themeChanged();
    void errorOccurred(const QString& message);

  private:
    armswing::Profile snapshot() const;
    void remember(const QString& path);
    armswing::Profile draft_;
    armswing::Profile saved_;
    MappingModel* mappings_;
    QSettings settings_;
    QString path_;
    QString status_ = "New profile";
    QStringList recent_;
    bool darkMode_ = true;
};
