// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_editor.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ProfileEditor::ProfileEditor(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    name_ = new QLineEdit(this);
    appId_ = new QLineEdit(this);
    appId_->setPlaceholderText("Optional");
    runtime_ = new QComboBox(this);
    runtime_->addItem("SteamVR / OpenVR", "openvr");
    runtime_->addItem("OpenXR", "openxr");
    controller_ = new QLineEdit(this);
    activation_ = new QLineEdit(this);
    arms_ = new QComboBox(this);
    arms_->addItem("Both arms", "both");
    arms_->addItem("Left arm", "left");
    arms_->addItem("Right arm", "right");
    output_ = new QComboBox(this);
    output_->addItem("Left controller", "left");
    output_->addItem("Right controller", "right");
    steering_ = new QComboBox(this);
    steering_->addItem("Head", "head");
    steering_->addItem("Left hand", "left-hand");
    steering_->addItem("Right hand", "right-hand");
    sensitivity_ = new QDoubleSpinBox(this);
    sensitivity_->setRange(0.1, 5.0);
    sensitivity_->setSingleStep(0.1);
    sensitivity_->setDecimals(2);
    form->addRow("Profile name", name_);
    form->addRow("Steam App ID", appId_);
    form->addRow("Runtime", runtime_);
    form->addRow("Controller profile", controller_);
    form->addRow("Hold to activate", activation_);
    form->addRow("Measure swings from", arms_);
    form->addRow("Send movement to", output_);
    form->addRow("Steering reference", steering_);
    form->addRow("Sensitivity", sensitivity_);
    layout->addLayout(form);
    layout->addWidget(new QLabel("Controller mappings", this));
    mappings_ = new QTableWidget(0, 2, this);
    mappings_->setHorizontalHeaderLabels({"Source input", "Destination input"});
    mappings_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mappings_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mappings_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(mappings_);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton("Add mapping", this);
    auto* remove = new QPushButton("Remove selected", this);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    layout->addLayout(buttons);
    connect(add, &QPushButton::clicked, this, [this] {
        const int row = mappings_->rowCount();
        mappings_->insertRow(row);
        mappings_->setItem(row, 0, new QTableWidgetItem);
        mappings_->setItem(row, 1, new QTableWidgetItem);
        mappings_->setCurrentCell(row, 0);
        mappings_->editItem(mappings_->item(row, 0));
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (mappings_->currentRow() >= 0)
            mappings_->removeRow(mappings_->currentRow());
    });
    setProfile({});
}

armswing::Profile ProfileEditor::profile() const {
    armswing::Profile result;
    result.name = name_->text().trimmed();
    result.steamAppId = appId_->text().trimmed();
    result.runtime = runtime_->currentData().toString();
    result.controller = controller_->text().trimmed();
    result.activationInput = activation_->text().trimmed();
    result.contributingArms = arms_->currentData().toString();
    result.outputHand = output_->currentData().toString();
    result.steering = steering_->currentData().toString();
    result.sensitivity = sensitivity_->value();
    for (int row = 0; row < mappings_->rowCount(); ++row) {
        const auto* source = mappings_->item(row, 0);
        const auto* destination = mappings_->item(row, 1);
        result.mappings.append({source ? source->text().trimmed() : QString{},
                                destination ? destination->text().trimmed() : QString{}});
    }
    return result;
}

void ProfileEditor::setProfile(const armswing::Profile& profile) {
    name_->setText(profile.name);
    appId_->setText(profile.steamAppId);
    runtime_->setCurrentIndex(runtime_->findData(profile.runtime));
    controller_->setText(profile.controller);
    activation_->setText(profile.activationInput);
    arms_->setCurrentIndex(arms_->findData(profile.contributingArms));
    output_->setCurrentIndex(output_->findData(profile.outputHand));
    steering_->setCurrentIndex(steering_->findData(profile.steering));
    sensitivity_->setValue(profile.sensitivity);
    mappings_->setRowCount(static_cast<int>(profile.mappings.size()));
    for (int row = 0; row < mappings_->rowCount(); ++row) {
        mappings_->setItem(row, 0, new QTableWidgetItem(profile.mappings[row].source));
        mappings_->setItem(row, 1, new QTableWidgetItem(profile.mappings[row].destination));
    }
}
