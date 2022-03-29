/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "doubleoption.h"

#include <QVarLengthArray>

#include <ksanecore_debug.h>

static const double FIXED_MAX = 32767.9999;
static const double FIXED_MIN = -32768.0;
static const double MIN_FIXED_STEP = 0.0001;
static const double FIXED_PRECISION = 1.0 / 65536;

namespace KSane
{

DoubleOption::DoubleOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = CoreOption::TypeDouble;
}

void DoubleOption::readOption()
{
    BaseOption::readOption();

    double step = MIN_FIXED_STEP;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        step = SANE_UNFIX(m_optDesc->constraint.range->quant);
        if (step < MIN_FIXED_STEP) {
            step = MIN_FIXED_STEP;
        }
    }
    m_minChange = step;
}

void DoubleOption::readValue()
{
    if (state() == CoreOption::StateHidden) {
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

    double newValue = SANE_UNFIX(toSANE_Word(data.data()));
    if (abs(newValue - m_value) >= FIXED_PRECISION) {
        m_value = newValue;
        Q_EMIT valueChanged(m_value);
    }
}

bool DoubleOption::setValue(const QVariant &value)
{
    if (state() == CoreOption::StateHidden) {
        return false;
    }
    bool ok;
    double newValue = value.toDouble(&ok);
    if (ok && abs(newValue - m_value) >= m_minChange) {
        unsigned char data[4];
        SANE_Word fixed;
        //qCDebug(KSANE_LOG) <<m_optDesc->name << fVal << "!=" << val;
        m_value = newValue;
        fixed = SANE_FIX(newValue);
        fromSANE_Word(data, fixed);
        writeData(data);
        Q_EMIT valueChanged(m_value);
    }
    return ok;
}

QVariant DoubleOption::minimumValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = SANE_UNFIX(m_optDesc->constraint.range->min);
    } else {
        value = FIXED_MIN;
    }
    return value;
}

QVariant DoubleOption::maximumValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = SANE_UNFIX(m_optDesc->constraint.range->max);
    } else {
        value = FIXED_MAX;
    }
    return value;
}

QVariant DoubleOption::stepValue() const
{
    QVariant value;
    if (m_optDesc->constraint_type == SANE_CONSTRAINT_RANGE) {
        value = SANE_UNFIX(m_optDesc->constraint.range->quant);
        /* work around broken backends, the step value must never be zero
         * assume a minimum step value of 0.1 */
        if (value == 0) {
            value = 0.1;
        }
    } else {
        value = MIN_FIXED_STEP;
    }
    return value;
}

QVariant DoubleOption::value() const
{
    if (state() == CoreOption::StateHidden) {
        return QVariant();
    }
    return QVariant(m_value);
}

QString DoubleOption::valueAsString() const
{
    if (state() == CoreOption::StateHidden) {
        return QString();
    }
    return QString::number(m_value, 'F', 6);
}

} // namespace KSane
