/*
 * SPDX-FileCopyrightText: 2022 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_DEVICEINFORMATION_H
#define KSANE_DEVICEINFORMATION_H

#include <memory>

// Qt includes
#include <QString>

#include "ksanecore_export.h"

namespace KSaneCore
{

class DeviceInformationPrivate;

/**
 * A wrapper class providing access to the internal information
 * of the scanner device provided by SANE
 */
class KSANECORE_EXPORT DeviceInformation
{

public:
    explicit DeviceInformation();
    ~DeviceInformation();

    /** This function returns a unique device name for the scanner device
     * @return the unique device name name */
    QString name() const;

    /** This function returns the device vendor string of the scanner device
     * @return the device vendor string */
    QString vendor() const;

    /** This function returns the device vendor string of the scanner device
     * @return the device model name */
    QString model() const;

    /** This function returns the device type (e.g., "flatbed scanner")
     * @return the device type */
    QString type() const;

protected:
    std::unique_ptr<KSaneCore::DeviceInformationPrivate> d;
};

} // namespace KSaneCore

#endif // KSANE_DEVICEINFORMATION_H

