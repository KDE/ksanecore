/*
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */


#include "internaloption.h"
#include "option_p.h"

namespace KSaneCore
{

InternalOption::InternalOption(BaseOption *option, QObject *parent)
    : Option(parent)
{
    d->option = option;
    connect(d->option, &BaseOption::optionReloaded, this, &Option::optionReloaded);
    connect(d->option, &BaseOption::valueChanged, this, &Option::valueChanged);
    connect(d->option, &BaseOption::destroyed, this, [=]() {
        d->option = nullptr;
    });
}

} // namespace KSaneCore

#include "moc_internaloption.cpp"
