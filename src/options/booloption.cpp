/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "booloption.h"

#include <QVarLengthArray>

#include <ksanecore_debug.h>

namespace KSaneCore
{

BoolOption::BoolOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = Option::TypeBool;
}

bool BoolOption::setValue(const QVariant &value)
{
    if (state() == Option::StateHidden) {
        return false;
    }

    bool toggled = value.toBool();

    if (m_checked != toggled) {
        unsigned char data[4];

        m_checked = toggled;
        fromSANE_Word(data, (toggled) ? 1 : 0);
        writeData(data);
        Q_EMIT valueChanged(m_checked);
    }
    return true;
}

void BoolOption::readValue()
{
    if (state() == Option::StateHidden) {
        return;
    }

    // read the current value
    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        return;
    }
    bool old = m_checked;
    m_checked = (toSANE_Word(data.data()) != 0) ? true : false;

    if ((old != m_checked) && ((m_optDesc->cap & SANE_CAP_SOFT_SELECT) == 0)) {
        Q_EMIT valueChanged(m_checked);
    }
}

QVariant BoolOption::value() const
{
    if (state() == Option::StateHidden) {
        return QVariant();
    }
    return m_checked;
}

QString BoolOption::valueAsString() const
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

} // namespace KSaneCore

#include "moc_booloption.cpp"
