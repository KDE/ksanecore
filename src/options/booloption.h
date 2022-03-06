/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_BOOL_OPTION_H
#define KSANE_BOOL_OPTION_H

#include "baseoption.h"

namespace KSane
{

class BoolOption : public BaseOption
{
    Q_OBJECT

public:
    BoolOption(const SANE_Handle handle, const int index);

    void readValue() override;

    QVariant value() const override;
    QString valueAsString() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    bool m_checked = false;
};

} // namespace KSane

#endif // KSANE_BOOL_OPTION_H
