
#pragma once

#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include "iextension.h"

#define AUTOCONF_MEMBER AutoConf *configurator;
#define AUTOCONF_SETUP(widget)  configurator = new AutoConf(widget, name_)
//#define AUTOCONF(sender, key, store)    configurator->registerUIItem(sender, key, store)
#define AUTOCONF(sender, key, store, init) configurator->registerUIItem(sender, key, store, init)

class Listener;

class AutoConf : public QObject
{
    Q_OBJECT

public:
    AutoConf(QObject *parent, QString group);
    AutoConf(QObject *parent, const char *group);
    AutoConf(QObject *parent, IExtension *ext);
    AutoConf(QObject *parent, IExtension &ext);
    virtual ~AutoConf();

    void registerUIItem(QLineEdit *, const char* key, QString& storage, const char* initialValue = nullptr);
    void registerUIItem(QLineEdit *, QString key, QString& storage, QString initialValue = QStringLiteral(""));

    void registerUIItem(QListWidget *, const char* key, QStringList &storage, QStringList &initialValue = emptyStringList);
    void registerUIItem(QListWidget *, QString key, QStringList &storage, QStringList &initialValue = emptyStringList);

    void registerUIItem(QCheckBox *, const char *key, bool &storage, bool initialValue = false);
    void registerUIItem(QCheckBox *, QString key, bool &storage, bool initialValue = false);

    void registerUIItem(QSpinBox *, const char *key, int &storage, int initialValue = 0);
    void registerUIItem(QSpinBox *, QString key, int &storage, int initialValue = 0);
    void registerUIItem(QSpinBox *, const char *key, uint &storage, uint initialValue = 0);
    void registerUIItem(QSpinBox *, QString key, uint &storage, uint initialValue = 0);

public slots:
    void destroyed(QObject *toDestroy);

private:
    static QStringList emptyStringList;
    QString configGroup;
    QList<Listener*> listeners;
};

