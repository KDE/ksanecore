/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_DOUBLE_OPTION_H
#define KSANE_DOUBLE_OPTION_H

#include "baseoption.h"

namespace KSaneCore
{

class DoubleOption : public BaseOption
{
    Q_OBJECT

public:
    DoubleOption(const SANE_Handle handle, const int index);

    void readValue() override;
    void readOption() override;

    QVariant minimumValue() const override;
    QVariant maximumValue() const override;
    QVariant stepValue() const override;
    QVariant value() const override;
    QString valueAsString() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    double m_value = 0;
    double m_minChange = 0.0001;
};

} // namespace KSaneCore

#endif // KSANE_DOUBLE_OPTION_H
