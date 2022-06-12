/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "actionoption.h"

#include <ksanecore_debug.h>

namespace KSaneCore
{

ActionOption::ActionOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = Option::TypeAction;
}

bool ActionOption::setValue(const QVariant &)
{
    unsigned char data[4];
    writeData(data);
    return true;
}

} // namespace KSaneCore
