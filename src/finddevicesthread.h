/*
 * SPDX-FileCopyrightText: 2007-2008 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_FIND_DEVICES_THREAD_H
#define KSANE_FIND_DEVICES_THREAD_H

#include "deviceinformation.h"
#include "interface.h"

#include <QThread>
#include <QList>

namespace KSaneCore
{

class FindSaneDevicesThread : public QThread
{
    Q_OBJECT

public:
    static FindSaneDevicesThread *getInstance();
    ~FindSaneDevicesThread() override;
    void run() override;

    QList<DeviceInformation *> devicesList() const;
    void setDeviceType(const Interface::DeviceType type);

private:
    FindSaneDevicesThread();

    QList<DeviceInformation *> m_deviceList;
    Interface::DeviceType m_deviceType = Interface::AllDevices;
};

} // namespace KSaneCore

#endif // KSANE_FIND_DEVICES_THREAD_H
