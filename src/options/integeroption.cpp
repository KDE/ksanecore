/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "integeroption.h"

#include <QVarLengthArray>

static const int KSW_INT_MAX = 2147483647;
static const int KSW_INT_MIN = -2147483647 - 1; // prevent warning

namespace KSaneCore
{

IntegerOption::IntegerOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = Option::TypeInteger;
}

void IntegerOption::readValue()
{
    if (state() == Option::StateHidden) {
        return;
    }

    // read that current value
    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        return;
    }

    int newValue = toSANE_Word(data.data());
    if (newValue != m_iVal) {
        m_iVal = newValue;
        Q_EMIT valueChanged(m_iVal);
    }
}

QVariant IntegerOption::minimumValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = static_cast<int>(m_optDesc->constraint.range->min);
    } else {
        value = KSW_INT_MIN;
    }
    return value;
}

QVariant IntegerOption::maximumValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = static_cast<int>(m_optDesc->constraint.range->max);
    } else {
        value = KSW_INT_MAX;
    }
    return value;
}

QVariant IntegerOption::stepValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = static_cast<int>(m_optDesc->constraint.range->quant);
        // guard against possible broken backends
        if (value == 0) {
            value = 1;
        }
    } else {
        value = 1;
    }
    return value;
}

QVariant IntegerOption::value() const
{
    QVariant value;
    if (state() == Option::StateHidden) {
        return value;
    }
    value = m_iVal;
    return value;
}

QString IntegerOption::valueAsString() const
{
    if (state() == Option::StateHidden) {
        return QString();
    }
    return QString::number(m_iVal);
}

bool IntegerOption::setValue(const QVariant &val)
{
    bool ok;
    int newValue = val.toInt(&ok);
    if (ok && newValue != m_iVal) {
        unsigned char data[4];
        m_iVal = newValue;
        fromSANE_Word(data, newValue);
        writeData(data);
        Q_EMIT valueChanged(m_iVal);
    }
    return ok;
}

} // namespace KSaneCore
