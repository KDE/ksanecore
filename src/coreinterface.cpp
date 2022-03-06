/*
 * SPDX-FileCopyrightText: 2007-2010 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2009 Matthias Nagl <matthias at nagl dot info>
 * SPDX-FileCopyrightText: 2009 Grzegorz Kurtyka <grzegorz dot kurtyka at gmail dot com>
 * SPDX-FileCopyrightText: 2007-2008 Gilles Caulier <caulier dot gilles at gmail dot com>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 * 
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

//Qt includes

#include <QMutex>

// Sane includes
extern "C"
{
#include <sane/saneopts.h>
#include <sane/sane.h>
}

#include "coreinterface.h"
#include "coreinterface_p.h"

#include <ksanecore_debug.h>

namespace KSane
{
static int s_objectCount = 0;

Q_GLOBAL_STATIC(QMutex, s_objectMutex)

CoreInterface::CoreInterface(QObject *parent)
    : QObject(parent), d(std::unique_ptr<CoreInterfacePrivate>(new CoreInterfacePrivate(this)))
{
    SANE_Int    version;
    SANE_Status status;

    s_objectMutex->lock();
    s_objectCount++;

    if (s_objectCount == 1) {
        // only call sane init for the first instance
        status = sane_init(&version, &Authentication::authorization);
        if (status != SANE_STATUS_GOOD) {
            qCDebug(KSANECORE_LOG) << "libksane: sane_init() failed("
                     << sane_strstatus(status) << ")";
        }
    }
    s_objectMutex->unlock();

    d->m_readValuesTimer.setSingleShot(true);
    connect(&d->m_readValuesTimer, &QTimer::timeout, d.get(), &CoreInterfacePrivate::reloadValues);
}

CoreInterface::~CoreInterface()
{
    closeDevice();

    s_objectMutex->lock();
    s_objectCount--;
    if (s_objectCount <= 0) {
        // only delete the find-devices and authorization singletons and call sane_exit
        // if this is the last instance
        delete d->m_findDevThread;
        delete d->m_auth;
        sane_exit();
    }
    s_objectMutex->unlock();
}

QString CoreInterface::deviceName() const
{
    return d->m_devName;
}

QString CoreInterface::deviceVendor() const
{
    return d->m_vendor;
}

QString CoreInterface::deviceModel() const
{
    return d->m_model;
}

bool CoreInterface::reloadDevicesList(const DeviceType type)
{
    /* On some SANE backends, the handle becomes invalid when
     * querying for new devices. Hence, this is only allowed when
     * no device is currently opened. */
    if (d->m_saneHandle == nullptr) {
        d->m_findDevThread->setDeviceType(type);
        d->m_findDevThread->start();
        return true;
    }
    return false;
}

CoreInterface::OpenStatus CoreInterface::openDevice(const QString &deviceName)
{
    SANE_Status                    status;

    if (d->m_saneHandle != nullptr) {
        // this CoreInterface already has an open device
        return OpenStatus::OpeningFailed;
    }

    // don't bother trying to open if the device string is empty
    if (deviceName.isEmpty()) {
        return OpenStatus::OpeningFailed;
    }
    // save the device name
    d->m_devName = deviceName;

    // Try to open the device
    status = sane_open(deviceName.toLatin1().constData(), &d->m_saneHandle);

    if (status == SANE_STATUS_ACCESS_DENIED) {
        return OpenStatus::OpeningDenied;
    }
 
    if (status != SANE_STATUS_GOOD) {
        qCDebug(KSANECORE_LOG) << "sane_open(\"" << deviceName << "\", &handle) failed! status = " << sane_strstatus(status);
        d->m_devName.clear();
        return OpenStatus::OpeningFailed;
    }

    return d->loadDeviceOptions();
}

CoreInterface::OpenStatus CoreInterface::openRestrictedDevice(const QString &deviceName, QString userName, QString password)
{
    SANE_Status                    status;

    if (d->m_saneHandle != nullptr) {
        // this CoreInterface already has an open device
        return OpenStatus::OpeningFailed;
    }

    // don't bother trying to open if the device string is empty
    if (deviceName.isEmpty()) {
        return OpenStatus::OpeningFailed;
    }
    // save the device name
    d->m_devName = deviceName;
    
    // add/update the device user-name and password for authentication
    d->m_auth->setDeviceAuth(d->m_devName, userName, password);
    
    // Try to open the device
    status = sane_open(deviceName.toLatin1().constData(), &d->m_saneHandle);

    if (status == SANE_STATUS_ACCESS_DENIED) {
        return OpenStatus::OpeningDenied;
    }

    if (status != SANE_STATUS_GOOD) {
        qCDebug(KSANECORE_LOG) << "sane_open(\"" << deviceName << "\", &handle) failed! status = " << sane_strstatus(status);
        d->m_auth->clearDeviceAuth(d->m_devName);
        d->m_devName.clear();
        return OpenStatus::OpeningFailed;
    }
    
    return d->loadDeviceOptions();
}

bool CoreInterface::closeDevice()
{
    if (!d->m_saneHandle) {
        return false;
    }
    stopScan();
    
    disconnect(d->m_scanThread);
    if (d->m_scanThread->isRunning()) {
        connect(d->m_scanThread, &QThread::finished, d->m_scanThread, &QThread::deleteLater);
    }
    if (d->m_scanThread->isFinished()) {
        d->m_scanThread->deleteLater();
    }
    d->m_scanThread = nullptr;
    
    d->m_auth->clearDeviceAuth(d->m_devName);
    sane_close(d->m_saneHandle);
    d->m_saneHandle = nullptr;
    d->clearDeviceOptions();

    return true;
}

void CoreInterface::startScan()
{
    if (!d->m_saneHandle) {
        return;
    }
    d->m_cancelMultiPageScan = false;
    // execute a pending value reload
    while (d->m_readValuesTimer.isActive()) {
        d->m_readValuesTimer.stop();
        d->reloadValues();
    }
    d->m_optionPollTimer.stop();
    Q_EMIT scanProgress(-1);
    d->m_scanThread->start();
}

void CoreInterface::stopScan()
{
    if (!d->m_saneHandle) {
        return;
    }
    
    d->m_cancelMultiPageScan = true;
    if (d->m_scanThread->isRunning()) {
        d->m_scanThread->cancelScan();
    }
    if (d->m_batchModeTimer.isActive()) {
        d->m_batchModeTimer.stop();
        Q_EMIT batchModeCountDown(0);
        Q_EMIT scanFinished(ScanStatus::NoError, i18n("Scanning stopped by user."));
    }
}

QImage *CoreInterface::scanImage() const
{
    if (d->m_saneHandle != nullptr) {
        return d->m_scanThread->scanImage();
    }
    return nullptr;
}

void CoreInterface::lockScanImage()
{
    if (d->m_saneHandle != nullptr) {
        d->m_scanThread->lockScanImage();
    }
}

void CoreInterface::unlockScanImage()
{
    if (d->m_saneHandle != nullptr) {
        d->m_scanThread->unlockScanImage();
    }
}

QList<CoreOption *> CoreInterface::getOptionsList()
{
    return d->m_externalOptionsList;
}

CoreOption *CoreInterface::getOption(CoreInterface::OptionName optionEnum)
{
    auto it = d->m_optionsLocation.find(optionEnum);
    if (it != d->m_optionsLocation.end()) {
        return d->m_externalOptionsList.at(it.value());
    }
    return nullptr;
}

CoreOption *CoreInterface::getOption(const QString &optionName)
{
    for (const auto &option : qAsConst(d->m_externalOptionsList)) {
        if (option->name() == optionName) {
            return option;
        }
    }
    return nullptr;
}

QMap <QString, QString> CoreInterface::getOptionsMap()
{
    BaseOption *option;
    QMap <QString, QString> options;
    QString tmp;

    for (int i = 0; i < d->m_optionsList.size(); i++) {
        option = d->m_optionsList.at(i);
        tmp = option->valueAsString();
        if (!tmp.isEmpty()) {
            options[option->name()] = tmp;
        }
    }
    return options;
}

int CoreInterface::setOptionsMap(const QMap <QString, QString> &opts)
{
    if (!d->m_saneHandle || d->m_scanThread->isRunning()) {
        return -1;
    }

    QMap <QString, QString> optionMapCopy = opts;

    int i;
    int ret = 0;

    CoreOption *sourceOption = getOption(SourceOption);
    CoreOption *modeOption = getOption(ScanModeOption);
    
    // Priorize source option
    if (sourceOption != nullptr && optionMapCopy.contains(sourceOption->name())) {
        if (sourceOption->setValue(optionMapCopy[sourceOption->name()]) ) {
            ret++;
        }
        optionMapCopy.remove(sourceOption->name());
    }

    // Priorize mode option
    if (modeOption != nullptr && optionMapCopy.contains(modeOption->name())) {
        if (modeOption->setValue(optionMapCopy[modeOption->name()])) {
            ret++;
        }
        optionMapCopy.remove(modeOption->name());
    }

    // Update remaining options
    for (i = 0; i < d->m_optionsList.size(); i++) {
        const auto it = optionMapCopy.find(d->m_optionsList.at(i)->name());
        if (it != optionMapCopy.end() && d->m_optionsList.at(i)->setValue(it.value())) {
            ret++;
        }
    }
    return ret;
}

}  // NameSpace KSane
