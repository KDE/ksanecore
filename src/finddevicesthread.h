/*
 * SPDX-FileCopyrightText: 2007-2008 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_FIND_DEVICES_THREAD_H
#define KSANE_FIND_DEVICES_THREAD_H

#include "coreinterface.h"

#include <QThread>
#include <QList>

namespace KSane
{

class FindSaneDevicesThread : public QThread
{
    Q_OBJECT

public:
    static FindSaneDevicesThread *getInstance();
    ~FindSaneDevicesThread() override;
    void run() override;

    const QList<CoreInterface::DeviceInfo> devicesList() const;
    void setDeviceType(const CoreInterface::DeviceType type);

private:
    FindSaneDevicesThread();

    QList<CoreInterface::DeviceInfo> m_deviceList;
    CoreInterface::DeviceType m_deviceType = CoreInterface::AllDevices;
};

} // namespace KSane

#endif // KSANE_FIND_DEVICES_THREAD_H
