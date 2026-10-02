// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <armswing/profile.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <cmath>

namespace armswing {

QString validate(const Profile& profile) {
    if (profile.name.trimmed().isEmpty())
        return "Give the profile a name.";
    const QRegularExpression appId("^[1-9][0-9]*$");
    if (!profile.steamAppId.isEmpty() && !appId.match(profile.steamAppId).hasMatch())
        return "Steam App ID must be a positive integer, or blank for a non-Steam game.";
    if (profile.runtime != "openvr" && profile.runtime != "openxr")
        return "Runtime must be openvr or openxr.";
    if (profile.controller.trimmed().isEmpty() || profile.activationInput.trimmed().isEmpty())
        return "Controller and activation input must be specified.";
    if (!QStringList{"left", "right", "both"}.contains(profile.contributingArms))
        return "Contributing arms must be left, right, or both.";
    if (profile.outputHand != "left" && profile.outputHand != "right")
        return "Output hand must be left or right.";
    if (!QStringList{"head", "left-hand", "right-hand"}.contains(profile.steering))
        return "Choose a supported steering reference.";
    if (!std::isfinite(profile.sensitivity) || profile.sensitivity < 0.1 ||
        profile.sensitivity > 5.0)
        return "Sensitivity must be between 0.1 and 5.0.";
    if (std::abs(profile.sensitivity * 100.0 - std::round(profile.sensitivity * 100.0)) > 1e-9)
        return "Sensitivity supports up to two decimal places.";
    QSet<QString> sources;
    for (const auto& mapping : profile.mappings) {
        if (mapping.source.trimmed().isEmpty() || mapping.destination.trimmed().isEmpty())
            return "Each mapping needs a source and destination.";
        if (sources.contains(mapping.source))
            return "A source input can have only one mapping.";
        sources.insert(mapping.source);
    }
    return {};
}

QJsonObject toJson(const Profile& profile) {
    QJsonArray mappings;
    for (const auto& mapping : profile.mappings)
        mappings.append(
            QJsonObject{{"source", mapping.source}, {"destination", mapping.destination}});
    return {{"schema_version", 1},
            {"name", profile.name},
            {"steam_app_id", profile.steamAppId},
            {"runtime", profile.runtime},
            {"controller", profile.controller},
            {"activation_input", profile.activationInput},
            {"contributing_arms", profile.contributingArms},
            {"output_hand", profile.outputHand},
            {"steering", profile.steering},
            {"sensitivity", profile.sensitivity},
            {"mappings", mappings}};
}

bool fromJson(const QJsonObject& object, Profile& profile, QString& error) {
    if (!object.value("schema_version").isDouble() ||
        object.value("schema_version").toDouble() != 1) {
        error = "Unsupported or missing profile schema version.";
        return false;
    }
    const QStringList fields{"name",        "steam_app_id",     "runtime",
                             "controller",  "activation_input", "contributing_arms",
                             "output_hand", "steering"};
    for (const auto& key : fields) {
        if (!object.value(key).isString()) {
            error = "Missing or invalid text field: " + key;
            return false;
        }
    }
    // Reject unfamiliar fields rather than silently dropping future settings on save.
    const QSet<QString> supported{"schema_version",    "name",        "steam_app_id",
                                  "runtime",           "controller",  "activation_input",
                                  "contributing_arms", "output_hand", "steering",
                                  "sensitivity",       "mappings"};
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!supported.contains(it.key())) {
            error = "Unsupported profile field: " + it.key();
            return false;
        }
    }
    if (!object.value("sensitivity").isDouble() || !object.value("mappings").isArray()) {
        error = "Sensitivity must be a number and mappings must be an array.";
        return false;
    }
    Profile candidate;
    candidate.name = object.value("name").toString();
    candidate.steamAppId = object.value("steam_app_id").toString();
    candidate.runtime = object.value("runtime").toString();
    candidate.controller = object.value("controller").toString();
    candidate.activationInput = object.value("activation_input").toString();
    candidate.contributingArms = object.value("contributing_arms").toString();
    candidate.outputHand = object.value("output_hand").toString();
    candidate.steering = object.value("steering").toString();
    candidate.sensitivity = object.value("sensitivity").toDouble();
    for (const auto& value : object.value("mappings").toArray()) {
        const auto mapping = value.toObject();
        if (!value.isObject() || mapping.size() != 2 || !mapping.value("source").isString() ||
            !mapping.value("destination").isString()) {
            error = "Each mapping must contain only source and destination text fields.";
            return false;
        }
        candidate.mappings.append(
            {mapping.value("source").toString(), mapping.value("destination").toString()});
    }
    error = validate(candidate);
    if (!error.isEmpty())
        return false;
    profile = candidate;
    return true;
}

bool saveProfile(const QString& path, const Profile& profile, QString& error) {
    error = validate(profile);
    if (!error.isEmpty())
        return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    const auto bytes = QJsonDocument(toJson(profile)).toJson();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}

bool loadProfile(const QString& path, Profile& profile, QString& error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    constexpr qint64 maxProfileBytes = 1024 * 1024;
    const auto bytes = file.read(maxProfileBytes + 1);
    if (file.error() != QFileDevice::NoError || bytes.size() > maxProfileBytes) {
        error = "Unable to read profile or profile exceeds the 1 MiB size limit.";
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = "Invalid profile JSON: " + parseError.errorString();
        return false;
    }
    return fromJson(document.object(), profile, error);
}

} // namespace armswing
