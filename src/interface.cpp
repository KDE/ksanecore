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

// Qt includes
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>
#include <QMutex>
#include <QUrl>

// Sane includes
extern "C" {
#include <sane/sane.h>
#include <sane/saneopts.h>
}

#include "interface.h"
#include "interface_p.h"

#include <ksanecore_debug.h>

namespace KSaneCore
{
static int s_objectCount = 0;

Q_GLOBAL_STATIC(QMutex, s_objectMutex)

Interface::Interface(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<InterfacePrivate>(this))
{
    SANE_Int version;
    SANE_Status status;

    s_objectMutex->lock();
    s_objectCount++;

    if (s_objectCount == 1) {
        // only call sane init for the first instance
        status = sane_init(&version, &Authentication::authorization);
        if (status != SANE_STATUS_GOOD) {
            qCDebug(KSANECORE_LOG) << "libksane: sane_init() failed(" << sane_strstatus(status) << ")";
        }
    }
    s_objectMutex->unlock();

    d->m_readValuesTimer.setSingleShot(true);
    connect(&d->m_readValuesTimer, &QTimer::timeout, d.get(), &InterfacePrivate::reloadValues);
}

Interface::~Interface()
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

QString Interface::deviceName() const
{
    return d->m_devName;
}

QString Interface::deviceVendor() const
{
    return d->m_vendor;
}

QString Interface::deviceModel() const
{
    return d->m_model;
}

void Interface::setPreviewResolution(float dpi)
{
    d->m_previewDPI = dpi;
}

bool Interface::reloadDevicesList(const DeviceType type)
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

Interface::OpenStatus Interface::openDevice(const QString &deviceName)
{
    SANE_Status status;

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

Interface::OpenStatus Interface::openRestrictedDevice(const QString &deviceName, const QString &userName, const QString &password)
{
    SANE_Status status;

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

bool Interface::closeDevice()
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

void Interface::startScan()
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
    d->emitProgress(-1);
    d->m_scanThread->start();
}

void Interface::startPreviewScan()
{
    d->m_previewScan = true;
    Option *topLeftXOption = getOption(Interface::TopLeftXOption);
    Option *topLeftYOption = getOption(Interface::TopLeftYOption);
    Option *bottomRightXOption = getOption(Interface::BottomRightXOption);
    Option *bottomRightYOption = getOption(Interface::BottomRightYOption);
    Option *previewOption = getOption(Interface::PreviewOption);
    Option *resolutionOption = getOption(Interface::ResolutionOption);
    Option *bitDepthOption = getOption(Interface::BitDepthOption);
    Option *yResolutionOption = getOption(Interface::YResolutionOption);
    Option *xResolutionOption = getOption(Interface::XResolutionOption);

    int targetPreviewDPI;
    if (topLeftXOption != nullptr) {
        topLeftXOption->storeCurrentData();
        topLeftXOption->setValue(topLeftXOption->minimumValue());
    }
    if (topLeftYOption != nullptr) {
        topLeftYOption->storeCurrentData();
        topLeftYOption->setValue(topLeftYOption->minimumValue());
    }
    if (bottomRightXOption != nullptr) {
        bottomRightXOption->storeCurrentData();
        bottomRightXOption->setValue(bottomRightXOption->maximumValue());
    }
    if (bottomRightYOption != nullptr) {
        bottomRightYOption->storeCurrentData();
        bottomRightYOption->setValue(bottomRightYOption->maximumValue());
    }

    if (resolutionOption != nullptr) {
        resolutionOption->storeCurrentData();
        if (d->m_previewDPI < resolutionOption->minimumValue().toFloat()) {
            targetPreviewDPI = qMax(resolutionOption->minimumValue().toFloat(), 25.0f);
            if ((bottomRightXOption != nullptr) && (bottomRightYOption != nullptr)) {
                if (bottomRightXOption->valueUnit() == KSaneCore::Option::UnitMilliMeter) {
                    targetPreviewDPI = 300 * 25.4 / (bottomRightXOption->value().toFloat());
                    // always round to a multiple of 25
                    int remainder = targetPreviewDPI % 25;
                    targetPreviewDPI = targetPreviewDPI + 25 - remainder;
                }
            }
        } else {
            targetPreviewDPI = d->m_previewDPI;
        }
        if (resolutionOption->type() == KSaneCore::Option::TypeValueList) {
            const auto &values = resolutionOption->valueList();
            if (values.count() <= 0) {
                qCWarning(KSANECORE_LOG) << "Resolution option is broken and has no entries";
                return;
            }
            /* if there are discrete values, try to find the one which fits best. */
            int minIndex = 0;
            int minDistance = abs(values.at(0).toInt() - d->m_previewDPI);
            for (int i = 1; i < values.count(); ++i) {
                int distance = abs(values.at(i).toInt() - d->m_previewDPI);
                if (distance < minDistance) {
                    minIndex = i;
                    minDistance = distance;
                }
            }
            targetPreviewDPI = values.at(minIndex).toInt();
        }

        resolutionOption->setValue(targetPreviewDPI);
        if ((yResolutionOption != nullptr) && (resolutionOption == xResolutionOption)) {
            yResolutionOption->storeCurrentData();
            yResolutionOption->setValue(targetPreviewDPI);
        }
    }
    if (bitDepthOption != nullptr) {
        bitDepthOption->storeCurrentData();
        if (bitDepthOption->value() == 16) {
            bitDepthOption->setValue(8);
        }
    }
    if (previewOption != nullptr) {
        previewOption->setValue(true);
    }
    startScan();
}

void Interface::stopScan()
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

QImage *Interface::scanImage() const
{
    if (d->m_saneHandle != nullptr) {
        return d->m_scanThread->scanImage();
    }
    return nullptr;
}

void Interface::lockScanImage()
{
    if (d->m_saneHandle != nullptr) {
        d->m_scanThread->lockScanImage();
    }
}

void Interface::unlockScanImage()
{
    if (d->m_saneHandle != nullptr) {
        d->m_scanThread->unlockScanImage();
    }
}

QJsonObject Interface::scannerDeviceToJson()
{
    if (d->m_saneHandle == nullptr) {
        return QJsonObject();
    }
    QJsonObject scannerData;
    scannerData[QLatin1String("deviceName")] = d->m_devName;
    scannerData[QLatin1String("deviceModel")] = d->m_model;
    scannerData[QLatin1String("deviceVendor")] = d->m_vendor;

    return scannerData;
}

QJsonObject Interface::scannerOptionsToJson()
{
    if (d->m_saneHandle == nullptr) {
        return QJsonObject();
    }

    QJsonObject optionData;
    for (const auto &option : std::as_const(d->m_optionsList)) {
        QJsonObject JsonOption;
        JsonOption[QLatin1String("Title")] = option->title();
        JsonOption[QLatin1String("Description")] = option->description();
        JsonOption[QLatin1String("Type")] = QLatin1String(QMetaEnum::fromType<KSaneCore::Option::OptionType>().valueToKey(option->type()));
        JsonOption[QLatin1String("State")] = QLatin1String(QMetaEnum::fromType<KSaneCore::Option::OptionState>().valueToKey(option->state()));
        JsonOption[QLatin1String("Unit")] = QLatin1String(QMetaEnum::fromType<KSaneCore::Option::OptionUnit>().valueToKey(option->valueUnit()));
        JsonOption[QLatin1String("Value size")] = option->valueSize();
        JsonOption[QLatin1String("Step value")] = option->stepValue().toString();
        JsonOption[QLatin1String("Current value")] = option->value().toString();
        JsonOption[QLatin1String("Max value")] = option->maximumValue().toString();
        JsonOption[QLatin1String("Min value")] = option->minimumValue().toString();
        JsonOption[QLatin1String("Value list")] = QJsonArray::fromVariantList(option->valueList());
        JsonOption[QLatin1String("Internal value list")] = QJsonArray::fromVariantList(option->internalValueList());
        optionData[option->name()] = JsonOption;
    }
    return optionData;
}

QList<Option *> Interface::getOptionsList()
{
    return d->m_externalOptionsList;
}

Option *Interface::getOption(Interface::OptionName optionEnum)
{
    auto it = d->m_optionsLocation.find(optionEnum);
    if (it != d->m_optionsLocation.end()) {
        return d->m_externalOptionsList.at(it.value());
    }
    return nullptr;
}

Option *Interface::getOption(const QString &optionName)
{
    for (const auto &option : std::as_const(d->m_externalOptionsList)) {
        if (option->name() == optionName) {
            return option;
        }
    }
    return nullptr;
}

QMap<QString, QString> Interface::getOptionsMap()
{
    QMap<QString, QString> options;
    QString tmp;

    for (const auto option : std::as_const(d->m_optionsList)) {
        tmp = option->valueAsString();
        if (!tmp.isEmpty()) {
            options[option->name()] = tmp;
        }
    }
    return options;
}

int Interface::setOptionsMap(const QMap<QString, QString> &options)
{
    if (!d->m_saneHandle || d->m_scanThread->isRunning()) {
        return -1;
    }

    QMap<QString, QString> optionMapCopy = options;

    int i;
    int ret = 0;

    Option *sourceOption = getOption(SourceOption);
    Option *modeOption = getOption(ScanModeOption);
    Option *resolutionOption = getOption(ResolutionOption);

    // Priorize source option
    if (sourceOption != nullptr) {
        auto it = optionMapCopy.find(sourceOption->name());
        if (it != optionMapCopy.end() && sourceOption->setValue(it.value())) {
            ret++;
            optionMapCopy.erase(it);
        }
    }

    // Priorize mode option
    if (modeOption != nullptr) {
        auto it = optionMapCopy.find(modeOption->name());
        if (it != optionMapCopy.end() && modeOption->setValue(it.value())) {
            ret++;
            optionMapCopy.erase(it);
        }
    }

    // Get iterator to resolution option, but do not apply value
    QString value;
    if (resolutionOption != nullptr) {
        auto it = optionMapCopy.find(resolutionOption->name());
        if (it != optionMapCopy.end()) {
            value = it.value();
            optionMapCopy.erase(it);
        }
    }

    // Update remaining options
    for (i = 0; i < d->m_optionsList.size(); i++) {
        const auto it = optionMapCopy.find(d->m_optionsList.at(i)->name());
        if (it != optionMapCopy.end() && d->m_optionsList.at(i)->setValue(it.value())) {
            ret++;
        }
    }

    // Apply resolution value at the latest, as this option is likely
    // reset to a default value by SANE when setting other options
    if (resolutionOption->setValue(value)) {
        ret++;
    }

    return ret;
}

} // NameSpace KSaneCore

#include "moc_interface.cpp"
