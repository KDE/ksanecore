/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "batchdelayoption.h"
#include <ksanecore_debug.h>

namespace KSane
{

BatchDelayOption::BatchDelayOption()
{
    m_optionType = CoreOption::TypeInteger;
}

CoreOption::OptionState BatchDelayOption::state() const
{
    return CoreOption::StateActive;;
}

QString BatchDelayOption::name() const
{
    return BatchDelayOptionName;
}

QString BatchDelayOption::title() const
{
    return i18n("Batch mode time delay");
}

QString BatchDelayOption::description() const
{
    return i18n("Specify the time delay between each scan when batch mode is enabled.");
}

QVariant BatchDelayOption::minimumValue() const
{
    return 0;
}

QVariant BatchDelayOption::maximumValue() const
{
    return 300;
}

QVariant BatchDelayOption::stepValue() const
{
    return 1;
}

QVariant BatchDelayOption::value() const
{
    return m_delayValue;
}

QString BatchDelayOption::valueAsString() const
{
    return QString::number(m_delayValue);
}

CoreOption::OptionUnit BatchDelayOption::valueUnit() const
{
    return CoreOption::UnitSecond;
}

bool BatchDelayOption::setValue(const QVariant &val)
{
    bool ok;
    int newValue = val.toInt(&ok);
    if (ok && newValue != m_delayValue) {
        m_delayValue = newValue;
        Q_EMIT valueChanged(m_delayValue);
    }
    return ok;
}

}  // NameSpace KSane
