/*
 * SPDX-FileCopyrightText: 2022 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_DEVICEINFORMATION_P_H
#define KSANE_DEVICEINFORMATION_P_H

#include "baseoption.h"

namespace KSane
{

class DeviceInformationPrivate
{
public:
    QString name;     /* unique device name */
    QString vendor;   /* device vendor string */
    QString model;    /* device model name */
    QString type;     /* device type (e.g., "flatbed scanner") */
};

}  // namespace KSane

#endif // KSANE_DEVICEINFORMATION_P_H

