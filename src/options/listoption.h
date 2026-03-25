/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2026 Tobias Leupold <tl@stonemx.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_VALUELIST_OPTION_H
#define KSANE_VALUELIST_OPTION_H

#include "baseoption.h"

namespace KSaneCore
{

class ListOption : public BaseOption
{
    Q_OBJECT

public:
    ListOption(const SANE_Handle handle, const int index);

    void readValue() override;
    void readOption() override;

    QVariant minimumValue() const override;
    QVariant value() const override;
    QVariant internalValue() const override;
    QString valueAsString() const override;
    QString internalValueAsString() const override;
    QVariantList valueList() const override;
    QVariantList internalValueList() const override;
    Option::OptionState state() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    bool setValue(double value);
    bool setValue(const QString &value);
    void countEntries();

    QVariant m_currentValue;
    QVariant m_currentInternalValue;
    int m_entriesCount = 0;
};

} // namespace KSaneCore

#endif // KSANE_VALUELIST_OPTION_H
