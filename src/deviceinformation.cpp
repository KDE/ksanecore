/*
 * SPDX-FileCopyrightText: 2022 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "deviceinformation.h"
#include "deviceinformation_p.h"

namespace KSane
{

DeviceInformation::DeviceInformation() : d(std::make_unique<DeviceInformationPrivate>())
{
}

DeviceInformation::~DeviceInformation()
= default;

QString DeviceInformation::name() const
{
    return d->name;
}

QString DeviceInformation::vendor() const
{
    return d->vendor;
}

QString DeviceInformation::model() const
{
    return d->model;
}

QString DeviceInformation::type() const
{
    return d->type;
}

} // namespace KSane
