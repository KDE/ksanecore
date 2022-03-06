/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_DELAY_OPTION_H
#define KSANE_DELAY_OPTION_H

#include "baseoption.h"
#include "coreoption.h"

namespace KSane
{

static const QString BatchDelayOptionName = QStringLiteral("KSane::BatchTimeDelay");

class BatchDelayOption : public BaseOption
{
    Q_OBJECT

public:
    BatchDelayOption();

    CoreOption::OptionState state() const override;
    QString name() const override;
    QString title() const override;
    QString description() const override;

    QVariant minimumValue() const override;
    QVariant maximumValue() const override;
    QVariant stepValue() const override;
    QVariant value() const override;
    QString valueAsString() const override;
    CoreOption::OptionUnit valueUnit() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    CoreOption::OptionState m_state = CoreOption::StateHidden;
    int m_delayValue = 10;
};

}  // NameSpace KSane

#endif // KSANE_DELAY_OPTION_H
