// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_controller.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTextStream>

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
    controlPath_ = QFileInfo(settingsFile).absoluteFilePath() + ".control";
    connect(mappings_, &MappingModel::edited, this, &ProfileController::stopRuntime);
    heartbeat_.setInterval(100);
    connect(&heartbeat_, &QTimer::timeout, this, &ProfileController::refreshRuntime);
}
ProfileController::~ProfileController() { stopRuntime(); }
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
    stopRuntime();
    draft_ = candidate;
    emit profileChanged();
    emit stateChanged();
}
void ProfileController::newProfile() {
    stopRuntime();
    draft_ = {};
    saved_ = draft_;
    mappings_->replace({});
    path_.clear();
    status_ = "New profile";
    emit profileChanged();
    emit stateChanged();
}
void ProfileController::duplicateProfile() {
    stopRuntime();
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
    stopRuntime();
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

bool ProfileController::runtimeArmed() const { return armed_; }
QString ProfileController::runtimeStatus() const { return runtimeStatus_; }
static QString backendDirectory() {
    const QDir executable(QCoreApplication::applicationDirPath());
    if (executable.exists("libarmswing_openvr.so"))
        return executable.absolutePath();
    return executable.absoluteFilePath("../" ARMSWING_INSTALL_LIBDIR "/arm-swing-vr");
}
static QString shellQuote(QString value) { return "'" + value.replace("'", "'\\''") + "'"; }
QString ProfileController::launchCommand() const {
    const auto prefix = "ARMSWING_CONTROL_FILE=" + shellQuote(controlPath_) + " ";
    if (draft_.runtime == "openvr")
        return prefix + "LD_PRELOAD=" + shellQuote(backendDirectory() + "/libarmswing_openvr.so") +
               "${LD_PRELOAD:+:$LD_PRELOAD} %command%";
    const QDir executable(QCoreApplication::applicationDirPath());
    const QString layers = executable.exists("openxr/armswing.json")
                               ? executable.absoluteFilePath("openxr")
                               : executable.absoluteFilePath("../share/arm-swing-vr/openxr");
    return prefix + "XR_API_LAYER_PATH=" + shellQuote(layers) +
           "${XR_API_LAYER_PATH:+:$XR_API_LAYER_PATH} "
           "XR_ENABLE_API_LAYERS=XR_APILAYER_ARMSWING_locomotion${XR_ENABLE_API_LAYERS:+:$XR_"
           "ENABLE_API_LAYERS} %command%";
}
bool ProfileController::startRuntime() {
    stopRuntime();
    const auto p = snapshot();
    QString error = armswing::validate(p);
    if (error.isEmpty() && p.controller != "knuckles")
        error = "This prototype currently supports Valve Index / Knuckles controllers.";
    armswing::MotionConfig motion;
    const int activation = armswing::buttonIndex(p.activationInput.toStdString());
    if (error.isEmpty() && activation < 0)
        error = "Choose a left/right A or B click for activation. Other input types are not "
                "implemented yet.";
    motion.activation = activation < 0 ? 0 : uint32_t(activation);
    motion.arms = p.contributingArms == "both" ? 2 : p.contributingArms == "right" ? 1 : 0;
    motion.outputHand = p.outputHand == "right";
    motion.steering = p.steering == "head" ? 0 : p.steering == "left-hand" ? 1 : 2;
    motion.sensitivity = float(p.sensitivity);
    for (const auto& mapping : p.mappings) {
        const auto source = armswing::buttonIndex(mapping.source.toStdString());
        const auto destination = armswing::buttonIndex(mapping.destination.toStdString());
        if (source < 0 || destination < 0 || motion.mappingCount >= motion.mappings.size()) {
            error = "Live mappings currently support left/right A and B clicks only.";
            break;
        }
        motion.mappings[motion.mappingCount++] = {uint32_t(source), uint32_t(destination)};
    }
    if (error.isEmpty() && !armswing::validConfig(motion))
        error = "The activation input cannot map back to itself.";
    const QString library = backendDirectory() + (p.runtime == "openvr" ? "/libarmswing_openvr.so"
                                                                        : "/libarmswing_openxr.so");
    if (error.isEmpty() && !QFileInfo::exists(library))
        error = "Build or install the VR backend libraries alongside the application first.";
    if (!error.isEmpty()) {
        emit errorOccurred(error);
        return false;
    }
    QDir().mkpath(QFileInfo(controlPath_).absolutePath());
    runtimeLock_ = std::make_unique<QLockFile>(controlPath_ + ".lock");
    if (!runtimeLock_->tryLock()) {
        runtimeLock_.reset();
        emit errorOccurred(
            "Another editor is controlling VR input. Stop it before enabling this profile.");
        return false;
    }
    control_ = {};
    control_.motion = motion;
    control_.backend =
        p.runtime == "openvr" ? armswing::Backend::OpenVr : armswing::Backend::OpenXr;
    control_.generation = QRandomGenerator::global()->generate64();
    control_.steamAppId = p.steamAppId.toULongLong();
    if (!p.steamAppId.isEmpty() && !control_.steamAppId) {
        runtimeLock_.reset();
        emit errorOccurred("Steam App ID is outside the supported numeric range.");
        return false;
    }
    control_.enabled = 1;
    armed_ = true;
    runtimeStatus_ = "Enabled — waiting for the game backend";
    refreshRuntime();
    if (armed_)
        heartbeat_.start();
    emit runtimeChanged();
    return armed_;
}
void ProfileController::stopRuntime() {
    if (!armed_)
        return;
    heartbeat_.stop();
    armed_ = false;
    QFile::remove(controlPath_);
    QFile::remove(controlPath_ + ".status");
    runtimeLock_.reset();
    runtimeStatus_ = "VR input disabled";
    emit runtimeChanged();
}
void ProfileController::refreshRuntime() {
    if (!armed_)
        return;
    control_.heartbeatNs = armswing::wallTimeNs();
    if (!armswing::writeControl(controlPath_.toStdString(), control_)) {
        stopRuntime();
        emit errorOccurred("Unable to publish the input profile. VR input has been disabled.");
        return;
    }
    QString status = "Enabled — waiting for the game backend";
    QFile file(controlPath_ + ".status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        qint64 time = 0;
        unsigned backend = 0;
        int owns = 0;
        float x = 0, y = 0;
        stream >> time >> backend >> owns >> x >> y;
        const auto now = armswing::wallTimeNs();
        if (stream.status() == QTextStream::Ok && time > 0 && time <= now &&
            now - time < 500000000 && backend == uint32_t(control_.backend))
            status = owns ? QString("Backend connected · movement %1, %2")
                                .arg(x, 0, 'f', 2)
                                .arg(y, 0, 'f', 2)
                          : "Backend connected — waiting for valid tracking and input focus";
    }
    if (status != runtimeStatus_) {
        runtimeStatus_ = status;
        emit runtimeChanged();
    }
}
