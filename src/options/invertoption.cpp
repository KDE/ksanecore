/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "invertoption.h"

#include <ksanecore_debug.h>

namespace KSane
{

InvertOption::InvertOption()
{
    m_optionType = CoreOption::TypeBool;
}

bool InvertOption::setValue(const QVariant &value)
{
    if (value.canConvert<bool>()) {
        if (m_checked != value.toBool()) {
            m_checked = value.toBool();
            Q_EMIT valueChanged(m_checked);
        }
        return true;
    } else {
        return false;
    }
}

QVariant InvertOption::value() const
{
    return m_checked;
}

QString InvertOption::valueAsString() const
{
    if (m_checked) {
        return QStringLiteral("true");
    } else {
        return QStringLiteral("false");
    }
}

CoreOption::OptionState InvertOption::state() const
{
    return CoreOption::StateActive;
}

QString InvertOption::name() const
{
    return InvertColorsOptionName;
}

QString InvertOption::title() const
{
    return i18n("Invert colors");
}

QString InvertOption::description() const
{
    return i18n("Invert the colors of the scanned image.");
}

}  // namespace KSane
