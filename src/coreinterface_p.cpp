/*
 * SPDX-FileCopyrightText: 2007-2008 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2007-2008 Gilles Caulier <caulier dot gilles at gmail dot com>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "coreinterface_p.h"

#include <QImage>
#include <QRegularExpression>

#include <ksanecore_debug.h>

#include "internaloption.h"
#include "actionoption.h"
#include "booloption.h"
#include "listoption.h"
#include "stringoption.h"
#include "doubleoption.h"
#include "gammaoption.h"
#include "integeroption.h"
#include "invertoption.h"
#include "pagesizeoption.h"
#include "batchmodeoption.h"
#include "batchdelayoption.h"

namespace KSane
{
    
static const QHash<QString, CoreInterface::OptionName> stringEnumTranslation = {
    { QStringLiteral(SANE_NAME_SCAN_SOURCE), CoreInterface::SourceOption },
    { QStringLiteral(SANE_NAME_SCAN_MODE), CoreInterface::ScanModeOption },
    { QStringLiteral(SANE_NAME_BIT_DEPTH), CoreInterface::BitDepthOption },
    { QStringLiteral(SANE_NAME_SCAN_RESOLUTION), CoreInterface::ResolutionOption },
    { QStringLiteral(SANE_NAME_SCAN_TL_X), CoreInterface::TopLeftXOption },
    { QStringLiteral(SANE_NAME_SCAN_TL_Y), CoreInterface::TopLeftYOption },
    { QStringLiteral(SANE_NAME_SCAN_BR_X), CoreInterface::BottomRightXOption },
    { QStringLiteral(SANE_NAME_SCAN_BR_Y), CoreInterface::BottomRightYOption },
    { QStringLiteral("film-type"), CoreInterface::FilmTypeOption },
    { QStringLiteral(SANE_NAME_NEGATIVE), CoreInterface::NegativeOption },
    { InvertColorsOptionName, CoreInterface::InvertColorOption },
    { PageSizeOptionName, CoreInterface::PageSizeOption },
    { QStringLiteral(SANE_NAME_THRESHOLD), CoreInterface::ThresholdOption },
    { QStringLiteral(SANE_NAME_SCAN_X_RESOLUTION), CoreInterface::XResolutionOption },
    { QStringLiteral(SANE_NAME_SCAN_Y_RESOLUTION), CoreInterface::YResolutionOption },
    { QStringLiteral(SANE_NAME_PREVIEW), CoreInterface::PreviewOption },
    { QStringLiteral("wait-for-button"), CoreInterface::WaitForButtonOption },
    { QStringLiteral(SANE_NAME_BRIGHTNESS), CoreInterface::BrightnessOption },
    { QStringLiteral(SANE_NAME_CONTRAST), CoreInterface::ContrastOption },
    { QStringLiteral(SANE_NAME_GAMMA_VECTOR), CoreInterface::GammaOption },
    { QStringLiteral(SANE_NAME_GAMMA_VECTOR_R), CoreInterface::GammaRedOption },
    { QStringLiteral(SANE_NAME_GAMMA_VECTOR_G), CoreInterface::GammaGreenOption },
    { QStringLiteral(SANE_NAME_GAMMA_VECTOR_B), CoreInterface::GammaBlueOption },
    { QStringLiteral(SANE_NAME_BLACK_LEVEL), CoreInterface::BlackLevelOption },
    { QStringLiteral(SANE_NAME_WHITE_LEVEL), CoreInterface::WhiteLevelOption },
    { BatchModeOptionName, CoreInterface::BatchModeOption },
    { BatchDelayOptionName, CoreInterface::BatchDelayOption }, };
    
CoreInterfacePrivate::CoreInterfacePrivate(CoreInterface *parent):
    q(parent)
{
    clearDeviceOptions();

    m_findDevThread = FindSaneDevicesThread::getInstance();
    connect(m_findDevThread, &FindSaneDevicesThread::finished, this, &CoreInterfacePrivate::devicesListUpdated);
    connect(m_findDevThread, &FindSaneDevicesThread::finished, this, &CoreInterfacePrivate::signalDevicesListUpdate);

    m_auth = Authentication::getInstance();
    m_optionPollTimer.setInterval(100);
    connect(&m_optionPollTimer, &QTimer::timeout, this, &CoreInterfacePrivate::pollPollOptions);

    m_batchModeTimer.setInterval(1000);
    connect(&m_batchModeTimer, &QTimer::timeout, this, &CoreInterfacePrivate::batchModeTimerUpdate);
}

CoreInterface::OpenStatus CoreInterfacePrivate::loadDeviceOptions() 
{
    const SANE_Option_Descriptor  *optDesc;
    SANE_Status                    status;
    SANE_Word                      numSaneOptions;
    SANE_Int                       res;
    // update the device list if needed to get the vendor and model info
    if (m_findDevThread->devicesList().size() == 0) {
        m_findDevThread->start();
    } else {
        // use the "old" existing list
        devicesListUpdated();
        // if m_vendor is not updated it means that the list needs to be updated.
        if (m_vendor.isEmpty()) {
            m_findDevThread->start();
        }
    }

    // Read the options (start with option 0 the number of parameters)
    optDesc = sane_get_option_descriptor(m_saneHandle, 0);
    if (optDesc == nullptr) {
        m_auth->clearDeviceAuth(m_devName);
        m_devName.clear();
        return CoreInterface::OpeningFailed;
    }
    QVarLengthArray<char> data(optDesc->size);
    status = sane_control_option(m_saneHandle, 0, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        m_auth->clearDeviceAuth(m_devName);
        m_devName.clear();
        return CoreInterface::OpeningFailed;
    }
    numSaneOptions = *reinterpret_cast<SANE_Word *>(data.data());

    // read the rest of the options
    BaseOption *option = nullptr;
    BaseOption *optionTopLeftX = nullptr;
    BaseOption *optionTopLeftY = nullptr;
    BaseOption *optionBottomRightX = nullptr;
    BaseOption *optionBottomRightY = nullptr;
    BaseOption *optionResolution = nullptr;
    m_optionsList.reserve(numSaneOptions);
    m_externalOptionsList.reserve(numSaneOptions);
    for (int i = 1; i < numSaneOptions; ++i) {
        switch (BaseOption::optionType(sane_get_option_descriptor(m_saneHandle, i))) {
        case CoreOption::TypeDetectFail:
            option = new BaseOption(m_saneHandle, i);
            break;
        case CoreOption::TypeBool:
            option = new BoolOption(m_saneHandle, i);
            break;
        case CoreOption::TypeInteger:
            option = new IntegerOption(m_saneHandle, i);
            break;
        case CoreOption::TypeDouble:
            option = new DoubleOption(m_saneHandle, i);
            break;
        case CoreOption::TypeValueList:
            option = new ListOption(m_saneHandle, i);
            break;
        case CoreOption::TypeString:
            option = new StringOption(m_saneHandle, i);
            break;
        case CoreOption::TypeGamma:
            option = new GammaOption(m_saneHandle, i);
            break;
        case CoreOption::TypeAction:
            option = new ActionOption(m_saneHandle, i);
            break;
        }
        option->readOption();
        option->readValue();

        if (option->name() == QStringLiteral(SANE_NAME_SCAN_TL_X)) {
            optionTopLeftX = option;
        }
        if (option->name() == QStringLiteral(SANE_NAME_SCAN_TL_Y)) {
            optionTopLeftY = option;
        }
        if (option->name() == QStringLiteral(SANE_NAME_SCAN_BR_X)) {
            optionBottomRightX = option;
        }
        if (option->name() == QStringLiteral(SANE_NAME_SCAN_BR_Y)) {
            optionBottomRightY = option;
        }
        if (option->name() == QStringLiteral(SANE_NAME_SCAN_RESOLUTION)) {
            optionResolution = option;
        }
        if (option->name() == QStringLiteral(SANE_NAME_SCAN_SOURCE)) {
            // some scanners only have ADF and never update the source name
            determineMultiPageScanning(option->value());
            connect(option, &BaseOption::valueChanged, this, &CoreInterfacePrivate::determineMultiPageScanning);
        }
        if (option->name() == QStringLiteral("wait-for-button")) {
            connect(option, &BaseOption::valueChanged, this, &CoreInterfacePrivate::setWaitForExternalButton);
        }

        m_optionsList.append(option);
        m_externalOptionsList.append(new InternalOption(option));
        connect(option, &BaseOption::optionsNeedReload, this, &CoreInterfacePrivate::reloadOptions);
        connect(option, &BaseOption::valuesNeedReload, this, &CoreInterfacePrivate::scheduleValuesReload);

        if (option->needsPolling()) {
            m_optionsPollList.append(option);
            if (option->type() == CoreOption::TypeBool) {
                connect(option, &BaseOption::valueChanged, this,
                    [=]( const QVariant &newValue ) { Q_EMIT q->buttonPressed(option->name(), option->title(), newValue.toBool()); } );
            }
        }
        const auto it = stringEnumTranslation.find(option->name());
        if (it != stringEnumTranslation.constEnd()) {
            m_optionsLocation.insert(it.value(), i - 1);
        }
    }

    // add extra option for selecting specific page sizes
    BaseOption *pageSizeOption = new PageSizeOption(optionTopLeftX, optionTopLeftY,
                            optionBottomRightX, optionBottomRightY, optionResolution);
    m_optionsList.append(pageSizeOption);
    m_externalOptionsList.append(new InternalOption(pageSizeOption));
    m_optionsLocation.insert(CoreInterface::PageSizeOption, m_optionsList.size() - 1);

    // add extra option for batch mode scanning with a delay
    m_batchMode = new BatchModeOption();
    m_optionsList.append(m_batchMode);
    m_externalOptionsList.append(new InternalOption(m_batchMode));
    m_optionsLocation.insert(CoreInterface::BatchModeOption, m_optionsList.size() - 1);
    m_batchModeDelay = new BatchDelayOption();
    m_optionsList.append(m_batchModeDelay);
    m_externalOptionsList.append(new InternalOption(m_batchModeDelay));
    m_optionsLocation.insert(CoreInterface::BatchDelayOption, m_optionsList.size() - 1);

    // add extra option for inverting image colors
    BaseOption *invertOption = new InvertOption();
    m_optionsList.append(invertOption);
    m_externalOptionsList.append(new InternalOption(invertOption));
    m_optionsLocation.insert(CoreInterface::InvertColorOption, m_optionsList.size() - 1);

    // NOTICE The Pixma network backend behaves badly. polling a value will result in 1 second
    // sleeps for every poll. The problem has been reported, but no easy/quick fix was available and
    // the bug has been there for multiple years. Since this destroys the usability of the backend totally,
    // we simply put the backend on the naughty list and disable the option polling.
    static QRegularExpression pixmaNetworkBackend(QStringLiteral("pixma.*\\d+\\.\\d+\\.\\d+\\.\\d+"));
    m_optionPollingNaughtylisted = false;
    if (pixmaNetworkBackend.match(m_devName).hasMatch()) {
        m_optionPollingNaughtylisted = true;
    }

    // start polling the poll options
    if (m_optionsPollList.size() > 0 && !m_optionPollingNaughtylisted) {
        m_optionPollTimer.start();
    }

    // Create the scan thread
    m_scanThread = new ScanThread(m_saneHandle);

    m_scanThread->setImageInverted(invertOption->value());
    connect(invertOption, &InvertOption::valueChanged, m_scanThread, &ScanThread::setImageInverted);

    if (optionResolution != nullptr) {
        m_scanThread->setImageResolution(optionResolution->value());    
        connect(optionResolution, &BaseOption::valueChanged, m_scanThread, &ScanThread::setImageResolution);
    }

    connect(m_scanThread, &ScanThread::scanProgressUpdated, q, &CoreInterface::scanProgress);
    connect(m_scanThread, &ScanThread::finished, this, &CoreInterfacePrivate::imageScanFinished);

    // try to set to default values
    setDefaultValues();
    return CoreInterface::OpeningSucceeded;
}

void CoreInterfacePrivate::clearDeviceOptions()
{
    // delete all the options in the list.
    while (!m_optionsList.isEmpty()) {
        delete m_optionsList.takeFirst();
        delete m_externalOptionsList.takeFirst();
    }

    m_optionsLocation.clear();
    m_optionsPollList.clear();
    m_optionPollTimer.stop();

    m_devName.clear();
    m_model.clear();
    m_vendor.clear();
}

void CoreInterfacePrivate::devicesListUpdated()
{
    if (m_vendor.isEmpty()) {
        const QList<CoreInterface::DeviceInfo> deviceList = m_findDevThread->devicesList();
        for (const auto &device : deviceList) {
            if (device.name == m_devName) {
                m_vendor = device.vendor;
                m_model = device.model;
                break;
            }
        }
    }
}

void CoreInterfacePrivate::signalDevicesListUpdate()
{
    Q_EMIT q->availableDevices(m_findDevThread->devicesList());
}

void CoreInterfacePrivate::setDefaultValues()
{
    CoreOption *option;

    // Try to get Color mode by default
    if ((option = q->getOption(CoreInterface::ScanModeOption)) != nullptr) {
        option->setValue(sane_i18n(SANE_VALUE_SCAN_MODE_COLOR));
    }

    // Try to set 8 bit color
    if ((option = q->getOption(CoreInterface::BitDepthOption)) != nullptr) {
        option->setValue(8);
    }

    // Try to set Scan resolution to 300 DPI
    if ((option = q->getOption(CoreInterface::ResolutionOption)) != nullptr) {
        option->setValue(300);
    }
}

void CoreInterfacePrivate::scheduleValuesReload()
{
    m_readValuesTimer.start(5);
}

void CoreInterfacePrivate::reloadOptions()
{
    for (const auto option : qAsConst(m_optionsList)) {
        option->readOption();
        // Also read the values
        option->readValue();
    }
}

void CoreInterfacePrivate::reloadValues()
{
    for (const auto option : qAsConst(m_optionsList)) {
        option->readValue();
    }
}

void CoreInterfacePrivate::pollPollOptions()
{
    for (int i = 1; i < m_optionsPollList.size(); ++i) {
        m_optionsPollList.at(i)->readValue();
    }
}

void CoreInterfacePrivate::imageScanFinished()
{
    Q_EMIT q->scanProgress(100);
    if (m_scanThread->frameStatus() == ScanThread::ReadReady) {
        Q_EMIT q->scannedImageReady(*m_scanThread->scanImage());
        // now check if we should have automatic ADF batch scanning
        if (m_executeMultiPageScanning && !m_cancelMultiPageScan) {
            // in batch mode only one area can be scanned per page
            Q_EMIT q->scanProgress(-1);
            m_scanThread->start();
            return;
        }
        // check if we should have timed batch scanning
        if (m_batchMode->value().toBool() && !m_cancelMultiPageScan) {
            // in batch mode only one area can be scanned per page
            m_batchModeCounter = 0;
            batchModeTimerUpdate();
            m_batchModeTimer.start();
            return;
        }
        // Check if we have a "wait for button" batch scanning
        if (m_waitForExternalButton) {
            qCDebug(KSANECORE_LOG) << "waiting for external button press to start next scan";
            Q_EMIT q->scanProgress(-1);
            m_scanThread->start();
            return;
        }
        scanIsFinished(CoreInterface::NoError, QString());
    } else {
        switch (m_scanThread->saneStatus()) {
        case SANE_STATUS_GOOD:
        case SANE_STATUS_CANCELLED:
        case SANE_STATUS_EOF:
            scanIsFinished(CoreInterface::NoError, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            break;
        case SANE_STATUS_NO_DOCS:
            Q_EMIT q->userMessage(CoreInterface::Information, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            scanIsFinished(CoreInterface::Information, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            break;
        case SANE_STATUS_UNSUPPORTED:
        case SANE_STATUS_IO_ERROR:
        case SANE_STATUS_NO_MEM:
        case SANE_STATUS_INVAL:
        case SANE_STATUS_JAMMED:
        case SANE_STATUS_COVER_OPEN:
        case SANE_STATUS_DEVICE_BUSY:
        case SANE_STATUS_ACCESS_DENIED:
            Q_EMIT q->userMessage(CoreInterface::ErrorGeneral, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            scanIsFinished(CoreInterface::ErrorGeneral, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            break;
        }
    }
}

void CoreInterfacePrivate::scanIsFinished(CoreInterface::ScanStatus status, const QString &message) 
{
    sane_cancel(m_saneHandle);
    if (m_optionsPollList.size() > 0 && !m_optionPollingNaughtylisted) {
        m_optionPollTimer.start();
    }

    Q_EMIT q->scanFinished(status, message);
}

void CoreInterfacePrivate::determineMultiPageScanning(const QVariant &value)
{
    const QString sourceString = value.toString();

    m_executeMultiPageScanning = sourceString.contains(QStringLiteral("Automatic Document Feeder")) ||
    sourceString.contains(sane_i18n("Automatic Document Feeder")) || 
    sourceString.contains(QStringLiteral("ADF")) ||
    sourceString.contains(QStringLiteral("Duplex"));
}

void CoreInterfacePrivate::setWaitForExternalButton(const QVariant &value)
{
    m_waitForExternalButton = value.toBool();
}

void CoreInterfacePrivate::batchModeTimerUpdate()
{
    const int delay = m_batchModeDelay->value().toInt();
    Q_EMIT q->batchModeCountDown(delay - m_batchModeCounter);
    if (m_batchModeCounter >= delay) {
        m_batchModeCounter = 0;
        if (m_scanThread!= nullptr) {
            Q_EMIT q->scanProgress(-1);
            m_scanThread->start();
        }
        m_batchModeTimer.stop();
    }
    m_batchModeCounter++;
}

}  // NameSpace KSane
