/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "batchmodeoption.h"

#include <ksanecore_debug.h>

namespace KSaneCore
{

BatchModeOption::BatchModeOption()
{
    m_optionType = Option::TypeBool;
}

Option::OptionState BatchModeOption::state() const
{
    return Option::StateActive;
}

QString BatchModeOption::name() const
{
    return BatchModeOptionName;
}

QString BatchModeOption::title() const
{
    return i18n("Batch mode with time delay");
}

QString BatchModeOption::description() const
{
    return i18n("Enables batch mode scanning. Continues scanning after a delay until canceled.");
}

bool BatchModeOption::setValue(const QVariant &value)
{
    const bool toggled = value.toBool();

    if (m_checked != toggled) {
        m_checked = toggled;
        Q_EMIT valueChanged(m_checked);
    }
    return true;
}

QVariant BatchModeOption::value() const
{
    return m_checked;
}

QString BatchModeOption::valueAsString() const
{
    if (state() == Option::StateHidden) {
        return QString();
    }
    if (m_checked) {
        return QStringLiteral("true");
    } else {
        return QStringLiteral("false");
    }
}

} // NameSpace KSaneCore

#include "moc_batchmodeoption.cpp"
