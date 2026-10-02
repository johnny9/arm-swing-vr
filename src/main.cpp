// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_editor.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  public:
    MainWindow() {
        setWindowTitle("Arm Swing VR — Profile editor");
        resize(780, 660);
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        auto* status = new QLabel("Development preview: profiles can be edited and saved. "
                                  "VR movement and remapping are not connected yet.",
                                  this);
        status->setWordWrap(true);
        layout->addWidget(status);
        editor_ = new ProfileEditor(this);
        layout->addWidget(editor_);
        setCentralWidget(central);
        saved_ = editor_->profile();

        auto* file = menuBar()->addMenu("&File");
        file->addAction("&New profile", QKeySequence::New, this, [this] {
            if (confirmDiscard()) {
                editor_->setProfile({});
                saved_ = editor_->profile();
                path_.clear();
                statusBar()->showMessage("New profile");
            }
        });
        file->addAction("&Open profile…", QKeySequence::Open, this, [this] {
            if (!confirmDiscard())
                return;
            const auto path =
                QFileDialog::getOpenFileName(this, "Open profile", {}, "Profiles (*.json)");
            if (path.isEmpty())
                return;
            armswing::Profile profile;
            QString error;
            if (!armswing::loadProfile(path, profile, error)) {
                QMessageBox::warning(this, "Could not open profile", error);
                return;
            }
            editor_->setProfile(profile);
            saved_ = editor_->profile();
            path_ = path;
            statusBar()->showMessage(path_);
        });
        file->addAction("&Save profile", QKeySequence::Save, this, [this] { save(false); });
        file->addAction("Save profile &as…", QKeySequence::SaveAs, this, [this] { save(true); });
        file->addSeparator();
        file->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);
        auto* help = menuBar()->addMenu("&Help");
        help->addAction("About Arm Swing VR", this, [this] {
            QMessageBox::about(this, "Arm Swing VR",
                               "Arm Swing VR " ARMSWING_VERSION "\n"
                               "Copyright © 2026 Arm Swing VR contributors\n\n"
                               "Free software under the GNU GPL version 3 or later.\n"
                               "You may modify and redistribute it under that license.\n"
                               "This program comes with no warranty.\n\n"
                               "License and source: https://github.com/johnny9/arm-swing-vr");
        });
        help->addAction("About Qt", qApp, &QApplication::aboutQt);
    }

  protected:
    void closeEvent(QCloseEvent* event) override {
        if (confirmDiscard())
            event->accept();
        else
            event->ignore();
    }

  private:
    bool save(bool choosePath) {
        if (auto* focused = QApplication::focusWidget())
            focused->clearFocus(); // Commit an in-progress table cell edit before reading it.
        const auto profile = editor_->profile();
        QString error = armswing::validate(profile);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, "Could not save profile", error);
            return false;
        }
        auto destination = path_;
        if (choosePath || destination.isEmpty())
            destination =
                QFileDialog::getSaveFileName(this, "Save profile", path_, "Profiles (*.json)");
        if (destination.isEmpty())
            return false;
        if (!armswing::saveProfile(destination, profile, error)) {
            QMessageBox::warning(this, "Could not save profile", error);
            return false;
        }
        path_ = destination;
        saved_ = profile;
        statusBar()->showMessage("Saved " + path_);
        return true;
    }

    bool confirmDiscard() {
        if (auto* focused = QApplication::focusWidget())
            focused->clearFocus();
        if (editor_->profile() == saved_)
            return true;
        const auto choice = QMessageBox::question(
            this, "Unsaved profile", "Save your profile changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        return choice == QMessageBox::Discard || (choice == QMessageBox::Save && save(false));
    }

    ProfileEditor* editor_;
    armswing::Profile saved_;
    QString path_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Arm Swing VR");
    QCoreApplication::setApplicationVersion(ARMSWING_VERSION);
    QCoreApplication::setOrganizationName("Arm Swing VR");
    QCommandLineParser parser;
    parser.setApplicationDescription("Linux VR arm-swing locomotion — profile editor preview");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    MainWindow window;
    window.show();
    return app.exec();
}
