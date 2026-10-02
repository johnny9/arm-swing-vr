// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_controller.h"
#include <QFileInfo>

MappingModel::MappingModel(QObject* parent) : QAbstractListModel(parent) {}
int MappingModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}
QVariant MappingModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= entries_.size())
        return {};
    const auto& entry = entries_[index.row()];
    if (role == SourceRole)
        return entry.source;
    if (role == DestinationRole)
        return entry.destination;
    return {};
}
QHash<int, QByteArray> MappingModel::roleNames() const {
    return {{SourceRole, "sourceInput"}, {DestinationRole, "destinationInput"}};
}
void MappingModel::add() {
    const int row = rowCount();
    beginInsertRows({}, row, row);
    entries_.append(armswing::ButtonMapping{});
    endInsertRows();
    emit edited();
}
void MappingModel::remove(int row) {
    if (row < 0 || row >= rowCount())
        return;
    beginRemoveRows({}, row, row);
    entries_.removeAt(row);
    endRemoveRows();
    emit edited();
}
void MappingModel::setInput(int row, bool source, const QString& value) {
    if (row < 0 || row >= rowCount())
        return;
    auto& field = source ? entries_[row].source : entries_[row].destination;
    if (field == value)
        return;
    field = value;
    emit dataChanged(index(row), index(row), {source ? SourceRole : DestinationRole});
    emit edited();
}
const QVector<armswing::ButtonMapping>& MappingModel::entries() const { return entries_; }
void MappingModel::replace(const QVector<armswing::ButtonMapping>& entries) {
    beginResetModel();
    entries_ = entries;
    endResetModel();
}
ProfileController::ProfileController(const QString& settingsFile, QObject* parent)
    : QObject(parent), mappings_(new MappingModel(this)),
      settings_(settingsFile, QSettings::IniFormat) {
    recent_ = settings_.value("recentProfiles").toStringList();
    darkMode_ = settings_.value("darkMode", true).toBool();
    connect(mappings_, &MappingModel::edited, this, &ProfileController::stateChanged);
}
armswing::Profile ProfileController::snapshot() const {
    auto result = draft_;
    result.mappings = mappings_->entries();
    return result;
}
QVariantMap ProfileController::profile() const {
    auto result = armswing::toJson(draft_).toVariantMap();
    result.remove("mappings");
    return result;
}
MappingModel* ProfileController::mappings() { return mappings_; }
bool ProfileController::dirty() const { return snapshot() != saved_; }
QUrl ProfileController::fileUrl() const {
    return path_.isEmpty() ? QUrl{} : QUrl::fromLocalFile(path_);
}
QString ProfileController::status() const { return status_; }
bool ProfileController::darkMode() const { return darkMode_; }
void ProfileController::setDarkMode(bool dark) {
    if (dark == darkMode_)
        return;
    darkMode_ = dark;
    settings_.setValue("darkMode", dark);
    emit themeChanged();
}
QVariantList ProfileController::recentProfiles() const {
    QVariantList result;
    for (const auto& path : recent_) {
        if (QFileInfo::exists(path))
            result.append(QVariantMap{{"name", QFileInfo(path).completeBaseName()},
                                      {"url", QUrl::fromLocalFile(path)}});
    }
    return result;
}
void ProfileController::setField(const QString& field, const QVariant& value) {
    auto candidate = draft_;
    if (field == "name")
        candidate.name = value.toString();
    else if (field == "steam_app_id")
        candidate.steamAppId = value.toString();
    else if (field == "runtime")
        candidate.runtime = value.toString();
    else if (field == "controller")
        candidate.controller = value.toString();
    else if (field == "activation_input")
        candidate.activationInput = value.toString();
    else if (field == "contributing_arms")
        candidate.contributingArms = value.toString();
    else if (field == "output_hand")
        candidate.outputHand = value.toString();
    else if (field == "steering")
        candidate.steering = value.toString();
    else if (field == "sensitivity")
        candidate.sensitivity = value.toDouble();
    else
        return;
    if (candidate == draft_)
        return;
    draft_ = candidate;
    emit profileChanged();
    emit stateChanged();
}
void ProfileController::newProfile() {
    draft_ = {};
    saved_ = draft_;
    mappings_->replace({});
    path_.clear();
    status_ = "New profile";
    emit profileChanged();
    emit stateChanged();
}
void ProfileController::duplicateProfile() {
    draft_.name += " (copy)";
    path_.clear();
    status_ = "Profile duplicated — save to a new file";
    emit profileChanged();
    emit stateChanged();
}
void ProfileController::remember(const QString& path) {
    recent_.removeAll(path);
    recent_.prepend(path);
    while (recent_.size() > 10)
        recent_.removeLast();
    settings_.setValue("recentProfiles", recent_);
}
bool ProfileController::load(const QUrl& url) {
    if (!url.isLocalFile()) {
        emit errorOccurred("Choose a local profile file.");
        return false;
    }
    armswing::Profile candidate;
    QString error;
    if (!armswing::loadProfile(url.toLocalFile(), candidate, error)) {
        emit errorOccurred(error);
        return false;
    }
    draft_ = candidate;
    saved_ = candidate;
    mappings_->replace(candidate.mappings);
    path_ = QFileInfo(url.toLocalFile()).absoluteFilePath();
    status_ = "Profile opened";
    remember(path_);
    emit profileChanged();
    emit stateChanged();
    return true;
}
bool ProfileController::save(const QUrl& url) {
    if (!url.isLocalFile()) {
        emit errorOccurred("Choose a local destination for the profile.");
        return false;
    }
    QString error;
    const auto candidate = snapshot();
    if (!armswing::saveProfile(url.toLocalFile(), candidate, error)) {
        emit errorOccurred(error);
        return false;
    }
    saved_ = candidate;
    path_ = QFileInfo(url.toLocalFile()).absoluteFilePath();
    status_ = "All changes saved";
    remember(path_);
    emit stateChanged();
    return true;
}
