// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace armswing {

struct ButtonMapping {
    QString source;
    QString destination;
    bool operator==(const ButtonMapping&) const = default;
};

// Draft profile format. Backends must validate paths and capabilities before activation.
struct Profile {
    QString name = "New profile";
    QString steamAppId;
    QString runtime = "openvr";
    QString controller = "knuckles";
    QString activationInput = "/user/hand/left/input/a/click";
    QString contributingArms = "both";
    QString outputHand = "left";
    QString steering = "head";
    double sensitivity = 1.0;
    QVector<ButtonMapping> mappings;
    bool operator==(const Profile&) const = default;
};

QString validate(const Profile& profile);
QJsonObject toJson(const Profile& profile);
// Failure leaves the caller's profile unchanged and returns a user-facing error.
bool fromJson(const QJsonObject& object, Profile& profile, QString& error);
bool saveProfile(const QString& path, const Profile& profile, QString& error);
bool loadProfile(const QString& path, Profile& profile, QString& error);

} // namespace armswing
