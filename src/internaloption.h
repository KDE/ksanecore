/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_INTERNAL_OPTION_H
#define KSANE_INTERNAL_OPTION_H

#include "baseoption.h"
#include "option.h"

namespace KSaneCore
{

class InternalOption : public Option
{
    Q_OBJECT

public:

    explicit InternalOption(BaseOption *option, QObject *parent = nullptr);

};

} // namespace KSaneCore

#endif // KSANE_INTERNAL_OPTION_H

