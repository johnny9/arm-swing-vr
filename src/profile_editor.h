// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <armswing/profile.h>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QTableWidget;

class ProfileEditor : public QWidget {
  public:
    explicit ProfileEditor(QWidget* parent = nullptr);
    armswing::Profile profile() const;
    void setProfile(const armswing::Profile& profile);

  private:
    QLineEdit* name_;
    QLineEdit* appId_;
    QComboBox* runtime_;
    QLineEdit* controller_;
    QLineEdit* activation_;
    QComboBox* arms_;
    QComboBox* output_;
    QComboBox* steering_;
    QDoubleSpinBox* sensitivity_;
    QTableWidget* mappings_;
};
