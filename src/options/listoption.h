/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_VALUELIST_OPTION_H
#define KSANE_VALUELIST_OPTION_H

#include "baseoption.h"

namespace KSane
{

class ListOption : public BaseOption
{
    Q_OBJECT

public:
    ListOption(const SANE_Handle handle, const int index);

    void readValue() override;

    QVariant minimumValue() const override;
    QVariant value() const override;
    QString valueAsString() const override;
    QVariantList valueList() const override;
    QVariantList internalValueList() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    bool setValue(double value);
    bool setValue(const QString &value);

    QVariant       m_currentValue;
};

} // namespace KSane

#endif // KSANE_VALUELIST_OPTION_H
