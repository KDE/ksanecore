/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_OPTION_P_H
#define KSANE_OPTION_P_H

#include "ksanebaseoption.h"

namespace KSane
{

class KSaneOptionPrivate
{
public:
    KSaneBaseOption *option;
};

}  // namespace KSane

#endif // KSANE_OPTION_P_H

