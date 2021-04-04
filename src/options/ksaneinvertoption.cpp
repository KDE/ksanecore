/* ============================================================
 *
 * Date        : 2021-03-21
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#include "ksaneinvertoption.h"

#include <ksane_debug.h>

namespace KSaneIface
{

KSaneInvertOption::KSaneInvertOption()
{
    m_optionType = KSaneOption::TypeBool;
}

bool KSaneInvertOption::setValue(const QVariant &value)
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

QVariant KSaneInvertOption::getValue() const
{
    return m_checked;
}

QString KSaneInvertOption::getValueAsString() const
{
    if (m_checked) {
        return QStringLiteral("true");
    } else {
        return QStringLiteral("false");
    }
}

KSaneOption::KSaneOptionState KSaneInvertOption::state() const
{
    return StateActive;
}

QString KSaneInvertOption::name() const
{
    return InvertColorsOptionName;
}

QString KSaneInvertOption::title() const
{
    return i18n("Invert colors");
}

QString KSaneInvertOption::description() const
{
    return i18n("Invert the colors of the scanned image.");
}

}  // NameSpace KSaneIface