/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_CORE_PRIVATE_H
#define KSANE_CORE_PRIVATE_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QTime>
#include <QTimer>
#include <QVarLengthArray>
#include <QVector>

#include "authentication.h"
#include "baseoption.h"
#include "finddevicesthread.h"
#include "interface.h"
#include "scanthread.h"

/** This namespace collects all methods and classes in LibKSane. */
namespace KSaneCore
{

class InterfacePrivate : public QObject
{
    Q_OBJECT

public:
    explicit InterfacePrivate(Interface *parent);
    Interface::OpenStatus loadDeviceOptions();
    void clearDeviceOptions();
    void setDefaultValues();
    void scanIsFinished(Interface::ScanStatus status, const QString &message);

public Q_SLOTS:
    void devicesListUpdated();
    void signalDevicesListUpdate();
    void imageScanFinished();
    void scheduleValuesReload();
    void reloadOptions();
    void reloadValues();
    void emitProgress(int progress);

Q_SIGNALS:
    void optionsAboutToBeReloaded();
    void optionsReloaded();

private Q_SLOTS:
    void determineMultiPageScanning(const QVariant &value);
    void setWaitForExternalButton(const QVariant &value);
    void pollPollOptions();
    void batchModeTimerUpdate();

public:
    // device info
    SANE_Handle m_saneHandle = nullptr;
    QString m_devName;
    QString m_vendor;
    QString m_model;

    // Option variables
    QList<BaseOption *> m_optionsList;
    QList<Option *> m_externalOptionsList;
    QHash<Interface::OptionName, int> m_optionsLocation;
    QList<BaseOption *> m_optionsPollList;
    QTimer m_readValuesTimer;
    QTimer m_optionPollTimer;
    bool m_optionPollingNaughtylisted = false;

    QString m_saneUserName;
    QString m_sanePassword;

    ScanThread *m_scanThread = nullptr;
    FindSaneDevicesThread *m_findDevThread;
    Authentication *m_auth;
    Interface *q;

    // state variables
    // determines whether a preview scan is carried out
    bool m_previewScan = false;
    float m_previewDPI = 50;
    // determines whether scanner will send multiple images
    bool m_executeMultiPageScanning = false;
    // scanning has been cancelled externally
    bool m_cancelMultiPageScan = false;
    // next scanning will start with a hardware button press
    bool m_waitForExternalButton = false;
    // batch mode options
    BaseOption *m_batchMode = nullptr;
    BaseOption *m_batchModeDelay = nullptr;
    QTimer m_batchModeTimer;
    int m_batchModeCounter = 0;
};

} // NameSpace KSaneCore

#endif // KSANE_CORE_PRIVATE_H
