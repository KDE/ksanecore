/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_STRING_OPTION_H
#define KSANE_STRING_OPTION_H

#include "ksanebaseoption.h"

namespace KSane
{

class KSaneStringOption : public KSaneBaseOption
{
    Q_OBJECT

public:
    KSaneStringOption(const SANE_Handle handle, const int index);

    void readValue() override;

    QVariant value() const override;
    int valueSize() const override;
    QString valueAsString() const override;

public Q_SLOTS:
    bool setValue(const QVariant &val) override;

private:
    QString       m_string;
};

} // namespace KSane

#endif // KSANE_STRING_OPTION_H
