/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_DELAY_OPTION_H
#define KSANE_DELAY_OPTION_H

#include "baseoption.h"
#include "option.h"

namespace KSaneCore
{

static const QString BatchDelayOptionName = QStringLiteral("KSane::BatchTimeDelay");

class BatchDelayOption : public BaseOption
{
    Q_OBJECT

public:
    BatchDelayOption();

    Option::OptionState state() const override;
    QString name() const override;
    QString title() const override;
    QString description() const override;

    QVariant minimumValue() const override;
    QVariant maximumValue() const override;
    QVariant stepValue() const override;
    QVariant value() const override;
    QString valueAsString() const override;
    Option::OptionUnit valueUnit() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    Option::OptionState m_state = Option::StateHidden;
    int m_delayValue = 10;
};

} // NameSpace KSaneCore

#endif // KSANE_DELAY_OPTION_H
