/*
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */


#include "internaloption.h"
#include "coreoption_p.h"

namespace KSane
{

InternalOption::InternalOption(BaseOption *option, QObject *parent) : CoreOption(parent)
{
    d->option = option;
    connect(d->option, &BaseOption::optionReloaded, this, &CoreOption::optionReloaded);
    connect(d->option, &BaseOption::valueChanged, this, &CoreOption::valueChanged);
    connect(d->option, &BaseOption::destroyed, this, [=]() {
        d->option = nullptr;
    });
}

}  // namespace KSane
