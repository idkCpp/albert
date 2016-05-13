// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCloseEvent>
#include <QShortcut>
#include <QDesktopWidget>
#include <QFocusEvent>
#include "settingswidget.h"
#include "hotkeymanager.h"
#include "mainwindow.h"
#include "pluginmanager.h"
#include "pluginmodel.h"
#include "iextension.h"


/** ***************************************************************************/
SettingsWidget::SettingsWidget(MainWindow *mainWindow, HotkeyManager *hotkeyManager, PluginManager *pluginManager, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f), mainWindow_(mainWindow), hotkeyManager_(hotkeyManager), pluginManager_(pluginManager) {

    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);


    /*
     * HOTKEY
     */

    QSet<int> hks = hotkeyManager->hotkeys();
    if (hks.size() < 1)
        ui.grabKeyButton_hotkey->setText("Press to set hotkey");
    else
        ui.grabKeyButton_hotkey->setText(QKeySequence(*hks.begin()).toString()); // OMG
    connect(ui.grabKeyButton_hotkey, &GrabKeyButton::keyCombinationPressed,
            this, &SettingsWidget::changeHotkey);


    /*
     * MAINWINDOW
     */

    // ALWAYS CENTER
    ui.checkBox_center->setChecked(mainWindow_->showCentered());
    connect(ui.checkBox_center, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setShowCentered);

    // ALWAYS ON TOP
    ui.checkBox_onTop->setChecked(mainWindow_->alwaysOnTop());
    connect(ui.checkBox_onTop, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setAlwaysOnTop);

    // HIDE ON FOCUS OUT
    ui.checkBox_hideOnFocusOut->setChecked(mainWindow_->hideOnFocusLoss());
    mainWindow_->setHideOnFocusLoss(false); // Disabled while settings are open

    // HIDE ON CLOSE
    ui.checkBox_hideOnClose->setChecked(mainWindow_->hideOnClose());
    connect(ui.checkBox_hideOnClose, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setHideOnClose);

    // MAX PROPOSALS
    ui.spinBox_proposals->setValue(mainWindow_->maxProposals());
    connect(ui.spinBox_proposals, (void (QSpinBox::*)(int))&QSpinBox::valueChanged,
            mainWindow_, &MainWindow::setMaxProposals);

    // DISPLAY SCROLLBAR
    ui.checkBox_scrollbar->setChecked(mainWindow_->displayScrollbar());
    connect(ui.checkBox_scrollbar, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setDisplayScrollbar);

    // DISPLAY ICONS
    ui.checkBox_icons->setChecked(mainWindow_->displayIcons());
    connect(ui.checkBox_icons, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setDisplayIcons);

    // DISPLAY SHADOW
    ui.checkBox_shadow->setChecked(mainWindow_->displayShadow());
    connect(ui.checkBox_shadow, &QCheckBox::toggled,
            mainWindow_, &MainWindow::setDisplayShadow);

    // THEMES
    QFileInfoList themes;
    int i = 0 ;
    QStringList themeDirs =
            QStandardPaths::locateAll(QStandardPaths::DataLocation, "themes",
                                      QStandardPaths::LocateDirectory);
    for (const QDir &d : themeDirs)
        themes << d.entryInfoList(QStringList("*.qss"), QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo &fi : themes) {
        ui.comboBox_themes->addItem(fi.baseName(), fi.canonicalFilePath());
        if ( fi.baseName() == mainWindow_->theme())
            ui.comboBox_themes->setCurrentIndex(i);
        ++i;
    }
    connect(ui.comboBox_themes, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
            this, &SettingsWidget::onThemeChanged);


    /*
     * PLUGINS
     */

    // Show the plugins. This* widget takes ownership of the model
    ui.listView_plugins->setModel(new PluginModel(pluginManager_, ui.listView_plugins));

    // Update infos when item is changed
    connect(ui.listView_plugins->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SettingsWidget::updatePluginInformations);

    connect(ui.listView_plugins->model(), &QAbstractListModel::dataChanged,
            this, &SettingsWidget::onPluginDataChanged);

    // Initially hide the title label
    ui.label_pluginTitle->hide();


    /*
     * ABOUT
     */

    ui.about_text->setText(QString(ui.about_text->text()).replace("___versionstring___", qApp->applicationVersion()));

    QDesktopWidget *dw = QApplication::desktop();
    move(dw->availableGeometry(dw->screenNumber(QCursor::pos())).center()
                -QPoint(width()/2,height()/2));
    raise();
    activateWindow();
}



/** ***************************************************************************/
SettingsWidget::~SettingsWidget() {
    mainWindow_->setHideOnFocusLoss(ui.checkBox_hideOnFocusOut->isChecked()); // Disabled while settings are open
}



/** ***************************************************************************/
void SettingsWidget::updatePluginInformations(const QModelIndex & current) {
    // Hidde the placehodler text
    QLayoutItem *i = ui.widget_pluginInfos->layout()->takeAt(1);
    delete i->widget();
    delete i;

    if (pluginManager_->plugins()[current.row()]->isLoaded()){
        IExtension *e = dynamic_cast<IExtension*>(pluginManager_->plugins()[current.row()]->instance());
        QWidget *pw = e->widget();
        if ( pw->layout() != nullptr)
            pw->layout()->setMargin(0);
        ui.widget_pluginInfos->layout()->addWidget(pw);// Takes ownership
        ui.label_pluginTitle->setText(e->name_);
        ui.label_pluginTitle->show();
    }
    else{
        QLabel *lbl = new QLabel("Plugin not loaded.");
        lbl->setEnabled(false);
        lbl->setAlignment(Qt::AlignCenter);
        ui.widget_pluginInfos->layout()->addWidget(lbl);
        ui.label_pluginTitle->hide();
    }
}



/** ***************************************************************************/
void SettingsWidget::changeHotkey(int newhk) {
    int oldhk = *hotkeyManager_->hotkeys().begin(); //TODO Make cool sharesdpointer design

    // Try to set the hotkey
    if (hotkeyManager_->registerHotkey(newhk)) {
        QString hkText(QKeySequence((newhk&~Qt::GroupSwitchModifier)).toString());//QTBUG-45568
        ui.grabKeyButton_hotkey->setText(hkText);
        QSettings().setValue("hotkey", hkText);
        hotkeyManager_->unregisterHotkey(oldhk);
    } else {
        ui.grabKeyButton_hotkey->setText(QKeySequence(oldhk).toString());
        QMessageBox(QMessageBox::Critical, "Error",
                    QKeySequence(newhk).toString() + " could not be registered.",
                    QMessageBox::NoButton,
                    this).exec();
    }
}



/** ***************************************************************************/
void SettingsWidget::onThemeChanged(int i) {
    // Apply and save the theme
    QString currentTheme = mainWindow_->theme();
    if (!mainWindow_->setTheme(ui.comboBox_themes->itemText(i))) {
        QMessageBox(QMessageBox::Critical, "Error",
                    "Could not apply theme.",
                    QMessageBox::NoButton,
                    this).exec();
        if (!mainWindow_->setTheme(currentTheme)) {
           qFatal("Rolling back theme failed.");
        }
    }
}



/** ***************************************************************************/
void SettingsWidget::onPluginDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
    Q_UNUSED(bottomRight)
    if (topLeft == ui.listView_plugins->currentIndex())
        for (int role : roles)
            if (role == Qt::CheckStateRole)
                updatePluginInformations(topLeft);
}



/** ***************************************************************************/
void SettingsWidget::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Escape ) {
        close();
    }
}



/** ***************************************************************************/
void SettingsWidget::closeEvent(QCloseEvent *event) {
    if (hotkeyManager_->hotkeys().empty()) {
        QMessageBox msgBox(QMessageBox::Critical, "Error",
                           "Hotkey is invalid, please set it. Press Ok to go "\
                           "back to the settings, or press Cancel to quit albert.",
                           QMessageBox::Close|QMessageBox::Ok,
                           this);
        msgBox.exec();
        if ( msgBox.result() == QMessageBox::Ok ) {
            ui.tabs->setCurrentIndex(0);
            show();
        }
        else
            qApp->quit();
        event->ignore();
        return;
    }
    event->accept();
}
