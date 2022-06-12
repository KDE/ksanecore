/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_COREOPTION_P_H
#define KSANE_COREOPTION_P_H

#include "baseoption.h"

namespace KSaneCore
{

class OptionPrivate
{
public:
    BaseOption *option;
};

} // namespace KSaneCore

#endif // KSANE_COREOPTION_P_H
