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

#include "xdgiconlookup.h"
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>


/** ***************************************************************************/
class ThemeFileParser
{
public:
    QSettings iniFile_;
    QString path_;

    ThemeFileParser(const QString &iniFile)
        : iniFile_(iniFile, QSettings::IniFormat), path_(QFileInfo(iniFile).path()){
    }

    QString path() {return path_;}

    QString name() {return iniFile_.value("Icon Theme/Name").toString();}

    QString comment() {return iniFile_.value("Icon Theme/Comment").toString();}

    QStringList inherits() {return iniFile_.value("Icon Theme/Inherits").toStringList();}

    QStringList directories() {return iniFile_.value("Icon Theme/Directories").toStringList();}

    bool hidden() {return iniFile_.value("Icon Theme/Hidden").toBool();}

    int size(const QString& directory){
        iniFile_.beginGroup(directory);
        int result = iniFile_.value("Size").toInt();
        iniFile_.endGroup();
        return result;
    }

    QString context(const QString& directory){
        iniFile_.beginGroup(directory);
        QString result = iniFile_.value("Context").toString();
        iniFile_.endGroup();
        return result;
    }

    QString type(const QString& directory) {
        iniFile_.beginGroup(directory);
        QString result = iniFile_.contains("Type")
                ? iniFile_.value("Type").toString()
                : "Threshold";
        iniFile_.endGroup();
        return result;
    }

    int maxSize(const QString& directory){
        iniFile_.beginGroup(directory);
        int result = iniFile_.contains("MaxSize")
                ? iniFile_.value("MaxSize").toInt()
                : size(directory);
        iniFile_.endGroup();
        return result;
    }

    int minSize(const QString& directory){
        iniFile_.beginGroup(directory);
        int result = iniFile_.contains("MinSize")
                ? iniFile_.value("MinSize").toInt()
                : size(directory);
        iniFile_.endGroup();
        return result;
    }

    int threshold(const QString& directory){
        iniFile_.beginGroup(directory);
        int result = iniFile_.contains("Threshold")
                ? iniFile_.value("Threshold").toInt()
                : 2;
        iniFile_.endGroup();
        return result;
    }
};

/** ***************************************************************************/

XdgIconLookup *XdgIconLookup::instance_;
QStringList XdgIconLookup::icon_extensions = {"png", "svg", "xpm"};

/** ***************************************************************************/
XdgIconLookup::XdgIconLookup()
{
    /* Icons and themes are looked for in a set of directories. By default,
    apps should look in $HOME/.icons (for backwards compatibility), in
    $XDG_DATA_DIRS/icons and in /usr/share/pixmaps (in that order). */
    QString path;

    path = "~/.icons";
    if (QFile::exists(path))
        iconDirs_.append(path);

    for (const QString &basedir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)){
        path = QDir(basedir).filePath("icons");
        if (QFile::exists(path))
            iconDirs_.append(path);
    }

    path = "/usr/share/pixmaps";
    if (QFile::exists(path))
        iconDirs_.append(path);
}




/** ***************************************************************************/
XdgIconLookup *XdgIconLookup::instance()
{
    if (!instance_)
        instance_ = new XdgIconLookup();
    return instance_;
}


/** ***************************************************************************/
QString XdgIconLookup::themeIconPath(QString iconName, QString themeName){

    // if we have an absolute path, just return it
    if (iconName[0]=='/')
        return iconName;

    // check if it has an extension and strip it
    for (const QString &ext : icon_extensions)
        if (iconName.endsWith(QString(".").append(ext)))
            iconName.chop(4);

    // Check cache
    QString iconPath = iconCache_.value(iconName);
    if (!iconPath.isNull())
        return iconPath;

    // Lookup themefile
    QStringList checkedThemes;
    iconPath = doRecursiveIconLookup(iconName, themeName, &checkedThemes);
    if (!iconPath.isNull()){
        iconCache_.insert(iconName, iconPath);
        return iconPath;
    }

    // Lookup in hicolor
    iconPath = doRecursiveIconLookup(iconName, "hicolor", &checkedThemes);
    if (!iconPath.isNull()){
        iconCache_.insert(iconName, iconPath);
        return iconPath;
    }

    // Now search unsorted
    for (const QString &iconDir : iconDirs_){
        for (const QString &ext : icon_extensions){
            QString filename = QString("%1/%2.%3").arg(iconDir, iconName, ext);
            if (QFile(filename).exists()){
                return filename;
            }
        }
    }

    return QString();
}



/** ***************************************************************************/
QString XdgIconLookup::doRecursiveIconLookup(const QString &iconName, const QString &themeName, QStringList *checked){

    // Exlude multiple scans
    if (checked->contains(themeName))
        return QString();
    checked->append(themeName);

    // Check if theme exists
    QString themeFile = lookupThemeFile(themeName);
    if (themeFile.isNull())
        return QString();

    // Check if icon exists
    QString iconPath;
    iconPath = doIconLookup(iconName, themeFile);
    if (!iconPath.isNull())
        return iconPath;

    // Check its parents too
    for (const QString &parent : ThemeFileParser(themeFile).inherits()){
        iconPath = doRecursiveIconLookup(iconName, parent, checked);
        if (!iconPath.isNull())
            return iconPath;
    }

    return QString();
}



/** ***************************************************************************/
QString XdgIconLookup::doIconLookup(const QString &iconName, const QString &themeFile) {

    ThemeFileParser themeFileParser(themeFile);
    QDir themeDir = QFileInfo(themeFile).dir();
    QString themeName = themeDir.dirName();

    // Get the sizes of the dirs
    std::vector<std::pair<QString, int>> dirsAndSizes;
    for (const QString &subdir : themeFileParser.directories())
        dirsAndSizes.push_back(std::make_pair(subdir, themeFileParser.size(subdir)));

    // Sort them by size
    std::sort(dirsAndSizes.begin(), dirsAndSizes.end(),
              [](std::pair<QString, int>  a, std::pair<QString, int> b) {
                  return a.second > b.second;
              });

    // Well now search for a file beginning with the greatest
    QString filename;
    QFile file;
    for (const auto &dirAndSize : dirsAndSizes){
        for (const QString &iconDir : iconDirs_){
            for (const QString &ext : icon_extensions){
                filename = QString("%1/%2/%3/%4.%5").arg(iconDir, themeName, dirAndSize.first, iconName, ext);
                if (file.exists(filename)){
                    return filename;
                }
            }
        }
    }

    return QString();
}


/** ***************************************************************************/
QString XdgIconLookup::lookupThemeFile(const QString &themeName)
{
    // Lookup themefile
    for (const QString &iconDir : iconDirs_){
        QString indexFile = QString("%1/%2/index.theme").arg(iconDir, themeName);
        if (QFile(indexFile).exists())
            return indexFile;
    }
    return QString();
}
