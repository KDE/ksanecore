/*
 * SPDX-FileCopyrightText: 2009 Grzegorz Kurtyka <grzegorz dot kurtyka at gmail dot com>
 * SPDX-FileCopyrightText: 2010 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "finddevicesthread.h"

// Sane includes
extern "C"
{
#include <sane/saneopts.h>
#include <sane/sane.h>
}

#include <QMutex>
#include <QMutexLocker>

#include <ksanecore_debug.h>

namespace KSane
{
static FindSaneDevicesThread *s_instancesane = nullptr;
Q_GLOBAL_STATIC(QMutex, s_mutexsane)

FindSaneDevicesThread *FindSaneDevicesThread::getInstance()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMutexLocker<QMutex> locker(s_mutexsane);
#else
    QMutexLocker locker(s_mutexsane);
#endif

    if (s_instancesane == nullptr) {
        s_instancesane = new FindSaneDevicesThread();
    }

    return s_instancesane;
}

FindSaneDevicesThread::FindSaneDevicesThread() : QThread(nullptr)
{
}

FindSaneDevicesThread::~FindSaneDevicesThread()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMutexLocker<QMutex> locker(s_mutexsane);
#else
    QMutexLocker locker(s_mutexsane);
#endif
    wait();
}

void FindSaneDevicesThread::run()
{
    SANE_Device const **devList;
    //SANE_Int            version;
    SANE_Status         status;

    // This is unfortunately not very reliable as many back-ends do not refresh
    // the device list after the sane_init() call...
    status = sane_get_devices(&devList, SANE_FALSE);

    m_deviceList.clear();
    if (status == SANE_STATUS_GOOD) {
        int i = 0;
        CoreInterface::DeviceInfo deviceInfo;

        while (devList[i] != nullptr) {
            /* Do not list cameras as scanner devices when requested.
             * Strings taken from SANE API documentation. */
            const QString type = QString::fromUtf8(devList[i]->type);
            if (m_deviceType == CoreInterface::AllDevices || (m_deviceType == CoreInterface::NoCameraAndVirtualDevices &&
                type != QLatin1String("still camera") && type != QLatin1String("video camera") && type != QLatin1String("virtual device"))) {
                deviceInfo.name = QString::fromUtf8(devList[i]->name);
                deviceInfo.vendor = QString::fromUtf8(devList[i]->vendor);
                deviceInfo.model = QString::fromUtf8(devList[i]->model);
                deviceInfo.type = type;
                m_deviceList << deviceInfo;
                qCDebug(KSANECORE_LOG) << "Adding device " << deviceInfo.vendor <<
                deviceInfo.name << deviceInfo.model << deviceInfo.type << " to device list";
            } else {
                qCDebug(KSANECORE_LOG) << "Ignoring device type" << type;
            }
            i++;
        }
    }
}

const QList<CoreInterface::DeviceInfo> FindSaneDevicesThread::devicesList() const
{
    return m_deviceList;
}

void FindSaneDevicesThread::setDeviceType(const CoreInterface::DeviceType type)
{
    m_deviceType = type;
}

} // namespace KSane
